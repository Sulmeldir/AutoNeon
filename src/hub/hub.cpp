
#include <GyverHub.h>

GyverHub hub("MyDevices3", "ESP8266", ""); // имя сети, имя устройства, иконка
extern int TypeAlg;
extern uint8_t Col;
extern String AlgStr;
extern int Vsp_INTERVAL;
int NLeds;

namespace HUB
{

  // билдер
  void build(gh::Builder &b)
  {
    b.Input(&NLeds).size(2);
    b.Select_("sel", &TypeAlg).text(AlgStr);
    b.Color(&Col);
    b.Slider(&Vsp_INTERVAL).text("Интервал вспышки").range(1, 2000, 1);
  }

  void setup()
  {
    // настройка MQTT/Serial/Bluetooth..
    hub.mqtt.config("m3.wqtt.ru", 14635, "u_9ICRMS", "n5V6oZGA");
    // hub.mqtt.config("test.mosquitto.org", 8081);

    hub.onBuild(build); // подключаем билдер
    hub.begin();        // запускаем систему

    Serial.println("hub begin");
    hub.tick();
  }

  void tick()
  {
    hub.tick();
  }

}
