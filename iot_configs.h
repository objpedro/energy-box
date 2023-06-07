#include "EmonLib.h"
#include <WiFi.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <LiquidCrystal_I2C.h>
#define SECRET_USERDB "polfcm6z906rsg39"
#define SECRET_PASSDB "lpx5g8to7y1jvkyv"

// VARIAVEIS EXTERNA
extern const char* ssid;
extern const char* password;
extern const int ledPin1;
extern const int ledPin2;
extern const int pinSCT;
extern const int tensao;

// VARIAVEIS INTERNA
int idd = 1;                    // Variavel ID relacionada ao sensor
int potencia;                   // Variavel ultilizada para calculo da potencia
double Irms;                    // Variavel ultilizada para calculo da corrente
float accumulatedAmpere = 0.0;  // Variável para acumular os valores de corrente
float accumulatedWatts = 0.0;   // Variável para acumular os valores de potência
EnergyMonitor SCT013;           // Renomeando a função da biblioteca para o sensor

// INSTANCIANDO SERVIDOR ntpUDP PARA DATA E HORA
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "0.br.pool.ntp.org",-3 * 60 * 60);

// DECLARAÇÃO DE VARIÁVEIS PARA MySQL
WiFiClient client;
MySQL_Connection conn((Client *)&client);  
IPAddress server_addr(54, 88, 122, 109);  // IP of the MySQL *server* here
char user[] = SECRET_USERDB;              // MySQL user login username
char passdb[] = SECRET_PASSDB;

//Endereço LCD
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

//Inseri dados banco
char INSERT_SQL[] = "INSERT INTO irsxnvht0fovr06r.consumo(datetime, amperagem, watts, idd, tempo) VALUES ('%s', '%f', '%f', '%d', '%d')";
char query[200];

unsigned long lastTransmissionTime = 0;                    // Hora da ultima transmiição
const unsigned long transmissionInterval = 1 * 60 * 1000;  // Intervalo de 5 minutos em milissegundos
time_t lastSendTime = 0;                                   // Variável global para armazenar o tempo do último envio

//iniciar display 
void startDisplay() {

  lcd.init();                   
  lcd.backlight();
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  Serial.begin(9600);
  digitalWrite(ledPin1, LOW);
  digitalWrite(ledPin2, LOW);
}

// Conectar WIFI
void connectToWiFi() {
    digitalWrite(ledPin1, LOW);
    lcd.setCursor(0, 0);
    lcd.print("Conectado");
    lcd.setCursor(0, 1);
    lcd.print("  WIFI...");
    delay(5000);
    

    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(5000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Tentando");
      lcd.setCursor(0, 1);
      lcd.print("  CONECTAR....");
    }
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Conectado");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    digitalWrite(ledPin1, HIGH);
    delay(2000);
}

// Conectar ao servico NTP
void connectNTP() {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando");
  lcd.setCursor(0, 1);
  lcd.print("  NTP....");

  // Obtém a data e hora atual
  timeClient.begin();
  delay(5000);
  while (!timeClient.update()) {
    timeClient.forceUpdate();
    delay(5000);
  }

  // Obtém a hora atual
  time_t now = timeClient.getEpochTime();
  struct tm *timeinfo;
  timeinfo = localtime(&now);

  // Formata a data e hora como uma string
  char dateTimeStr[20];
  snprintf(dateTimeStr, sizeof(dateTimeStr), "%02d/%02d/%04d %02d:%02d:%02d",
           timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900,
           timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

  // Imprime a string no display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Data e Hora");
  lcd.setCursor(0, 1);
  lcd.print(dateTimeStr);
  delay(2000);
}

// Conectar ao servidor SQL
void connectToSQL() { 
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando");
  lcd.setCursor(0, 1);
  lcd.print("Banco SQL Server");
  delay(5000);



  // CONECTA NO MySQL
  while (!conn.connect(server_addr, 3306, user, passdb)) {
  
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Conexao");
    lcd.setCursor(0, 1);
    lcd.print("SQL falhou");
    delay(5000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Conectando BD");
    lcd.setCursor(0, 1);
    lcd.print("SQL novamente...");
    delay(5000);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Banco");
  lcd.setCursor(0, 1);
  lcd.print("Conectado");
  digitalWrite(ledPin2, HIGH);
  delay(5000);
  
}

// Enviar dados ao SQL
void sendDataToServer() {
  
  digitalWrite(ledPin2, LOW);
  time_t now = timeClient.getEpochTime();
  struct tm *timeinfo;
  timeinfo = localtime(&now);

  // Formata a data e hora como uma string
  char dateTimeStr[20];
  snprintf(dateTimeStr, sizeof(dateTimeStr), "%02d/%02d/%04d %02d:%02d:%02d",
           timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900,
           timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

  // Obter o tempo atual
  time_t currentSendTime = time(NULL);

  // Calcular a diferença de tempo em segundos desde o último envio
  double timeElapsed = difftime(currentSendTime, lastSendTime);

  // Atualizar o tempo do último envio com o tempo atual
  lastSendTime = currentSendTime;

  // Converter o valor do tempo do último envio para uma string
  char lastSendTimeStr[20];
  snprintf(lastSendTimeStr, sizeof(lastSendTimeStr), "%lu", lastSendTime);

  // ...

  // Converter o valor de timeElapsed para um inteiro (int)
  int timeElapsedInt = (int) timeElapsed;



  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  sprintf(query, INSERT_SQL,dateTimeStr,accumulatedAmpere,accumulatedWatts,idd,timeElapsedInt);
  cur_mem->execute(query);
  delete cur_mem;

    // Imprime a string no display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dados Gravados");
  lcd.setCursor(0, 1);
  lcd.print("  SQL Server");
  
  for (int i=0; i < 5; i++){
    digitalWrite(ledPin2, LOW); // Liga o LED
    delay(100); // Aguarda 500 milissegundos (0,5 segundo)
    digitalWrite(ledPin2, HIGH); // Desliga o LED
    delay(100);
  }
}

// Enviar dados ao SQL
void ResetData(){
  accumulatedAmpere = 0;
  accumulatedWatts = 0;
}

// Calibrar Sensor
void connectSCT013() {
  SCT013.current(pinSCT, 1.46640);
  double Irms = SCT013.calcIrms(1480);   // Calcula o valor da Corrente
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibrando");
  lcd.setCursor(0, 1);
  lcd.print("  Sensor....");
  delay(3000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sensor");
  lcd.setCursor(0, 1);
  lcd.print("Calibrado");
  delay(3000);
}

// Imprimir dados do sensor
void SCT013Print() {
    double Irms = SCT013.calcIrms(1480);
    if (Irms<0.02){Irms = 0; potencia = 0;}
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Corr. = ");
    lcd.print(Irms);
    lcd.print(" A");
  

    lcd.setCursor(0,1);
    lcd.print("Pot. = ");
    lcd.print(potencia);
    lcd.print(" W");
}

// Imprimir data e hora
void TimePrint() {

  // Obtém a hora atual
  time_t now = timeClient.getEpochTime();
  struct tm *timeinfo;
  timeinfo = localtime(&now);

  // Formata a data e hora como uma string
  char dateTimeStr[20];
  snprintf(dateTimeStr, sizeof(dateTimeStr), "%02d/%02d/%04d %02d:%02d:%02d",
           timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900,
           timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

  // Imprime a string no display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Data e Hora");
  lcd.setCursor(0, 1);
  lcd.print(dateTimeStr);
}

//Reconect WIFI
void reconectWIFI() {
  WiFi.disconnect();
  WiFi.begin(ssid, password);
}

// Imprimir WIFI Status
void printWiFiStatus() {
  // Obtém o status atual da conexão Wi-Fi
  wl_status_t status = WiFi.status();

  digitalWrite(ledPin1, HIGH);
  if (status == WL_CONNECTED) {
    Serial.println("Conectado ao Wi-Fi");
  } else if (status == WL_NO_SHIELD) {
    digitalWrite(ledPin1, LOW);
    Serial.println("Sem shield Wi-Fi disponível");
  } else if (status == WL_IDLE_STATUS) {
    digitalWrite(ledPin1, LOW);
    Serial.println("Status ocioso");
  } else if (status == WL_CONNECT_FAILED) {
    digitalWrite(ledPin1, LOW);
    Serial.println("Falha na conexão Wi-Fi");
    connectToWiFi();
  } else if (status == WL_DISCONNECTED) {
    digitalWrite(ledPin1, LOW);
    Serial.println("Desconectado do Wi-Fi");
  } else {
    digitalWrite(ledPin1, LOW);
    Serial.println("Status desconhecido");
  }
}

// Imprimir SQL Status
void printMySQLStatus() {
  if (conn.connected()) {
    
    Serial.println("Conectado ao servidor MySQL");
  } else {
    Serial.println("Falha na conexão com o servidor MySQL");
    digitalWrite(ledPin2, LOW);
  }
}

// Acumular dados
void dataAcc() {
    double Irms = SCT013.calcIrms(1480);
    if (Irms<0.02){Irms = 0; potencia = 0;}
    potencia = Irms * tensao;          // Calcula o valor da Potencia Instantanea    
    accumulatedAmpere += Irms;
    accumulatedWatts += potencia;
    delay(1000);

    Serial.print("\nCorrente acumulada: ");
    Serial.print(accumulatedAmpere, 2);  // Exibe a corrente com 2 casas decimais
    Serial.print(" A\n");

    Serial.print("Potência acumulada: ");
    Serial.print(accumulatedWatts, 2);   // Exibe a potência com 2 casas decimais
    Serial.print(" W\n");

    printWiFiStatus();
    printMySQLStatus();
}

//Iprimir logo
void logo() {
  for (int i = 0; i < 4; i++){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" ENERGY BOX");
  delay(250);
  lcd.clear();

  lcd.setCursor(0,1);
  lcd.print("   ENERGY BOX");
  delay(250);
  lcd.clear(); 
  lcd.clear();
  lcd.setCursor(0, 0);

  lcd.clear();
  lcd.print("    ENERGY BOX");
  delay(250);
  lcd.clear();

  lcd.setCursor(0,1);
  lcd.print("     ENERGY BOX");
  delay(250);
  lcd.clear(); 
  }
}

// Verificar o ultimo envio
void checkMinutes() {
  unsigned long currentMillis = millis(); // Verifica se o tempo decorrido desde a última transmissão é maior ou igual ao intervalo desejado
  if (currentMillis - lastTransmissionTime >= transmissionInterval) {

    if (WiFi.status() == WL_CONNECTED) {

      if (conn.connected()) {
        digitalWrite(ledPin2, HIGH);
        sendDataToServer();
        ResetData();
        Serial.print("\nDADOS ENVIADOS COM SUCESSO");
      }
      else{
        digitalWrite(ledPin2, LOW);
        conn.connect(server_addr, 3306, user, passdb);
      }
    }
    else{
      digitalWrite(ledPin1, LOW);
      reconectWIFI();
    }
    lastTransmissionTime = currentMillis;
  }
}

// Reset Esp32 porta seria
void resetESP32() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    if (command == "reset") {
      ESP.restart();
    }
  }
}


