#include <Arduino.h>
#include "wifi_tools.h"
#include <Wire.h>
#include <4x14LED.h>
HT4x14LED led;

// #include "USB.h"
// #include "USBHIDConsumerControl.h"
// USBHIDConsumerControl ConsimetrControl;

namespace HUB
{
  void setup();
  void tick();
}

namespace NEON
{
  void setup();
  void tick();
}

void setup()
{
  Serial.begin(115200);
  Wire.begin();

  led.init(1);
  led.brig(10);
  led.blink(1);
  led.clear();

  wifi_tools.begin("Keenetic-7599", "AHXa5vSR");
  delay(3000);
  HUB::setup();
  NEON::setup();

  // ConsimetrControl.begin();
  // USB.begin();
}

void loop()
{

  led.print(1234, 0, -1, -1, -1, -1);
  // delay(1000);

  NEON::tick();

  if (wifi_tools.is_connected)
  {
    HUB::tick();
  }
  else
  {
    wifi_tools.reconnect();
  }
}
