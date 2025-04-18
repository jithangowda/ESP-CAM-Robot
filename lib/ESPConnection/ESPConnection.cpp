#include "ESPConnection.h"

ESPConnection::ESPConnection(const char *ssid1, const char *pass1, const char *ssid2, const char *pass2)
    : _ssid1(ssid1), _pass1(pass1), _ssid2(ssid2), _pass2(pass2) {}

void ESPConnection::begin()
{
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    connectToWiFi();

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
