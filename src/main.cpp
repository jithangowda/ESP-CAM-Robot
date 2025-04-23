#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include "esp_http_server.h"
#include "secrets.h"

WiFiUDP udp;
String serverIP;
bool serverFound = false;
bool messageSent = false;

static constexpr int LED_PIN = 4;
static constexpr int udpListenPort = 4210; // Port to listen for server broadcast
static constexpr int udpSendPort = 4211;   // Port to send messages to the server

httpd_handle_t stream_httpd = NULL;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32-CAM Stream</title>
</head>
<body>
  <h1>ESP32-CAM Video Stream</h1>
  <img src="/stream" />
</body>
</html>
)rawliteral";

static esp_err_t index_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, index_html, strlen(index_html));
}

static esp_err_t stream_handler(httpd_req_t *req)
{
  camera_fb_t *fb = NULL;
  struct timeval _timestamp;
  size_t _jpg_buf_len = 0;
  uint8_t *_jpg_buf = NULL;
  static int64_t last_frame = 0;

  if (!last_frame)
  {
    last_frame = esp_timer_get_time();
  }

  httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=123456789000000000000987654321");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  while (true)
  {
    fb = esp_camera_fb_get();
    if (!fb)
    {
      Serial.println("Camera capture failed");
      break;
    }

    _timestamp.tv_sec = fb->timestamp.tv_sec;
    _timestamp.tv_usec = fb->timestamp.tv_usec;

    if (fb->format != PIXFORMAT_JPEG)
    {
      bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
      esp_camera_fb_return(fb);
      fb = NULL;
      if (!jpeg_converted)
      {
        Serial.println("JPEG compression failed");
        break;
      }
    }
    else
    {
      _jpg_buf_len = fb->len;
      _jpg_buf = fb->buf;
    }

    httpd_resp_send_chunk(req, "--123456789000000000000987654321\r\n", -1);
    httpd_resp_send_chunk(req, "Content-Type: image/jpeg\r\nContent-Length: ", -1);
    httpd_resp_send_chunk(req, String(_jpg_buf_len).c_str(), -1);
    httpd_resp_send_chunk(req, "\r\nX-Timestamp: ", -1);
    httpd_resp_send_chunk(req, String(_timestamp.tv_sec).c_str(), -1);
    httpd_resp_send_chunk(req, ".", -1);
    httpd_resp_send_chunk(req, String(_timestamp.tv_usec).c_str(), -1);
    httpd_resp_send_chunk(req, "\r\n\r\n", -1);
    httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);

    if (fb)
    {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    }
    else if (_jpg_buf)
    {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }

    int64_t fr_end = esp_timer_get_time();
    int64_t frame_time = fr_end - last_frame;
    last_frame = fr_end;
    frame_time /= 1000;
    Serial.printf("MJPG: %uB %ums (%.1ffps)\n", (uint32_t)(_jpg_buf_len), (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);
  }

  return ESP_OK;
}

void startCameraServer()
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 2;

  httpd_uri_t index_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = index_handler,
      .user_ctx = NULL};

  httpd_uri_t stream_uri = {
      .uri = "/stream",
      .method = HTTP_GET,
      .handler = stream_handler,
      .user_ctx = NULL};

  log_i("Starting web server on port: '%d'", config.server_port);
  if (httpd_start(&stream_httpd, &config) == ESP_OK)
  {
    httpd_register_uri_handler(stream_httpd, &index_uri);
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

void connectToWiFi()
{
  Serial.println("Connecting to Wi-Fi...");

  WiFi.begin(ssid1, password1);
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
    WiFi.begin(ssid2, password2);
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

void blinkLED(int times)
{
  for (int i = 0; i < times; i++)
  {
    digitalWrite(LED_PIN, HIGH);
    delay(300);
    digitalWrite(LED_PIN, LOW);
    delay(300);
  }
}

void sendConnectedMessage()
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

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Camera Configuration
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
  config.frame_size = FRAMESIZE_HVGA;   //  HVGA (480x320)
  config.pixel_format = PIXFORMAT_JPEG; // Use JPEG format
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 10;
  config.fb_count = 2;

  // Camera Initialization
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  s->set_vflip(s, 0);   // Disable vertical flip
  s->set_hmirror(s, 0); // Disable horizontal mirror

  connectToWiFi();

  blinkLED(3);

  digitalWrite(LED_PIN, LOW);

  udp.begin(udpListenPort);
  Serial.println("Listening for server broadcast on UDP...");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void loop()
{

  if (!serverFound)
  {
    char incomingPacket[255];
    int packetSize = udp.parsePacket();
    if (packetSize)
    {
      int len = udp.read(incomingPacket, 255);
      if (len > 0)
      {
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
        }
      }
    }
  }

  sendConnectedMessage();

  delay(100);
}