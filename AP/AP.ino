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


WebServer server(80);


static const int spiClk = 40000000; // 1 MHz

//uninitalised pointers to SPI objects
SPIClass * vspi = NULL;

// Set these to your desired credentials.
const char *ssid = "teste";
const char *password = "1234567890";

//Timers que vão premitir criar funções periodicas preentivas
hw_timer_t * timerStim = NULL;
hw_timer_t * timerCharge = NULL;
hw_timer_t * timer = NULL;
hw_timer_t * timerRead_ = NULL;

//Semaferos - Nao estao a ser usados
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

//volatile uint32_t isrCounter = 0;
//volatile uint32_t lastIsrAt = 0;

//Contadores dos tempos relacionados com o estimulo
volatile uint32_t CicleTimeCounter = 0;
volatile uint32_t StimulusTimeCounter = 0;

volatile uint32_t CicleTime = 10000;
volatile uint32_t StimulusTime = 100;
volatile uint32_t Amplitude = 255;
volatile bool criticalRegion = false;

volatile uint8_t  data[100];
uint8_t n = 100;

volatile bool cicleDone = false;
volatile bool stimFlag = false;
volatile bool led = false;



//periodic task - T=100us
void IRAM_ATTR timerFunc() {
  CicleTimeCounter++;

  //Se passou um periodo e pode-se etimular
  if (CicleTimeCounter == CicleTime ) {
    CicleTimeCounter = 0;
    stimFlag = true;
  }
  //cronometra o tempo de estimulo
  if (stimFlag) StimulusTimeCounter++;

  //desliga a ordem de estimular
  if (StimulusTimeCounter == StimulusTime ) {
    StimulusTimeCounter = 0;
    stimFlag = false;
  }
}
//Onde vai estar o controlo do estimulo
void IRAM_ATTR Stim() {
  if (stimFlag) {
    digitalWrite(2, HIGH);
  }
  else {
    digitalWrite(2, LOW);
  }
}

//Controlo do DAC
void IRAM_ATTR charge() {

}


//Esta função é ainda um rascunho
void IRAM_ATTR read() {
  for (int i = 0; i < n - 1; i++)
  {
    data[i] = data[i + 1];
  }
  data[n - 1] = analogRead(A0);
}


//Comunicacao com o PC-----------------------------

//Controlo da pagina principal -- index.h
void handleRoot() {
  String s = MAIN_page; //Read HTML contents
  server.send(200, "text/html", s); //Send web page
}

//Sempre que forem enviados novos dados esta funcao vai os buscar e atualiza os registos no ESP
void handleForm() {
  uint32_t CicleTimeTemp = 0;
  uint32_t StimulusTimeTemp = 0;
  uint32_t AmplitudeTemp = 0;

  AmplitudeTemp = map(server.arg("Voltage").toDouble(), 0, 5, 0, 255);
  CicleTimeTemp = server.arg("Cicle Time").toDouble();
  StimulusTimeTemp = server.arg("Stimulus Time").toDouble();

  if (AmplitudeTemp != 0) {
    Amplitude = AmplitudeTemp;
    SPItoDAC( 1, Amplitude, 1);
  }
  if (CicleTimeTemp != 0) CicleTime = CicleTimeTemp;
  if (StimulusTimeTemp != 0) StimulusTime = StimulusTimeTemp;

  String s = MAIN_page; 
  server.send(200, "text/html", s); //Manda a pagina principal
}

//envia uma tabela simples. Disponibiliza um botao que permite converter a tabela num ficheiro do formato CVS
void handleData() {
  String send = "<!DOCTYPE html><html><body> <font size=\"1\"> <table>";
  for (int i = 0; i < n; i++) {
    send = send + "<tr><th>" + String(i + 1) + "</th><th>" + data[i] + "</th></tr>";
  }
  send = send + "</table><button>Export HTML table to CSV file</button><p></p></font><form action=\"/back\"><button>Back</button></form>" + script + "</body></html>";
  server.send(200, "text/html", send); //Send ADC value only to client ajax request
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
  pinMode(5, OUTPUT); //VSPI SS

  //timerSemaphore = xSemaphoreCreateBinary();


  // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
  // info).

  //ESP possui 4 timers
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

  //que funcao vai controlar o pedido HTML
  server.on("/", handleRoot);      //Which routine to handle at root location
  server.on("/back", handleRoot);
  server.on("/action_page", handleForm); 
  server.on("/goData_page", handleData);

  server.begin();

  Serial.println("Server started");
}

void loop() {
  server.handleClient();
}
