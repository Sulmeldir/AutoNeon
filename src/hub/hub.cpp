#include <GyverHub.h>
#include "project_meta.h"

GyverHub hub("AutoNEON", "ESP32-S3", "пѓ«");
extern int TypeAlg;
extern uint8_t Col;
extern String AlgStr;
extern int Vsp_INTERVAL;
int NLeds;

namespace HUB
{
  void build(gh::Builder &b)
  {
    b.Input(&NLeds).size(2);
    b.Select_("sel", &TypeAlg).text(AlgStr);
    b.Color(&Col);
    b.Slider(&Vsp_INTERVAL).text("Интервал вспышки").range(1, 2000, 1);
  }

  void setup()
  {
    hub.mqtt.config("m3.wqtt.ru", 14635, "u_9ICRMS", "n5V6oZGA");
    hub.setVersion(String(F(PROJECT_GH_REPO "@")) + F(PROJECT_FW_VERSION));
    // hub.mqtt.config("test.mosquitto.org", 8081);

    hub.onBuild(build);
    hub.begin();

    Serial.println("hub begin");
    hub.tick();
  }

  void tick()
  {
    hub.tick();
  }
}
