#ifndef ESP_CONNECTION_H
#define ESP_CONNECTION_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include "esp_camera.h"
#include "esp_http_server.h" // Add this line for HTTP server types
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "soc/soc.h"          //disable brownout problems
#include "soc/rtc_cntl_reg.h" //disable brownout problems
#include "driver/gpio.h"

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
    void initCamera();                                // Initialize the camera
    void startLocalServer();                          // Start the local HTTP server
    static esp_err_t streamHandler(httpd_req_t *req); // Handler for the /stream endpoint

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

    httpd_handle_t stream_httpd = NULL; // Handle for the HTTP server
};

#endif