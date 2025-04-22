#include "ESPConnection.h"
#include <esp_http_server.h>

ESPConnection::ESPConnection(const char *ssid1, const char *pass1, const char *ssid2, const char *pass2)
    : _ssid1(ssid1), _pass1(pass1), _ssid2(ssid2), _pass2(pass2) {}

void ESPConnection::begin()
{
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    connectToWiFi();
    initCamera();       // Initialize the camera
    startLocalServer(); // Start the local HTTP server

    udp.begin(udpListenPort);
    Serial.println("Listening for server broadcast on UDP...");
}

void ESPConnection::connectToWiFi()
{
    Serial.println("Connecting to Wi-Fi...");

    WiFi.begin(_ssid1, _pass1);
    for (int i = 0; i < 20; ++i)
    {
        if (WiFi.status() == WL_CONNECTED)
            break;
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("\nTrying second Wi-Fi...");
        WiFi.begin(_ssid2, _pass2);
        for (int i = 0; i < 20; ++i)
        {
            if (WiFi.status() == WL_CONNECTED)
                break;
            delay(500);
            Serial.print(".");
        }
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("\n❌ Failed to connect. Restarting...");
        ESP.restart();
    }

    Serial.println("\n✅ Connected to Wi-Fi");
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
}

void ESPConnection::blinkLED(int times)
{
    for (int i = 0; i < times; i++)
    {
        digitalWrite(LED_PIN, HIGH);
        delay(300);
        digitalWrite(LED_PIN, LOW);
        delay(300);
    }
}

void ESPConnection::sendConnectedMessage()
{
    if (!messageSent && serverFound && serverIP.length() > 0)
    {
        String msg = "ESP-CAM Connected";
        udp.beginPacket(serverIP.c_str(), udpSendPort);
        udp.write((uint8_t *)msg.c_str(), msg.length());
        udp.endPacket();
        messageSent = true;
        Serial.println("[ESP32-CAM] Sent connected message to server.");
    }
}

void ESPConnection::initCamera()
{
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = 5;
    config.pin_d1 = 18;
    config.pin_d2 = 19;
    config.pin_d3 = 21;
    config.pin_d4 = 36;
    config.pin_d5 = 39;
    config.pin_d6 = 34;
    config.pin_d7 = 35;
    config.pin_xclk = 0;
    config.pin_pclk = 22;
    config.pin_vsync = 25;
    config.pin_href = 23;
    config.pin_sccb_sda = 26;
    config.pin_sccb_scl = 27;
    config.pin_pwdn = 32;
    config.pin_reset = -1;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    if (psramFound())
    {
        config.frame_size = FRAMESIZE_UXGA; // Larger frame size
        config.jpeg_quality = 10;           // Higher quality
        config.fb_count = 2;
    }
    else
    {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    Serial.println("Camera initialized successfully.");
}

void ESPConnection::startLocalServer()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80; // Use port 80 for the HTTP server

    if (httpd_start(&stream_httpd, &config) == ESP_OK)
    {
        httpd_uri_t stream_uri = {
            .uri = "/stream",
            .method = HTTP_GET,
            .handler = streamHandler,
            .user_ctx = NULL};
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        Serial.println("Local HTTP server started at /stream");
    }
    else
    {
        Serial.println("Failed to start local HTTP server");
    }
}

esp_err_t ESPConnection::streamHandler(httpd_req_t *req)
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb)
    {
        Serial.println("Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
    String response = "--frame\r\nContent-Type: image/jpeg\r\n\r\n";
    httpd_resp_send_chunk(req, response.c_str(), response.length());
    httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);

    esp_camera_fb_return(fb);
    return ESP_OK;
}

void ESPConnection::loop()
{
    if (serverFound)
    {
        sendConnectedMessage();
        return;
    }

    char incomingPacket[255];
    int packetSize = udp.parsePacket();
    if (packetSize)
    {
        int len = udp.read(incomingPacket, 255);
        if (len > 0)
            incomingPacket[len] = '\0';

        Serial.print("\n📡 Received UDP: ");
        Serial.println(incomingPacket);

        if (String(incomingPacket).startsWith("SERVER_IP:"))
        {
            serverIP = String(incomingPacket).substring(10);
            serverFound = true;

            Serial.print("✅ Server IP: ");
            Serial.println(serverIP);

            blinkLED(3);
            digitalWrite(LED_PIN, HIGH); // Keep LED on
        }
    }
}