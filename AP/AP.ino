#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <WiFiAP.h>
#include "index.h"
#include <SPI.h>

#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();

#define LED_BUILTIN 2   // Set the GPIO pin where you connected your test LED or comment this line out if your dev board has a built-in LED


WebServer server(80);


static const int spiClk = 40000000; // 1 MHz

//uninitalised pointers to SPI objects
SPIClass * vspi = NULL;

// Set these to your desired credentials.
const char *ssid = "teste";
const char *password = "1234567890";

hw_timer_t * timerStim = NULL;
hw_timer_t * timerCharge = NULL;
hw_timer_t * timer = NULL;
hw_timer_t * timerRead_ = NULL;

volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;

volatile uint32_t CicleTimeCounter = 0;
volatile uint32_t StimulusTimeCounter = 0;

volatile uint32_t CicleTime = 10000;
volatile uint32_t StimulusTime = 100;
volatile uint32_t Amplitude = 255;
volatile bool criticalRegion= false;

volatile uint8_t  data[1000];
uint8_t n=1000;

volatile bool cicleDone = false;
volatile bool stimFlag = false;
volatile bool led = false;

//periodic task - T=100us
void IRAM_ATTR timerFunc() {
  CicleTimeCounter++;

  if (CicleTimeCounter == CicleTime ) {
    CicleTimeCounter = 0;
    stimFlag = true;
  }
  if (stimFlag) StimulusTimeCounter++;
  if (StimulusTimeCounter == StimulusTime ) {
    StimulusTimeCounter = 0;
    stimFlag = false;
  }
}

void IRAM_ATTR Stim() {
  if (stimFlag) {
    digitalWrite(2, HIGH);
  }
  else {
    digitalWrite(2, LOW);
  }
}
//Parar de estimular quando se muda a tensão de estimulo?
void IRAM_ATTR charge() {
  SPItoDAC( 1, Amplitude, 1);
}

void IRAM_ATTR read() {
  for(int i=0;i<n-1;i++)
    {
        data[i]=data[i+1];
    }
  data[n-1] = temprature_sens_read();
}

void handleRoot() {
  String s = MAIN_page; //Read HTML contents
  server.send(200, "text/html", s); //Send web page
}


void handleForm() {
  Amplitude = map(server.arg("Voltage").toDouble(), 0, 5, 0, 255);
  CicleTime = server.arg("Cicle Time").toDouble();
  StimulusTime = server.arg("Stimulus Time").toDouble();


  String s = MAIN_page; //Read HTML contents
  server.send(200, "text/html", s); //Send web page
}

void handleSensor() {
  //  int a = analogRead(A0);
  //  String adcValue = String(a);
  String send;
  for(int i=90; i<n; i++){
    send=send+data[i]+"; ";
  }
  server.send(200, "text/plane", send); //Send ADC value only to client ajax request
}

void NivelTensao (char modo_volts, char volts_ext) {

  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE1));
  digitalWrite(5, LOW); //pull SS slow to prep other end for transfer
  vspi->transfer(modo_volts);
  vspi->transfer(volts_ext);
  digitalWrite(5, HIGH); //pull ss high to signify end of data transfer
  vspi->endTransaction();
}

void SPItoDAC(int polaridade, int tensao, int pares) {
  int aux = 0x0;
  int valor = 0x0;

  for (int i = 0; i < tensao; i++)
  {
    aux++;
  }
  NivelTensao (highByte(0x9000), lowByte(0x9000));

  // // Serial.print(aux);
  // Serial.print(" ");
  if (pares == 1 && polaridade == 1) {
    valor = ( 0 << 12 | aux << 4 | 0 );
    NivelTensao (highByte(valor), lowByte(valor));
    // Serial.print(highByte(valor));
    // Serial.print(" ");
    // Serial.print(lowByte(valor));
    // Serial.print(" ");
    valor = 0x0;

    valor = ( 0xF << 12 | 0 << 8 | 0x10 );
    NivelTensao (highByte(valor), lowByte(valor));

    // Serial.print(highByte(valor));
    // Serial.print(" ");
    // Serial.print(lowByte(valor));
    // Serial.print(" ");
  }

  if (pares == 2 && polaridade == 1) {
    valor = ( 0x1 << 12 | aux << 4 | 0 );
    NivelTensao (highByte(valor), lowByte(valor));

    // Serial.print(highByte(valor));
    // Serial.print(" ");
    // Serial.print(lowByte(valor));
    // Serial.print(" "); valor = 0x0;

    valor = ( 0xF << 12 | 0 << 8 | 0x20 );
    NivelTensao (highByte(valor), lowByte(valor));
    // Serial.print(highByte(valor));
    // Serial.print(" ");
    // Serial.print(lowByte(valor));
    // Serial.print(" ");
  }

  if (pares == 3 && polaridade == 1) {
    valor = ( 0x2 << 12 | aux << 4 | 0 );
    NivelTensao (highByte(valor), lowByte(valor));
    // Serial.print(highByte(valor));
    // Serial.print(" ");
    // Serial.print(lowByte(valor));
    // Serial.print(" ");
    valor = 0x0;

    valor = ( 0xF << 12 | 0 << 8 | 0x40 );
    NivelTensao (highByte(valor), lowByte(valor));
    // Serial.print(highByte(valor));
    // Serial.print(" ");
    // Serial.print(lowByte(valor));
    // Serial.print(" ");
  }
  if (pares == 4 && polaridade == 1) {
    valor = ( 0x3 << 12 | aux << 4 | 0 );
    NivelTensao (highByte(valor), lowByte(valor));
    // Serial.print(highByte(valor));
    // Serial.print(" ");
    // Serial.print(lowByte(valor));
    // Serial.print(" ");
    valor = 0x0;

    valor = ( 0xF << 12 | 0 << 8 | 0x80 );
    NivelTensao (highByte(valor), lowByte(valor));
    // Serial.print(highByte(valor));
    // Serial.print(" ");
    // Serial.print(lowByte(valor));
    // Serial.print(" ");
  }
  if (pares == 1 && polaridade == 2) {
    valor = ( 0x4 << 12 | aux << 4 | 0x0 );
    NivelTensao (highByte(valor), lowByte(valor));
    // Serial.print(highByte(valor));
    // Serial.print(" ");
    // Serial.print(lowByte(valor));
    // Serial.print(" ");
    valor = 0x0;

    valor = (0xF << 12 | 0 << 8 | 0x1 );
    NivelTensao (highByte(valor), lowByte(valor));
    // Serial.print(highByte(valor));
    // Serial.print(" ");
    // Serial.print(lowByte(valor));
    // Serial.print(" ");
  }

  if (pares == 2 && polaridade == 2) {
    valor = ( 0x5 << 12 | aux << 4 | 0 );
    NivelTensao (highByte(valor), lowByte(valor));
    // Serial.print(highByte(valor));
    // Serial.print(" ");
    // Serial.print(lowByte(valor));
    // Serial.print(" ");
    valor = 0x0;

    valor = ( 0xF << 12 | 0 << 8 | 0x2 );
    NivelTensao (highByte(valor), lowByte(valor));
    // Serial.print(highByte(valor));
    // Serial.print(" ");
    // Serial.print(lowByte(valor));
    // Serial.print(" ");
  }
  if (pares == 3 && polaridade == 2) {
    valor = ( 0x6 << 12 | aux << 4 | 0 );
    NivelTensao (highByte(valor), lowByte(valor));
    // Serial.print(highByte(valor));
    // Serial.print(" ");
    // Serial.print(lowByte(valor));
    // Serial.print(" ");
    valor = 0x0;

    valor = ( 0xF << 12 | 0 << 8 | 0x4 );
    NivelTensao (highByte(valor), lowByte(valor));
    // Serial.print(highByte(valor));
    // Serial.print(" ");
    // Serial.print(lowByte(valor));
    // Serial.print(" ");
  }
  if (pares == 4 && polaridade == 2) {
    valor = ( 0x7 << 12 | aux << 4 | 0 );
    NivelTensao (highByte(valor), lowByte(valor));

    // Serial.print(highByte(valor));
    // Serial.print(" ");
    // Serial.print(lowByte(valor));
    // Serial.print(" ");
    valor = 0x0;

    valor = ( 0xF << 12 | 0 << 8 | 0x8 );
    NivelTensao (highByte(valor), lowByte(valor));
    // Serial.print(highByte(valor));
    // Serial.print(" ");
    // Serial.print(lowByte(valor));
    // Serial.print(" ");
  }
}



void setup() {
  vspi = new SPIClass(VSPI);
  vspi->begin();
  pinMode(5, O UTPUT); //VSPI SS

  //timerSemaphore = xSemaphoreCreateBinary();

  // Use 1st timer of 4 (counted from zero).
  // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
  // info).
  timerStim = timerBegin(0, 80, true);
  timerCharge = timerBegin(1, 80, true);
  timer = timerBegin(2, 80, true);
  timerRead_ = timerBegin(3, 80, true);

  // Attach onTimer function to our timer.
  timerAttachInterrupt(timerStim, &Stim, true);
  timerAttachInterrupt(timerCharge, &charge, true);
  timerAttachInterrupt(timer, &timerFunc, true);
  timerAttachInterrupt(timerRead_, &read, true);

  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter)
  timerAlarmWrite(timerStim, 50, true);
  timerAlarmWrite(timerCharge, 1000000, true); ///_________________________________
  timerAlarmWrite(timer, 100, true);
  timerAlarmWrite(timerRead_, 100000, true);

  // Start an alarm
  timerAlarmEnable(timerStim);
  timerAlarmEnable(timerCharge);
  timerAlarmEnable(timer);
  timerAlarmEnable(timerRead_);

  pinMode(2, OUTPUT);
  pinMode(22, OUTPUT);

  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  // You can remove the password parameter if you want the AP to be open.
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  // Serial.print("AP IP address: ");
  Serial.println(myIP);

  server.on("/", handleRoot);      //Which routine to handle at root location
  server.on("/action_page", handleForm); //form action is handled here
  server.on("/readData", handleSensor);//To get update of ADC Value only

  server.begin();

  Serial.println("Server started");
}

void loop() {
  server.handleClient();
}
