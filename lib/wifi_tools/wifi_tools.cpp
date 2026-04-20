#include "wifi_tools.h"

WiFi_Tools::WiFi_Tools() {}

WiFi_Tools wifi_tools;

void WiFi_Tools::enable_logging()
{
    _echo = true;
}

void WiFi_Tools::begin(const char *ssid, const char *pass)
{
    WiFi.setAutoReconnect(false);
    WiFi.onEvent(_event_handler);
    WiFi.persistent(false);
    WiFi.begin(ssid, pass);
    if (_echo)
        Serial.println("\n\twifi connecting...");
}

void WiFi_Tools::reconnect()
{
    if ((millis() - _reconnect_timer > RECONNECT_INTERVAL) && _should_reconnect)
    {
        if (_echo)
            Serial.println("\n\treconnecting...\n");
        WiFi.reconnect();
        _reconnect_timer = millis();
    }
}

void WiFi_Tools::_event_handler(WiFiEvent_t event, WiFiEventInfo_t info)
{
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED)
    {
        if (wifi_tools.is_connected && wifi_tools._echo)
            Serial.println("\twifi disconnected...\n");
        wifi_tools.is_connected = false;
        bool user_disconnected = info.wifi_sta_disconnected.reason == WIFI_REASON_ASSOC_LEAVE;
        wifi_tools._should_reconnect = !(user_disconnected || wifi_tools._first_disconnect);
        wifi_tools._first_disconnect = false;
    }
    
    if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP)
    {
        if (!wifi_tools.is_connected && wifi_tools._echo)
        {
            Serial.println("\twifi connected...\n");
            Serial.println(WiFi.localIP());
        }
        wifi_tools.is_connected = true;
    }
}