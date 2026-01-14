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
    void attemptOTAUpdate();
    
private:
    WiFiServer wifiServer;
    WiFiServer wifiOBDII;
    WiFiClient wifiClient;
    WiFiUDP wifiUDPServer;
    uint32_t lastBroadcast;
};
