#include <FastLED.h>
#define LED_PIN GPIO_NUM_1 // пин ленты
// #define DATA_PIN            DATA_PIN
#define NUM_LEDS 160 // кол-во светодиодов
#define LED_TYPE WS2811
#define COLOR_ORDER BRG

CRGB leds[NUM_LEDS];
byte heat[NUM_LEDS];

int TypeAlg;
uint8_t Col;
extern int NLeds;
int Vsp_INTERVAL = 1000;

String AlgStr = "cylon;fill;layeredEffect;twinkleFade;blurredChase;breathe;blendedChase;fire;pacifica;Lum1;";

enum class Alg
{
    cylon,
    fill,
    layeredEffect,
    twinkleFade,
    blurredChase,
    breathe,
    blendedChase,
    fire,
    pacifica,
    Lum1,
};

const uint32_t updateInterval = 20; // 20 мс = 50 кадров в секунду

enum class FluorescentPhase : uint8_t
{
    // Полный перезапуск сценария розжига.
    reset,
    // Небольшая пауза в темноте перед "оживанием" лампы.
    offDelay,
    // Прогрев электродов: на концах появляется тёплое свечение.
    preheat,
    // Серия неудачных попыток зажечь дугу: резкие вспышки и провалы.
    strike,
    // Лампа уже почти запустилась, но ещё плавает по яркости.
    stabilize,
    // Нормальная работа с редкими микросбоями.
    steady
};

struct FluorescentLampState
{
    // Текущая стадия сценария розжига лампы.
    FluorescentPhase phase = FluorescentPhase::reset;
    // Время входа в текущую фазу: нужно для расчёта её длительности.
    uint32_t phaseStartedAt = 0;
    // Момент следующего переключения или обновления внутри текущей фазы.
    uint32_t nextChangeAt = 0;
    // Сколько ещё резких вспышек осталось до устойчивого свечения.
    uint8_t remainingBursts = 0;
    // Базовая яркость на этапе стабилизации.
    uint8_t settleBrightness = 0;
    // В текущий момент дуга "схватилась" или лампа снова погасла.
    bool outputOn = false;
};

FluorescentLampState lumLamp;

CRGBPalette16 pacifica_palette_1 =
    {0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117,
     0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x14554B, 0x28AA50};
CRGBPalette16 pacifica_palette_2 =
    {0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117,
     0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x0C5F52, 0x19BE5F};
CRGBPalette16 pacifica_palette_3 =
    {0x000208, 0x00030E, 0x000514, 0x00061A, 0x000820, 0x000927, 0x000B2D, 0x000C33,
     0x000E39, 0x001040, 0x001450, 0x001860, 0x001C70, 0x002080, 0x1040BF, 0x2060FF};

void pacifica_one_layer(fl::CRGBPalette16 &p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff);
void pacifica_loop();
void pacifica_add_whitecaps();
void pacifica_deepen_colors();

void pacifica_loop()
{
    // Increment the four "color index start" counters, one for each wave layer.
    // Each is incremented at a different speed, and the speeds vary over time.
    static uint16_t sCIStart1, sCIStart2, sCIStart3, sCIStart4;
    static uint32_t sLastms = 0;
    uint32_t ms = GET_MILLIS();
    uint32_t deltams = ms - sLastms;
    sLastms = ms;
    uint16_t speedfactor1 = beatsin16(3, 179, 269);
    uint16_t speedfactor2 = beatsin16(4, 179, 269);
    uint32_t deltams1 = (deltams * speedfactor1) / 256;
    uint32_t deltams2 = (deltams * speedfactor2) / 256;
    uint32_t deltams21 = (deltams1 + deltams2) / 2;
    sCIStart1 += (deltams1 * beatsin88(1011, 10, 13));
    sCIStart2 -= (deltams21 * beatsin88(777, 8, 11));
    sCIStart3 -= (deltams1 * beatsin88(501, 5, 7));
    sCIStart4 -= (deltams2 * beatsin88(257, 4, 6));

    // Clear out the LED fl::array to a dim background blue-green
    fill_solid(leds, NUM_LEDS, CRGB(2, 6, 10));

    // Render each of four layers, with different scales and speeds, that vary over time
    pacifica_one_layer(pacifica_palette_1, sCIStart1, beatsin16(3, 11 * 256, 14 * 256), beatsin8(10, 70, 130), 0 - beat16(301));
    pacifica_one_layer(pacifica_palette_2, sCIStart2, beatsin16(4, 6 * 256, 9 * 256), beatsin8(17, 40, 80), beat16(401));
    pacifica_one_layer(pacifica_palette_3, sCIStart3, 6 * 256, beatsin8(9, 10, 38), 0 - beat16(503));
    pacifica_one_layer(pacifica_palette_3, sCIStart4, 5 * 256, beatsin8(8, 10, 28), beat16(601));

    // Add brighter 'whitecaps' where the waves lines up more
    pacifica_add_whitecaps();

    // Deepen the blues and greens a bit
    pacifica_deepen_colors();
}

// Add one layer of waves into the led fl::array
void pacifica_one_layer(CRGBPalette16 &p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff)
{
    uint16_t ci = cistart;
    uint16_t waveangle = ioff;
    uint16_t wavescale_half = (wavescale / 2) + 20;
    for (uint16_t i = 0; i < NUM_LEDS; i++)
    {
        waveangle += 250;
        uint16_t s16 = sin16(waveangle) + 32768;
        uint16_t cs = scale16(s16, wavescale_half) + wavescale_half;
        ci += cs;
        uint16_t sindex16 = sin16(ci) + 32768;
        uint8_t sindex8 = scale16(sindex16, 240);
        CRGB c = ColorFromPalette(p, sindex8, bri, LINEARBLEND);
        leds[i] += c;
    }
}

// Add extra 'white' to areas where the four layers of light have lined up brightly
void pacifica_add_whitecaps()
{
    uint8_t basethreshold = beatsin8(9, 55, 65);
    uint8_t wave = beat8(7);

    for (uint16_t i = 0; i < NUM_LEDS; i++)
    {
        uint8_t threshold = scale8(sin8(wave), 20) + basethreshold;
        wave += 7;
        uint8_t l = leds[i].getAverageLight();
        if (l > threshold)
        {
            uint8_t overage = l - threshold;
            uint8_t overage2 = qadd8(overage, overage);
            leds[i] += CRGB(overage, overage2, qadd8(overage2, overage2));
        }
    }
}

// Deepen the blues and greens
void pacifica_deepen_colors()
{
    for (uint16_t i = 0; i < NUM_LEDS; i++)
    {
        leds[i].blue = scale8(leds[i].blue, 145);
        leds[i].green = scale8(leds[i].green, 200);
        leds[i] |= CRGB(2, 5, 7);
    }
}

void fire()
{
    // Step 1: Cool down every cell a little
    for (int i = 0; i < NUM_LEDS; i++)
    {
        heat[i] = qsub8(heat[i], random8(0, ((55 * 10) / NUM_LEDS) + 2));
    }

    // Step 2: Heat from each cell drifts 'up' and diffuses a little
    for (int k = NUM_LEDS - 1; k >= 2; k--)
    {
        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
    }

    // Step 3: Randomly ignite new 'sparks' at the bottom
    if (random8() < 120)
    {
        int y = random8(7);
        heat[y] = qadd8(heat[y], random8(160, 255));
    }

    // Step 4: Map heat to LED colors
    for (int j = 0; j < NUM_LEDS; j++)
    {
        // Scale heat to 0-240 for palette index
        byte colorIndex = scale8(heat[j], 240);

        // Use HeatColors palette
        leds[j] = ColorFromPalette(OceanColors_p, colorIndex);
    }
}
void layeredEffect()
{
    // Layer 1: Slow moving wave
    static uint8_t wave1 = 0;
    for (int i = 0; i < NUM_LEDS; i++)
    {
        uint8_t brightness = sin8(wave1 + (i * 10));
        leds[i] = CHSV(160, 255, brightness / 2);
    }
    wave1 += 2;

    // Layer 2: Fast twinkles (additive)
    if (random8() < 30)
    {
        int pos = random16(NUM_LEDS);
        leds[pos] += CRGB(50, 50, 50);
    }
}

void twinkleFade()
{
    // Fade all LEDs
    fadeToBlackBy(leds, NUM_LEDS, 32);

    // Add new twinkles
    if (random8() < 50)
    {
        int pos = random16(NUM_LEDS);
        leds[pos] = CHSV(random8(), 200, 255);
    }
}

void blurredChase()
{
    static uint8_t hue = 0;
    static uint8_t pos = 0;

    // Fade and blur
    fadeToBlackBy(leds, NUM_LEDS, 20);
    blur1d(leds, NUM_LEDS, 60);

    // Add new position
    leds[pos] = CHSV(hue, 255, 255);

    pos = (pos + 1) % NUM_LEDS;
    hue += 3;
}

void breathe(CRGB color)
{
    uint8_t brightness = beatsin8(12, 50, 255); // Smooth oscillation

    CRGB dimmedColor = color;
    dimmedColor.nscale8(brightness);

    fill_solid(leds, NUM_LEDS, dimmedColor);
}

void cylon()
{
    static uint8_t pos = 0;
    static int8_t direction = 1;

    // Погасить все светодиоды
    fadeToBlackBy(leds, NUM_LEDS, 20);

    // Установка текущей позиции
    leds[pos] = Col;

    // Перемещение позиции
    pos += direction;
    if (pos == 0 || pos == NUM_LEDS - 1)
    {
        direction = -direction;
    }
}

void blendedChase()
{
    static uint8_t pos = 0;
    static uint8_t hue = 0;

    // Fade all LEDs gradually
    fadeToBlackBy(leds, NUM_LEDS, 30);

    // Add bright LED at current position
    leds[pos] = CHSV(hue, 255, 255);

    // Blur to create smooth trail
    blur1d(leds, NUM_LEDS, 80);

    // Move to next position
    EVERY_N_MILLISECONDS(50)
    {
        pos = (pos + 1) % NUM_LEDS;
        hue += 4;
    }
}

CRGB scaleColor(const CRGB &color, uint8_t brightness)
{
    CRGB scaled = color;
    scaled.nscale8_video(brightness);
    return scaled;
}

void drawFluorescentTube(uint8_t brightness, uint8_t noiseAmount = 0, uint8_t warmEdge = 0, bool addDarkGap = false)
{
    // Холодный бело-зелёный оттенок характерен для люминесцентной лампы.
    const CRGB tubeTint = CRGB(200, 255, 228);
    fill_solid(leds, NUM_LEDS, scaleColor(tubeTint, brightness));

    if (noiseAmount > 0)
    {
        // Небольшой случайный разброс яркости делает свечение "грязным".
        for (int i = 0; i < NUM_LEDS; i++)
        {
            leds[i].nscale8_video(qsub8(255, random8(noiseAmount + 1)));
        }
    }

    if (addDarkGap && NUM_LEDS > 12)
    {
        // Иногда дуга зажигает трубку неравномерно, оставляя тёмный провал.
        int gapStart = random16(NUM_LEDS - 6);
        int gapLength = 2 + random8(5);

        for (int i = gapStart; i < NUM_LEDS && i < gapStart + gapLength; i++)
        {
            leds[i].fadeToBlackBy(120 + random8(80));
        }
    }

    if (warmEdge > 0)
    {
        // На старте концы лампы могут светиться теплее основного тела трубки.
        CRGB filamentTint = scaleColor(CRGB(255, 120, 18), warmEdge);
        const uint8_t edgeSize = NUM_LEDS >= 60 ? 5 : 3;

        for (uint8_t i = 0; i < edgeSize; i++)
        {
            leds[i] += filamentTint;
            leds[NUM_LEDS - 1 - i] += filamentTint;
        }
    }

    // Лёгкий blur сглаживает отдельные точки и делает "ламповый" объём.
    blur1d(leds, NUM_LEDS, addDarkGap ? 48 : 24);
}

void resetFluorescentLamp(uint32_t now)
{
    // Каждый новый запуск начинается с короткой темноты и случайного числа вспышек,
    // чтобы эффект не выглядел одинаково при каждом выборе алгоритма.
    lumLamp.phase = FluorescentPhase::offDelay;
    lumLamp.phaseStartedAt = now;
    lumLamp.nextChangeAt = now + 120 + random16(450);
    lumLamp.remainingBursts = 6 + random8(8);
    lumLamp.settleBrightness = 0;
    lumLamp.outputOn = false;

    FastLED.clear();
}

void Lum1()
{
    uint32_t now = millis();

    if (lumLamp.phase == FluorescentPhase::reset)
    {
        resetFluorescentLamp(now);
    }

    switch (lumLamp.phase)
    {
    case FluorescentPhase::offDelay:
        // Полностью тёмная лампа перед стартом.
        fill_solid(leds, NUM_LEDS, CRGB::Black);

        if (now >= lumLamp.nextChangeAt)
        {
            lumLamp.phase = FluorescentPhase::preheat;
            lumLamp.phaseStartedAt = now;
            lumLamp.nextChangeAt = now + 350 + random16(650);
        }
        break;

    case FluorescentPhase::preheat:
    {
        // Плавный тёплый прогрев концов и редкие слабые пробои по трубке.
        uint8_t edgeWarm = beatsin8(18, 24, 90);
        bool faintArc = (now - lumLamp.phaseStartedAt > 150) && (random8() < 70);

        drawFluorescentTube(faintArc ? (14 + random8(28)) : 0, 140, edgeWarm, faintArc);

        if (now >= lumLamp.nextChangeAt)
        {
            lumLamp.phase = FluorescentPhase::strike;
            lumLamp.phaseStartedAt = now;
            lumLamp.nextChangeAt = now;
            lumLamp.outputOn = false;
        }
        break;
    }

    case FluorescentPhase::strike:
        // Главная "нервная" стадия: лампа то вспыхивает, то снова срывается.
        if (now >= lumLamp.nextChangeAt)
        {
            lumLamp.outputOn = !lumLamp.outputOn;
            lumLamp.nextChangeAt = now + (lumLamp.outputOn ? 20 + random8(70) : 35 + random8(120));

            if (!lumLamp.outputOn && lumLamp.remainingBursts > 0)
            {
                lumLamp.remainingBursts--;
            }

            if (lumLamp.remainingBursts == 0 && !lumLamp.outputOn)
            {
                lumLamp.phase = FluorescentPhase::stabilize;
                lumLamp.phaseStartedAt = now;
                lumLamp.nextChangeAt = now + 40;
                lumLamp.settleBrightness = 90;
            }
        }

        if (lumLamp.outputOn)
        {
            // Резкая яркая вспышка, но ещё с шумом и провалами.
            drawFluorescentTube(150 + random8(106), 70 + random8(50), 10 + random8(24), true);
        }
        else
        {
            // Почти полное гашение, иногда со слабым остаточным свечением.
            drawFluorescentTube(random8() < 200 ? 0 : (12 + random8(20)), 180, 25 + random8(30), true);
        }
        break;

    case FluorescentPhase::stabilize:
        // После удачного пробоя лампа постепенно набирает рабочую яркость.
        if (now >= lumLamp.nextChangeAt)
        {
            lumLamp.settleBrightness = qadd8(lumLamp.settleBrightness, 10 + random8(18));
            lumLamp.nextChangeAt = now + 45 + random8(90);

            if (now - lumLamp.phaseStartedAt > 1200 + random16(1200))
            {
                lumLamp.phase = FluorescentPhase::steady;
                lumLamp.phaseStartedAt = now;
                lumLamp.nextChangeAt = now + max(250, Vsp_INTERVAL / 2) + random16(max(1, Vsp_INTERVAL));
            }
        }

        // Яркость растёт, но пока ещё возможны локальные провалы и дрожание.
        drawFluorescentTube(qsub8(lumLamp.settleBrightness + random8(30), random8() < 60 ? random8(70) : 0),
                            40 + random8(30),
                            8 + random8(12),
                            random8() < 120);
        break;

    case FluorescentPhase::steady:
    {
        // Нормальный режим: почти стабильный свет с лёгкой сетевой пульсацией.
        uint8_t ripple = beatsin8(100, 188, 225);
        uint8_t slowDrift = beatsin8(7, 0, 12);
        drawFluorescentTube(qadd8(ripple, slowDrift), 10, 4, false);

        if (random8() < 10)
        {
            // Редкие микропровалы добавляют ощущение старой уставшей лампы.
            leds[random16(NUM_LEDS)].fadeToBlackBy(70);
        }

        if (now >= lumLamp.nextChangeAt)
        {
            // Иногда даже уже горящая лампа кратко "спотыкается" и перескакивает
            // обратно в короткую серию вспышек.
            lumLamp.phase = FluorescentPhase::strike;
            lumLamp.phaseStartedAt = now;
            lumLamp.nextChangeAt = now;
            lumLamp.remainingBursts = 1 + random8(3);
            lumLamp.outputOn = true;
        }
        break;
    }

    default:
        resetFluorescentLamp(now);
        break;
    }
}

void lum2()
{
    


}

namespace NEON
{

    void setup()
    {
        // NLeds = NUM_LEDS;
        FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
        FastLED.clear();
        FastLED.show();
        lumLamp.phase = FluorescentPhase::reset;
        // FastLED.setBrightness(50); // Начните с низкой яркости
    }

    void tick()
    {
        static int prevAlg = -1;

        EVERY_N_MILLISECONDS(updateInterval)
        {
            if (TypeAlg != prevAlg)
            {
                if (TypeAlg == (int)Alg::Lum1)
                {
                    // При повторном выборе эффекта запускаем весь сценарий заново.
                    lumLamp.phase = FluorescentPhase::reset;
                }

                prevAlg = TypeAlg;
            }

            switch (TypeAlg)
            {
            case (int)Alg::cylon:
                cylon();
                break;
            case (int)Alg::layeredEffect:
                layeredEffect();
                break;
            case (int)Alg::twinkleFade:
                twinkleFade();
                break;
            case (int)Alg::blurredChase:
                blurredChase();
                break;
            case (int)Alg::fire:
                fire();
                break;
            case (int)Alg::pacifica:
                pacifica_loop();
                break;
            case (int)Alg::breathe:
                breathe(Col);
                break;
            case (int)Alg::Lum1:
                Lum1();
                break;
            default:
                break;
            }

            FastLED.show();
        }
    }
}
