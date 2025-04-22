#include "ESPConnection.h"
#include "secrets.h"

ESPConnection espConn(wifi1_ssid, wifi1_pass, wifi2_ssid, wifi2_pass);

void setup()
{
  espConn.begin();
}

void loop()
{
  espConn.loop();
}
