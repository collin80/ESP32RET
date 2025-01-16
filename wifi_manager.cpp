#include "config.h"
#include "wifi_manager.h"
#include "gvret_comm.h"
#include "SerialConsole.h"
#include <ESPmDNS.h>
#include <Update.h> 
#include <WiFi.h>
#include <FastLED.h>
#include "ELM327_Emulator.h"

extern CRGB leds[A5_NUM_LEDS];

static IPAddress broadcastAddr(255,255,255,255);

WiFiManager::WiFiManager()
{
    lastBroadcast = 0;
}

void WiFiManager::setup()
{
    if (settings.wifiMode == 1) //connect to an AP
    {        
        Serial.println("Attempting to connect to a WiFi AP.");
        WiFi.mode(WIFI_STA);
        WiFi.setSleep(true); //sleeping could cause delays
        WiFi.begin((const char *)settings.SSID, (const char *)settings.WPA2Key);

        WiFiEventId_t eventID = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) 
        {
           if (SysSettings.fancyLED)
           {
               leds[SysSettings.LED_CONNECTION_STATUS] = CRGB::Red;
               FastLED.show();
           }
           Serial.print("WiFi lost connection. Reason: ");
           Serial.println(info.wifi_sta_disconnected.reason);
           SysSettings.isWifiConnected = false;
           if ( (info.wifi_sta_disconnected.reason == 202) || (info.wifi_sta_disconnected.reason == 3)) 
           {
              Serial.println("Connection failed, rebooting to fix it.");
              esp_sleep_enable_timer_wakeup(10);
              esp_deep_sleep_start();
              delay(100);
           }
        }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    }
    if (settings.wifiMode == 2) //BE an AP
    {
        WiFi.mode(WIFI_AP);
        WiFi.setSleep(true);
        WiFi.softAP((const char *)settings.SSID, (const char *)settings.WPA2Key);
        if (SysSettings.fancyLED)
        {
            leds[SysSettings.LED_CONNECTION_STATUS] = CRGB::Green;
            FastLED.show();
        }
    }
}

void WiFiManager::loop()
{
    boolean needServerInit = false; 
    int i;    

    if (settings.wifiMode > 0)
    {
        if (!SysSettings.isWifiConnected)
        {
            if (WiFi.isConnected())
            {
                //WiFi.setSleep(false);
                Serial.print("Wifi now connected to SSID ");
                Serial.println((const char *)settings.SSID);
                Serial.print("IP address: ");
                Serial.println(WiFi.localIP());
                Serial.print("RSSI: ");
                Serial.println(WiFi.RSSI());
                needServerInit = true;
                if (SysSettings.fancyLED)
                {
                    leds[SysSettings.LED_CONNECTION_STATUS] = CRGB::Green;
                    FastLED.show();
                }
            }
            if (settings.wifiMode == 2)
            {
                Serial.print("Wifi setup as SSID ");
                Serial.println((const char *)settings.SSID);
                Serial.print("IP address: ");
                Serial.println(WiFi.softAPIP());
                needServerInit = true;
            }
            if (needServerInit)
            {
                SysSettings.isWifiConnected = true;
                //MDNS.begin wants the name we will register as without the .local on the end. That's added automatically.
                if (!MDNS.begin(deviceName)) Serial.println("Error setting up MDNS responder!");
                MDNS.addService("telnet", "tcp", 23);// Add service to MDNS-SD
                MDNS.addService("ELM327", "tcp", 1000);// Add service to MDNS-SD
                wifiServer.begin(23); //setup as a telnet server
                wifiServer.setNoDelay(true);
                Serial.println("TCP server started");
                wifiOBDII.begin(1000); //setup for wifi linked ELM327 emulation
                wifiOBDII.setNoDelay(true);
                ArduinoOTA.setPort(3232);
                ArduinoOTA.setHostname(deviceName);
                // No authentication by default
                //ArduinoOTA.setPassword("admin");
                
                ArduinoOTA
                   .onStart([]() {
                      String type;
                      if (SysSettings.fancyLED)
                      {
                          leds[SysSettings.LED_CONNECTION_STATUS] = CRGB::Purple;
                          FastLED.show();
                      }
                      if (ArduinoOTA.getCommand() == U_FLASH)
                         type = "sketch";
                      else // U_SPIFFS
                         type = "filesystem";

                      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                      Serial.println("Start updating " + type);
                   })
                   .onEnd([]() {
                      Serial.println("\nEnd");
                   })
                   .onProgress([](unsigned int progress, unsigned int total) {
                       Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
                   })
                   .onError([](ota_error_t error) {
                      Serial.printf("Error[%u]: ", error);
                      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
                      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
                      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
                      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
                      else if (error == OTA_END_ERROR) Serial.println("End Failed");
                   });
                   
                ArduinoOTA.begin();
            }
        }
        else
        {
            if (WiFi.isConnected() || settings.wifiMode == 2)
            {
                if (wifiServer.hasClient())
                {
                    for(i = 0; i < MAX_CLIENTS; i++)
                    {
                        if (!SysSettings.clientNodes[i] || !SysSettings.clientNodes[i].connected())
                        {
                            if (SysSettings.clientNodes[i]) SysSettings.clientNodes[i].stop();
                            SysSettings.clientNodes[i] = wifiServer.available();
                            if (!SysSettings.clientNodes[i]) Serial.println("Couldn't accept client connection!");
                            else 
                            {
                                Serial.print("New client: ");
                                Serial.print(i); Serial.print(' ');
                                Serial.println(SysSettings.clientNodes[i].remoteIP());
                                if (SysSettings.fancyLED)
                                {
                                    leds[SysSettings.LED_CONNECTION_STATUS] = CRGB::Blue;
                                    FastLED.show();
                                }
                            }
                        }
                    }
                    if (i >= MAX_CLIENTS) {
                        //no free/disconnected spot so reject
                        wifiServer.available().stop();
                    }
                }

                if (wifiOBDII.hasClient())
                {
                    for(i = 0; i < MAX_CLIENTS; i++)
                    {
                        if (!SysSettings.wifiOBDClients[i] || !SysSettings.wifiOBDClients[i].connected())
                        {
                            if (SysSettings.wifiOBDClients[i]) SysSettings.wifiOBDClients[i].stop();
                            SysSettings.wifiOBDClients[i] = wifiOBDII.available();
                            if (!SysSettings.wifiOBDClients[i]) Serial.println("Couldn't accept client connection!");
                            else 
                            {
                                Serial.print("New wifi ELM client: ");
                                Serial.print(i); Serial.print(' ');
                                Serial.println(SysSettings.wifiOBDClients[i].remoteIP());
                            }
                        }
                    }
                    if (i >= MAX_CLIENTS) {
                        //no free/disconnected spot so reject
                        wifiOBDII.available().stop();
                    }
                }

                //check clients for data
                for(i = 0; i < MAX_CLIENTS; i++)
                {
                    if (SysSettings.clientNodes[i] && SysSettings.clientNodes[i].connected())
                    {
                        if(SysSettings.clientNodes[i].available())
                        {
                            //get data from the telnet client and push it to input processing
                            while(SysSettings.clientNodes[i].available()) 
                            {
                                uint8_t inByt;
                                inByt = SysSettings.clientNodes[i].read();
                                SysSettings.isWifiActive = true;
                                //Serial.write(inByt); //echo to serial - just for debugging. Don't leave this on!
                                wifiGVRET.processIncomingByte(inByt);
                            }
                        }
                    }
                    else
                    {
                        if (SysSettings.clientNodes[i]) 
                        {
                            SysSettings.clientNodes[i].stop();
                            if (SysSettings.fancyLED)
                            {
                                leds[SysSettings.LED_CONNECTION_STATUS] = CRGB::Green;
                                FastLED.show();
                            }
                        }
                    }

                    if (SysSettings.wifiOBDClients[i] && SysSettings.wifiOBDClients[i].connected())
                    {
                        elmEmulator.setWiFiClient(&SysSettings.wifiOBDClients[i]);
                        /*if(SysSettings.wifiOBDClients[i].available())
                        {
                            //get data from the telnet client and push it to input processing
                            while(SysSettings.wifiOBDClients[i].available()) 
                            {
                                uint8_t inByt;
                                inByt = SysSettings.wifiOBDClients[i].read();
                                SysSettings.isWifiActive = true;
                                //wifiGVRET.processIncomingByte(inByt);
                            }
                        }*/
                    }
                    else
                    {
                        if (SysSettings.wifiOBDClients[i])
                        {
                            SysSettings.wifiOBDClients[i].stop();
                            elmEmulator.setWiFiClient(0);
                        }
                    }
                }                    
            }
            else 
            {
                if (settings.wifiMode == 1)
                {
                    Serial.println("WiFi disconnected. Bummer!");
                    SysSettings.isWifiConnected = false;
                    SysSettings.isWifiActive = false;
                    if (SysSettings.fancyLED)
                    {  
                        leds[SysSettings.LED_CONNECTION_STATUS] = CRGB::Red;
                        FastLED.show();
                    }
                }
            }
        }
    }

    if (SysSettings.isWifiConnected && ((micros() - lastBroadcast) > 1000000ul) ) //every second send out a broadcast ping
    {
        uint8_t buff[4] = {0x1C,0xEF,0xAC,0xED};
        lastBroadcast = micros();
        wifiUDPServer.beginPacket(broadcastAddr, 17222);
        wifiUDPServer.write(buff, 4);
        wifiUDPServer.endPacket();
    }

    ArduinoOTA.handle();
}

void WiFiManager::sendBufferedData()
{
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        size_t wifiLength = wifiGVRET.numAvailableBytes();
        uint8_t* buff = wifiGVRET.getBufferedBytes();
        if (SysSettings.clientNodes[i] && SysSettings.clientNodes[i].connected())
        {
            SysSettings.clientNodes[i].write(buff, wifiLength);
        }
    }
    wifiGVRET.clearBufferedBytes();
}

// Utility to extract header value from headers
String getHeaderValue(String header, String headerName)
{
  return header.substring(strlen(headerName.c_str()));
}

void onOTAProgress(uint32_t progress, size_t fullSize)
{
    static int OTAcount = 0;
    //esp_task_wdt_reset();
    if (OTAcount++ == 10)
    {
        Serial.println(progress);
        OTAcount = 0;
    }
    else 
    {
        Serial.print("...");
        Serial.print(progress);
    }
}

void WiFiManager::attemptOTAUpdate()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\n\n");
        Serial.print("*WIFI STATUS* SSID:");
        Serial.print(WiFi.SSID());
        Serial.print(" with IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.println("Attempting the OTA update from remote host");
    }
    else
    {
        Serial.println("\n\nIt appears there is no wireless connection. Cannot update.");
        return;
    }

    int contentLength = 0;
    bool isValidContentType = false;
    //TODO: figure out where github stores files in releases and/or how and where the images will be stored.
    int port = 80; // Non https. HTTPS would be 443 but that might not work.
    String host = String(otaHost);
    String bin = String(otaFilename);
    //esp_task_wdt_reset(); //in case watchdog was set, update it.
    Update.onProgress(onOTAProgress);

    Serial.println("Connecting to OTA server: " + host);

    if (wifiClient.connect(otaHost, port))
    {
        wifiClient.print(String("GET ") + bin + " HTTP/1.1\r\n" +
                     "Host: " + String(host) + "\r\n" +
                     "Cache-Control: no-cache\r\n" +
                     "Connection: close\r\n\r\n");  // Get the contents of the bin file

        unsigned long timeout = millis();
        while (wifiClient.available() == 0)
        {
            if (millis() - timeout > 5000)
            {
                Serial.println("Timeout trying to connect! Aborting!");
                wifiClient.stop();
                return;
            }
        }
    
        while (wifiClient.available())
        {
            String line = wifiClient.readStringUntil('\n');// read line till /n
            line.trim();// remove space, to check if the line is end of headers

            // if the the line is empty,this is end of headers break the while and feed the
            // remaining `client` to the Update.writeStream();

            if (!line.length()) {
                break;
            }

            // Check if the HTTP Response is 200 else break and Exit Update

            if (line.startsWith("HTTP/1.1"))
            {
                if (line.indexOf("200") < 0)
                {
                    Serial.println("FAIL...Got a non 200 status code from server. Exiting OTA Update.");
                    break;
                }
            }

            // extract headers here
            // Start with content length

            if (line.startsWith("Content-Length: "))
            {
                contentLength = atoi((getHeaderValue(line, "Content-Length: ")).c_str());
                Serial.println("              ...Server indicates " + String(contentLength) + " byte file size\n");
            }

            if (line.startsWith("Content-Type: "))
            {
                String contentType = getHeaderValue(line, "Content-Type: ");
                Serial.println("\n              ...Server indicates correct " + contentType + " payload.\n");
                if (contentType == "application/octet-stream")
                {
                    isValidContentType = true;
                }
            }
        } //end while client available
    }
    else 
    {
        // Connect to remote failed
        Serial.println("Connection to " + String(host) + " failed. Please check your setup!");
    }

    // Check what is the contentLength and if content type is `application/octet-stream`
    //Serial.println("File length: " + String(contentLength) + ", Valid Content Type flag:" + String(isValidContentType));

    // check contentLength and content type
    if (contentLength && isValidContentType) // Check if there is enough to OTA Update
    {
        bool canBegin = Update.begin(contentLength);
        if (canBegin)
        {
            Serial.println("There is sufficient space to update. Beginning update. \n");
            size_t written = Update.writeStream(wifiClient);

            if (written == contentLength)
            {
                Serial.println("\nWrote " + String(written) + " bytes to memory...");
            }
            else
            {
                Serial.println("\n********** FAILED - Wrote:" + String(written) + " of " + String(contentLength) + ". Try again later. ********\n\n" );
                return;
            }

            if (Update.end())
            {
                //  Serial.println("OTA file transfer completed!");
                if (Update.isFinished())
                {
                    Serial.println("Rebooting new firmware...\n");
                    ESP.restart();
                }
                else Serial.println("FAILED...update not finished? Something went wrong!");

            }
            else
            {
                Serial.println("Error Occurred. Error #: " + String(Update.getError()));
                return;
            }
        } //end if can begin
        else 
        {
            // not enough space to begin OTA
            // Understand the partitions and space availability

            Serial.println("Not enough space to begin OTA");
            wifiClient.flush();
        }
    } //End contentLength && isValidContentType
    else
    {
        Serial.println("There was no content in the response");
        wifiClient.flush();
    }
}
