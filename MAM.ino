/*********************************************************************
MAM Air Monitor
m0ro
**********************************************************************/

// MCU: ESPRESSIF ESP32-WROOM-32D
// particulate sensor: PMS7003 
// CO2 sensor: MH-Z19B
// Humidity/Temperature sensor: HTU21
// Display: SPI oled display 128x64

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// fill with correct data
const char* ssid = "";
const char* password = "";

// code from https://github.com/jmstriegel/Plantower_PMS7003
#include "Plantower_PMS7003.h"
// code from https://github.com/WifWaf/MH-Z19
#include "MHZ19.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_HTU21DF.h"

#define BAUDRATE 9600

#define CO2_RX_PIN 18                                        
#define CO2_TX_PIN 19   

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_SDA 21
#define SCREEN_SCL 22
#define OLED_RESET -1 
#define WIFI true

char output[256];
Plantower_PMS7003 dustsensor = Plantower_PMS7003();
MHZ19 CO2sensor;                                             
HardwareSerial CO2serial(0);
Adafruit_HTU21DF htu = Adafruit_HTU21DF();
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint16_t pm1;
uint16_t pm25;
uint16_t pm10;
double hum;
double temp;
float CO2;

WebServer server(80);

void setup()
{
  // serial commumication through USB
  Serial.begin(BAUDRATE);
  // serial with PMS7003
  Serial2.begin(BAUDRATE);
  dustsensor.init(&Serial2);
  // serial with MHZ19
  CO2serial.begin(BAUDRATE, SERIAL_8N1, CO2_RX_PIN, CO2_TX_PIN); // ESP32 example
  CO2sensor.begin(CO2serial); 
  
  if (!htu.begin()) {
    Serial.println("HTU21 allocation failed");
    while (1);
  }
  Wire.begin(SCREEN_SDA, SCREEN_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  // Clear the buffer1
  display.clearDisplay();
  display.display();
//  display.clearDisplay();
  // Normal 1:1 pixel scale
  display.setTextSize(1);      
  display.setTextColor(SSD1306_WHITE);

  ///////////////////////
  // manage server; allow to enable/disable server with a button
  // give the possibility to change ssid and pw?
  // start web server
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    display.print(".");
    display.display();
  }
  display.println("");
  display.print("Connected to ");
  display.println(ssid);
  display.print("IP address: ");
  display.println(WiFi.localIP());
  display.display();
  if (MDNS.begin("esp32")) {
    display.println("MDNS responder started");
  }
  server.on("/", handle_root);
  server.begin();
  display.println("HTTP server started");
  display.display();
  ////////////////////////
  
  // wait a little to initialize stuff
  delay(2000);
}

// Handle root url (/)
void handle_root() {
  sprintf(output, 
  "<b>PM<sub>1.0</sub></b>:  %2d ug/m<sup>3</sup><br>"
  "<b>PM<sub>2.5</sub></b>:  %2d ug/m<sup>3</sup><br>"
  "<b>PM<sub>10</sub></b> :  %2d ug/m<sup>3</sup><br>"
  "<b>Hum.</b> :  %.1f &#37;<br>"
  "<b>Temp.</b>:  %.1f &deg;C<br>"
  "<b>CO<sub>2</sub></b>  :  %.0f ppm<br>",
  pm1,pm25, pm10, hum, temp, CO2);
  
  server.send(200, "text/html", output);
}
 
void loop()
{
  server.handleClient();
  dustsensor.updateFrame();
  if (dustsensor.hasNewData()) {   
    display.clearDisplay();
    
    // dust sensor
    // all in (ug/m3)
    pm1 = dustsensor.getPM_1_0_atmos();
    pm25 = dustsensor.getPM_2_5_atmos();
    pm10 = dustsensor.getPM_10_0_atmos();
    sprintf(output, "PM1.0: %2d ug/m3\nPM2.5: %2d ug/m3\nPM10 : %2d ug/m3",pm1,pm25,pm10);
    display.setCursor(0, 0);
    display.println(output);
    
    // humidity / temperature sensor
    temp = htu.readTemperature();
    hum = htu.readHumidity();
    sprintf(output, "Hum. :  %.1f %c\nTemp.:  %.1f %cC", hum,(char)37,temp,(char)247);
    display.println(output);

    // CO2 sensor / double?
    CO2 = CO2sensor.getCO2Raw();
    int8_t CO2temp = CO2sensor.getTemperature();  
    // Exponential equation for Raw & CO2 relationship (see docs)
    CO2 = 6.60435861e+15 * exp(-8.78661228e-04 * CO2);
    sprintf(output, "CO2  :  %.0f ppm", CO2);
    display.println(output);

    display.println("");
    display.println(WiFi.localIP());
    
    display.display();
    
  }
}
