#pragma once

#include <WiFi.h>

#define RECONNECT_INTERVAL 10000


class WiFi_Tools {

    public:
        WiFi_Tools();
		void enable_logging();
		void begin(const char *, const char *);
		void reconnect();
        bool is_connected = false;

    private:
		bool _should_reconnect = true;
		bool _first_disconnect = true;
		bool _echo = true;
        unsigned long _reconnect_timer;
		static void _event_handler(WiFiEvent_t, WiFiEventInfo_t);
		
};

extern WiFi_Tools wifi_tools;