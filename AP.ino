#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <WiFiAP.h>
#include "index.h"
#include <SPI.h>
#include <Arduino.h>
#include <Wire.h>
#include <SD.h>

//Multiplicador para transformar a escala de ms em 100us
#define MULT_MS 10

String nameFile = "/leituras.csv";

WebServer server(80);

TaskHandle_t Server;
TaskHandle_t PaceMaker;

static const int spiClk = 40000000; // 1 MHz

//uninitalised pointers to SPI objects
SPIClass * vspi = NULL;

// Set these to your desired credentials.
const char *ssid = "teste";
const char *password = "1234567890";

hw_timer_t * timer = NULL;


//Semaferos - Nao estao a ser usados
//volatile SemaphoreHandle_t timerSemaphore;
//portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

//volatile uint32_t isrCounter = 0;
//volatile uint32_t lastIsrAt = 0;

//Contadores dos tempos relacionados com o estimulo
volatile uint32_t CicleTimeCounter = 0;
volatile uint32_t StimulusTimeCounter = 0;
volatile uint32_t writingCounter = 0;

// O multiplicador só aplica quando é lido apartir da web
volatile uint32_t CicleTime = 1000; //10ms
volatile uint32_t StimulusTime = 100; //1ms
volatile uint32_t WrittingTime = 166; //16ms
volatile uint32_t Amplitude = 255;

volatile bool criticalRegion = false;

//Flags usadas para controlar os ciclos, leituras e escreitas
volatile bool cicleDone = false;
volatile bool stimFlag = false;
volatile bool led = false;
volatile bool writingFlag = true;
volatile bool Reading = false;


//periodic task - T=100us
void IRAM_ATTR timerFunc() {
  CicleTimeCounter++;
  writingCounter++;

  //Se passou um periodo e pode-se etimular
  if (CicleTimeCounter == CicleTime) {
    CicleTimeCounter = 0;
    stimFlag = true;
  }
  //cronometra o tempo de estimulo
  if (stimFlag) StimulusTimeCounter++;

  if (writingCounter == WrittingTime) {
    writingFlag = true;
    writingCounter = 0;
  }

  //desliga a ordem de estimular
  if (StimulusTimeCounter == StimulusTime) {
    StimulusTimeCounter = 0;
    stimFlag = false;
  }


}


//Esta função é ainda um rascunho
void read() {
  Reading = true;
  int data = analogRead(A0);
  File file = SD.open(nameFile, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for writing");
    Reading = false;
    return;
  }
  if (!file.println(data)) {
    Serial.println("failed to print");
  }
  file.close();
  Reading = false;
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
  if (CicleTimeTemp != 0) CicleTime = CicleTimeTemp * MULT_MS;
  if (StimulusTimeTemp != 0) StimulusTime = StimulusTimeTemp * MULT_MS;

  String s = MAIN_page;
  server.send(200, "text/html", s); //Manda a pagina principal
}

//Permite transferir o ficheiro .csv apartir do SD
void handleData() {
  Reading = true;
  File download = SD.open("/" + nameFile);
  if (download) {
    server.sendHeader("Content-Type", "text/text");
    server.sendHeader("Content-Disposition", "attachment; filename=" + nameFile);
    server.sendHeader("Connection", "close");
    server.streamFile(download, "application/octet-stream");
    download.close();
  }
  //Caso haja algum erro vai para a pagina principal simplesmente
  else {
    String s = MAIN_page; //Read HTML contents
    server.send(200, "text/html", s); //Send web page
  }
  Reading = false;
}



//_____________________
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
    valor = 0x0;

    valor = ( 0xF << 12 | 0 << 8 | 0x10 );
    NivelTensao (highByte(valor), lowByte(valor));
  }

  if (pares == 2 && polaridade == 1) {
    valor = ( 0x1 << 12 | aux << 4 | 0 );
    NivelTensao (highByte(valor), lowByte(valor));

    valor = ( 0xF << 12 | 0 << 8 | 0x20 );
    NivelTensao (highByte(valor), lowByte(valor));
  }

  if (pares == 3 && polaridade == 1) {
    valor = ( 0x2 << 12 | aux << 4 | 0 );
    NivelTensao (highByte(valor), lowByte(valor));

    valor = 0x0;

    valor = ( 0xF << 12 | 0 << 8 | 0x40 );
    NivelTensao (highByte(valor), lowByte(valor));

  }
  if (pares == 4 && polaridade == 1) {
    valor = ( 0x3 << 12 | aux << 4 | 0 );
    NivelTensao (highByte(valor), lowByte(valor));

    valor = 0x0;

    valor = ( 0xF << 12 | 0 << 8 | 0x80 );
    NivelTensao (highByte(valor), lowByte(valor));
  }
  if (pares == 1 && polaridade == 2) {
    valor = ( 0x4 << 12 | aux << 4 | 0x0 );
    NivelTensao (highByte(valor), lowByte(valor));
    valor = 0x0;

    valor = (0xF << 12 | 0 << 8 | 0x1 );
    NivelTensao (highByte(valor), lowByte(valor));
  }

  if (pares == 2 && polaridade == 2) {
    valor = ( 0x5 << 12 | aux << 4 | 0 );
    NivelTensao (highByte(valor), lowByte(valor));
    valor = 0x0;

    valor = ( 0xF << 12 | 0 << 8 | 0x2 );
    NivelTensao (highByte(valor), lowByte(valor));
  }
  if (pares == 3 && polaridade == 2) {
    valor = ( 0x6 << 12 | aux << 4 | 0 );
    NivelTensao (highByte(valor), lowByte(valor));

    valor = 0x0;

    valor = ( 0xF << 12 | 0 << 8 | 0x4 );
    NivelTensao (highByte(valor), lowByte(valor));

  }
  if (pares == 4 && polaridade == 2) {
    valor = ( 0x7 << 12 | aux << 4 | 0 );
    NivelTensao (highByte(valor), lowByte(valor));
    valor = 0x0;

    valor = ( 0xF << 12 | 0 << 8 | 0x8 );
    NivelTensao (highByte(valor), lowByte(valor));
  }
}



void setup() {
  Serial.begin(115200);

  if (!SD.begin()) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println();
  Serial.println("Configuring access point...");
  vspi = new SPIClass(VSPI);
  vspi->begin();
  pinMode(5, OUTPUT); //VSPI SS

  WiFi.softAP("teste", "1234567890");
  IPAddress myIP = WiFi.softAPIP();
  //wifi_set_sleep_type(MODEM_SLEEP_T);

  pinMode(2, OUTPUT);
  pinMode(22, OUTPUT);

  //que funcao vai controlar o pedido HTML
  server.on("/", handleRoot);      //Which routine to handle at root location
  server.on("/back", handleRoot);
  server.on("/action_page", handleForm);
  server.on("/goData_page", handleData);

  Wire.begin();

  //timerSemaphore = xSemaphoreCreateBinary();


  timer = timerBegin(2, 80, true);
  timerAttachInterrupt(timer, &timerFunc, true);
  //Aqui podemos alterar o periodo a que é chamada a interrupção
  timerAlarmWrite(timer, 100, true);
  timerAlarmEnable(timer);

  //Não é cencessário visto o loop estar no core 0
  
  //  xTaskCreatePinnedToCore(
  //    ServerCore,   /* Task function. */
  //    "ServerCore",     /* name of task. */
  //    10000,       /* Stack size of task */
  //    NULL,        /* parameter of the task */
  //    1,           /* priority of the task */
  //    &Server,      /* Task handle to keep track of created task */
  //    0);          /* pin task to core 0 */
  //  delay(500);


  //cria uma task que vai ser executada na função PaceMakerCore com perioridade 1 no core 1
  xTaskCreatePinnedToCore(
    PaceMakerCore,   /* Task function. */
    "PaceMaker",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &PaceMaker,      /* Task handle to keep track of created task */
    1);          /* pin task to core 1 */
  delay(500);

  server.begin();
  Serial.println("Server started");
}

//void ServerCore( void * pvParameters ) {
//  while (1) server.handleClient();
//}

void PaceMakerCore( void * pvParameters ) {
  while (1) {
    if (writingFlag && !Reading) {
      read();
      writingFlag = false;
    }
    //Mais coisas...

  }
}
//esta task está a correr no core 0
void loop() {
  server.handleClient();
}
