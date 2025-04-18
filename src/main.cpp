#include "ESPConnection.h"

const char *wifi1_ssid = "Airtel_LOL125";
const char *wifi1_pass = "Trynd@714";
const char *wifi2_ssid = "Minion";
const char *wifi2_pass = "123bakra123";

ESPConnection espConn(wifi1_ssid, wifi1_pass, wifi2_ssid, wifi2_pass);

void setup()
{
  espConn.begin();
}

void loop()
{
  espConn.loop();
}
