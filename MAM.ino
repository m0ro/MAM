/*********************************************************************
MAM Air Monitor
m0ro

implement:
https://randomnerdtutorials.com/esp32-ntp-client-date-time-arduino-ide/
Install the ESP32 IDE with 
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
into : "preferences -> additional boards manager", then add ESP32 with "board manager"
SD card logging
SD card read of preferences (for the moment, wifi parameters)
button to show the screen

connect to RTC and read time for the log
remember to press "boot" button to updoad the firmware

implement timers and sleep mode? (for battery operation)
https://randomnerdtutorials.com/esp32-data-logging-temperature-to-microsd-card/

// MCU: ESPRESSIF ESP32-WROOM-32D
// dust sensor: PMS7003 
// CO2 sensor: MH-Z19B
// Humidity/Temperature sensor: HTU21
// Display: SPI oled display 128x64

**********************************************************************/

/****
 * di fatto l'unica roba che non funziona e' l'i2c...sembra non essere reliable, ogni tanto va, ogni tanto no...un contatto falso nel socket? controllare se si perdono bit (e.g. fare un confronto dei
 * dati prima e dopo con l'oscilloscopio, tipo una funzione "sottrazione"...
 *****/


#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include "time.h"

#include "FS.h"
#include "SD.h"

#define SD_CS 5
#define SD_SCK 18
#define SD_MOSI 23
#define SD_MISO 19
String dataMessage;

char ssid[50];
char password[50];

// code from https://github.com/jmstriegel/Plantower_PMS7003
#include "Plantower_PMS7003.h"
// code from https://github.com/WifWaf/MH-Z19
#include "MHZ19.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_HTU21DF.h"

#define BAUDRATE 9600
  
#define CO2_RX_PIN 15                                 
#define CO2_TX_PIN 4

#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define SCREEN_SDA 22
#define SCREEN_SCL 21
#define OLED_RESET -1 

#define WIFI true

char output[256];
int timeout = 0;

// use NTP server to get time
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
struct tm timeinfo;

// create objects for each sensor
Plantower_PMS7003 dustsensor = Plantower_PMS7003();
MHZ19 CO2sensor;                                             
HardwareSerial CO2serial(0);
Adafruit_HTU21DF htu = Adafruit_HTU21DF();
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

char formatted_date[256];
uint16_t pm1;
uint16_t pm25;
uint16_t pm10;
double hum;
double temp;
float CO2;

// initialize webserver
WebServer server(80);

void readFile(fs::FS &fs, const char * path){
    display.printf("Reading file: %s\n", path);
    File file = fs.open(path);
    if(!file){
        display.println("Failed to open file for reading");
        return;
    }
    display.print("Read from file: ");
    while(file.available()){
        display.write(file.read());
    }
    file.close();
}

void readWifiPars(fs::FS &fs, const char * path){
    File file = fs.open(path);
    if(!file){
        display.println("Failed to open wifi settings...");
        return;
    }
    // thanks https://forum.arduino.cc/index.php?topic=650384.0
    while(file.available() && file.peek() != '\n') {
      int l = file.readBytesUntil('\n', ssid, sizeof(ssid));
      if (l > 0 && ssid[l-1] == '\r') {
        l--;
      }
      ssid[l] = 0;
      while (file.available()) {
        int l = file.readBytesUntil('\n', password, sizeof(password));
        if (l > 0 && password[l-1] == '\r') {
          l--;
        }
        password[l] = 0;
        }
    }
    file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    display.printf("Writing file: %s\n", path);
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        display.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        display.println("File written");
    } else {
        display.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    display.printf("Appending to file: %s\n", path);
    File file = fs.open(path, FILE_APPEND);
    if(!file){
        display.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        display.println("Message appended");
    } else {
        display.println("Append failed");
    }
    file.close();
}

// Handle root url (/)
void handle_root(){
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

void print_to_serial(){
    // print to serial
    Serial.println(&timeinfo, "%H:%M:%S %d/%m/%y");
    sprintf(output, "PM1.0: %2d ug/m3\nPM2.5: %2d ug/m3\nPM10 : %2d ug/m3",pm1,pm25,pm10);
    Serial.println(output);
    sprintf(output, "Hum. :  %.1f %c\nTemp.:  %.1f %cC", hum,(char)37,temp,(char)247);
    Serial.println(output);
    sprintf(output, "CO2  :  %.0f ppm", CO2);
    Serial.println(output);
    Serial.println(WiFi.localIP());
}

void print_to_screen(){
    // print to screen
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(&timeinfo, "%H:%M:%S %d/%m/%y");
    sprintf(output, "PM1.0: %2d ug/m3\nPM2.5: %2d ug/m3\nPM10 : %2d ug/m3",pm1,pm25,pm10);
    display.println(output);
    sprintf(output, "Hum. :  %.1f %c\nTemp.:  %.1f %cC", hum,(char)37,temp,(char)247);
    display.println(output);
    sprintf(output, "CO2  :  %.0f ppm", CO2);
    display.println(output);
    display.println(WiFi.localIP());
    display.display();
}

void dump_to_sd(){
    // dump to sd card
    dataMessage = String(formatted_date) + ","  
                + String(pm1) + "," 
                + String(pm25) + "," 
                + String(pm10) + "," 
                + String(hum) + "," 
                + String(temp) + "," 
                + String(CO2) + "\r\n";
    appendFile(SD, "/pm_log.csv", dataMessage.c_str());
}

void setup(){
  pinMode(2, OUTPUT);
  // serial commumication through USB
  Serial.begin(BAUDRATE);
  Serial.println('Serial active...');
  Wire.begin(SCREEN_SDA, SCREEN_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS, false, false)) {
    Serial.println("SSD1306 allocation failed");
  }
  else {
    Serial.println("SSD1306 allocation succeed");
    // Clear the buffer
    display.clearDisplay();
    display.display();
    // Normal 1:1 pixel scale
    display.setTextSize(1);      
    display.setTextColor(WHITE);
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(50);
    display.println("SSD1306 allocation succeed");
    display.display();
  }
  if (!htu.begin()) {
    Serial.println("HTU21 allocation failed");
  }
  else{
    Serial.println("HTU21 allocation succeed");
  }
  // sd card initialization
//  SPIClass sd_spi(VSPI);
//  sd_spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  SD.begin(SD_CS); 
//  if (!SD.begin(SD_CS, sd_spi)){
  if (!SD.begin(SD_CS)){
    Serial.println("Card Mount Failed");
//    return;
  }
  else{
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Size: %lluMB\n", cardSize);
    // verify if log file exist, if not, create it
    File file = SD.open("/pm_log.csv");
    if(!file) {
      Serial.println("pm_log doens't exist");
      Serial.println("Creating pm_log...");
      writeFile(SD, "/pm_log.csv", "date,pm1,pm25,pm10,hum,temp,CO2 \r\n");
    }
    else{
      Serial.println("pm_log already exists");  
    }
    file.close();
    
    // read ssid and pw of wifi from wifi.txt in the sd card
    readWifiPars(SD, "/wifi.txt");
    Serial.println(ssid);
    Serial.println(password);
  }
  
  // start serial for PMS7003
  Serial2.begin(BAUDRATE);
  dustsensor.init(&Serial2);
  // start serial for MHZ19
  CO2serial.begin(BAUDRATE, SERIAL_8N1, CO2_TX_PIN, CO2_RX_PIN);
  CO2sensor.begin(CO2serial); 

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  timeout = 0;
  while ((WiFi.status() != WL_CONNECTED) and (timeout<10)) {
    delay(500);
    Serial.print(".");
    timeout = timeout + 1;
  //    display.display();
  }
  if (timeout<10){
    Serial.println("");
    Serial.println(ssid);
    Serial.println(WiFi.localIP());
  //  display.display();
    if (MDNS.begin("esp32")){
      Serial.println("multicast DNS start");
    }
    server.on("/", handle_root);
    server.begin();
    Serial.println("HTTP server started");
  
    //init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  }
  
  display.display();
  
  // wait a little to initialize stuff
  delay(2000);
  
}

void loop(){
  server.handleClient();
  dustsensor.updateFrame();
  // could be interesting to set the display on only between certain times...
  // or use a free pin for a switch...or read from web, or from SD
  display.ssd1306_command(SSD1306_DISPLAYON);
  //    display.ssd1306_command(SSD1306_DISPLAYOFF);

  // it's really needed to activate only if there is new data from dustsensor?
//  if (dustsensor.hasNewData()){
    // collect data
    // dust sensor, all in (ug/m3)
    pm1 = dustsensor.getPM_1_0_atmos();
    pm25 = dustsensor.getPM_2_5_atmos();
    pm10 = dustsensor.getPM_10_0_atmos();
    // humidity / temperature sensor
    temp = htu.readTemperature();
    hum = htu.readHumidity();
    // CO2 sensor
    CO2 = CO2sensor.getCO2Raw();
    int8_t CO2temp = CO2sensor.getTemperature();  
    // Exponential equation for Raw & CO2 relationship (see docs)
    CO2 = 6.60435861e+15 * exp(-8.78661228e-04 * CO2);
    // get time and format it
    if(!getLocalTime(&timeinfo)){
      Serial.println("fail to get time");
    }
    strftime(formatted_date, sizeof(formatted_date), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);

    print_to_serial();
    print_to_screen();
    dump_to_sd();

    digitalWrite(2, HIGH);
    delay(100);
    digitalWrite(2, LOW);
    delay(100);
    digitalWrite(2, HIGH);
    delay(100);
    digitalWrite(2, LOW);
    
    delay(5*1000);
//  }
}
