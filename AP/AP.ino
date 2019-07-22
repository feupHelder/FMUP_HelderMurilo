#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <WiFiAP.h>
#include "index.h"

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




// Set these to your desired credentials.
const char *ssid = "teste";
const char *password = "1234567890";

hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;
volatile bool led = false;

//periodic task with top priority
void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  digitalWrite(LED_BUILTIN, led);
  led = !led;
  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
}

void handleRoot() {
  String s = MAIN_page; //Read HTML contents
  server.send(200, "text/html", s); //Send web page
}


void handleForm() {
  String V = server.arg("Voltage");

  Serial.print("Voltage:");
  Serial.println(V);

  String s = MAIN_page; //Read HTML contents
  server.send(200, "text/html", s); //Send web page
}

void handleSensor() {
//  int a = analogRead(A0);
//  String adcValue = String(a);
  server.send(200, "text/plane", String((temprature_sens_read() - 32) / 1.8)); //Send ADC value only to client ajax request
}


void setup() {
  timerSemaphore = xSemaphoreCreateBinary();

  // Use 1st timer of 4 (counted from zero).
  // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
  // info).
  timer = timerBegin(0, 80, true);

  // Attach onTimer function to our timer.
  timerAttachInterrupt(timer, &onTimer, true);

  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter)
  timerAlarmWrite(timer, 500, true);

  // Start an alarm
  timerAlarmEnable(timer);

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  // You can remove the password parameter if you want the AP to be open.
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  server.on("/", handleRoot);      //Which routine to handle at root location
  server.on("/action_page", handleForm); //form action is handled here
  server.on("/readSensor", handleSensor);//To get update of ADC Value only

  server.begin();

  Serial.println("Server started");
}

void loop() {
  server.handleClient();
}
