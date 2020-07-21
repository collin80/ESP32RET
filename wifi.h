#pragma once
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

class WiFiManager
{
public:
    WiFiManager();
    void setup();
    void loop();
    void sendBufferedData();
private:
    WiFiServer wifiServer;
    WiFiUDP wifiUDPServer;
    uint32_t lastBroadcast;
};
