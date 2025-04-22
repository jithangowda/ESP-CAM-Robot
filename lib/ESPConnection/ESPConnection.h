#ifndef ESP_CONNECTION_H
#define ESP_CONNECTION_H

#include <WiFi.h>
#include <WiFiUdp.h>

class ESPConnection
{
public:
    ESPConnection(const char *ssid1, const char *pass1, const char *ssid2, const char *pass2);
    void begin();
    void loop();

private:
    void connectToWiFi();
    void blinkLED(int times);
    void sendConnectedMessage();

    const char *_ssid1;
    const char *_pass1;
    const char *_ssid2;
    const char *_pass2;

    WiFiUDP udp;
    String serverIP;
    bool serverFound = false;
    bool messageSent = false;

    static constexpr int LED_PIN = 4;
    static constexpr int udpListenPort = 4210;
    static constexpr int udpSendPort = 4211;
};

#endif