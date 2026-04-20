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
  namespace
  {
    String firmwareVersion()
    {
      return String(F(PROJECT_GH_REPO "@")) + F(PROJECT_FW_VERSION);
    }

    String projectJsonUrl()
    {
      return String(F("https://raw.githubusercontent.com/")) + F(PROJECT_GH_REPO) + F("/main/project.json");
    }

    String releaseAssetUrl()
    {
      return String(F("https://github.com/")) + F(PROJECT_GH_REPO) + F("/releases/latest/download/") + F(PROJECT_RELEASE_ASSET);
    }
  }

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
    hub.setVersion(firmwareVersion());
    // hub.mqtt.config("test.mosquitto.org", 8081);

    hub.onBuild(build);
    hub.begin();

    Serial.println("[OTA] GyverHub OTA metadata");
    Serial.println("[OTA] Firmware: " + firmwareVersion());
    Serial.println("[OTA] project.json: " + projectJsonUrl());
    Serial.println("[OTA] release bin: " + releaseAssetUrl());
    Serial.println("hub begin");
    hub.tick();
  }

  void tick()
  {
    hub.tick();
  }
}
