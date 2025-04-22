#include <ESPConnection.h>
#include "esp_camera.h"
#include "secrets.h"

ESPConnection espConn(wifi1_ssid, wifi1_pass, wifi2_ssid, wifi2_pass);

// Camera pin configuration for AI-Thinker ESP32-CAM
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#define LED_GPIO_NUM 4

// Function to initialize the camera
void startCamera()
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;

  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound())
  {
    config.frame_size = FRAMESIZE_HVGA; // Set resolution to 480x320 (HVGA)
    config.jpeg_quality = 12;
    config.fb_count = 2;
  }
  else
  {
    config.frame_size = FRAMESIZE_HVGA; // Lower resolution without PSRAM
    config.jpeg_quality = 15;
    config.fb_count = 1;
  }

  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  Serial.println("✅ Camera initialized successfully");

  // Optional: Adjust sensor settings
  sensor_t *s = esp_camera_sensor_get();
  s->set_vflip(s, 0);       // Flip vertically
  s->set_hmirror(s, 0);     // Flip horizontally
  s->set_brightness(s, 1);  // Increase brightness slightly
  s->set_saturation(s, -2); // Reduce saturation slightly
}

// HTTP server setup
WiFiServer server(80);

void startWebServer()
{
  server.begin();
  Serial.println("✅ HTTP server started on port 80");
}

void handleClient()
{
  WiFiClient client = server.available();
  if (!client)
  {
    return;
  }

  Serial.println("New client connected");

  // Read the HTTP request
  String request = "";
  while (client.connected())
  {
    if (client.available())
    {
      char c = client.read();
      request += c;
      if (request.endsWith("\r\n\r\n"))
      {
        break;
      }
    }
  }

  // Check if the request is for the video stream
  if (request.indexOf("/stream") != -1)
  {
    Serial.println("Streaming video...");
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
    client.println();

    while (client.connected())
    {
      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb)
      {
        Serial.println("Camera capture failed");
        continue;
      }

      // Send the frame
      client.print("--frame\r\n");
      client.print("Content-Type: image/jpeg\r\n\r\n");
      client.write(fb->buf, fb->len);
      client.print("\r\n");
      esp_camera_fb_return(fb);

      // Break if the client disconnects
      if (!client.connected())
      {
        Serial.println("Client disconnected from stream");
        break;
      }

      // Add a small delay to avoid overwhelming the client
      delay(100);
    }
  }
  else
  {
    // Serve the default HTML page
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println();
    client.println("<html><head><title>ESP32-CAM Stream</title></head>");
    client.println("<body><h1>ESP32-CAM Stream</h1>");
    client.println("<img src=\"/stream\" width=\"640\" height=\"480\" /></body></html>");
  }

  client.stop();
  Serial.println("Client disconnected");
}

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);

  // Connect to Wi-Fi
  espConn.begin();

  // Initialize the camera
  startCamera();

  // Start the HTTP server
  startWebServer();
}

void loop()
{
  // Handle Wi-Fi connection and UDP messages
  espConn.loop();

  // Handle HTTP clients
  handleClient();
}