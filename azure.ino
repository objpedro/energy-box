#include "iot_configs.h"

// Credenciais Wifi
const char* ssid = "DJ_GILSON_MORALLES";
const char* password ="THARO215";
//const char* ssid = "esp32";
//const char* passord ="brasileiro";

//Porta dos leds
const int ledPin1 = 13;
const int ledPin2 = 12;

//Pino analógico conectado ao SCT-013
const int pinSCT = 32; 

//Tensão da residencia
const int tensao = 220;

void setup()
{
  startDisplay();
  logo();
  connectToWiFi();
  connectNTP();
  connectToSQL();
  connectSCT013();
}

void loop()
{
  for (int i = 0; i < 9; i++) {
    dataAcc();
    SCT013Print();
  }
  TimePrint();
  dataAcc();
  checkMinutes();
  resetESP32();
}