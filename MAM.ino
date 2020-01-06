/*********************************************************************
MAM Air Monitor
m0ro
**********************************************************************/

// MCU: ESPRESSIF ESP32-WROOM-32D
// particulate sensor: PMS7003 
// CO2 sensor: MH-Z19B
// Display: SPI oled display 128x64

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
// code from https://github.com/jmstriegel/Plantower_PMS7003
#include "Plantower_PMS7003.h"
// code from https://github.com/WifWaf/MH-Z19
#include "MHZ19.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SERIALOUT true
#define BAUDRATE 9600

#define CO2_RX_PIN 18                                        
#define CO2_TX_PIN 19   

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_SDA 21
#define SCREEN_SCL 22
#define OLED_RESET -1 

char output[256];
Plantower_PMS7003 dustsensor = Plantower_PMS7003();
MHZ19 CO2sensor;                                             
HardwareSerial CO2serial(0);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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
  
  Wire.begin(SCREEN_SDA, SCREEN_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000);
  // Clear the buffer
  display.clearDisplay();
}
 
void loop()
{
  dustsensor.updateFrame();
  if (dustsensor.hasNewData()) {
    display.clearDisplay();
    // Normal 1:1 pixel scale
    display.setTextSize(1);      
    display.setTextColor(SSD1306_WHITE);
    
    // dust sensor
    // all in (ug/m3)
    sprintf(output, "PM1.0 : %2d ug/m3\nPM2.5 : %2d ug/m3\nPM10  : %2d ug/m3",
      dustsensor.getPM_1_0_atmos(),
      dustsensor.getPM_2_5_atmos(),
      dustsensor.getPM_10_0_atmos());
    display.setCursor(0, 0);
    display.println(output);

    // CO2 sensor
    double adjustedCO2 = CO2sensor.getCO2Raw();
    int8_t CO2temp = CO2sensor.getTemperature();  
    // Exponential equation for Raw & CO2 relationship (see docs)
    adjustedCO2 = 6.60435861e+15 * exp(-8.78661228e-04 * adjustedCO2);
    sprintf(output, "CO2: %d ppm\n %2dC", adjustedCO2, CO2temp);
    display.println(output);
    
    display.display();
    
    if (SERIALOUT) {
      sprintf(output, "PM1.0 : %2d / %2d\nPM2.5 : %2d / %2d\nPM10  : %2d / %2d",
          dustsensor.getPM_1_0(), dustsensor.getPM_1_0_atmos(),
          dustsensor.getPM_2_5(), dustsensor.getPM_2_5_atmos(),
          dustsensor.getPM_10_0(), dustsensor.getPM_10_0_atmos());
      Serial.println(output);
      // indicates the number of particles with diameter beyond *um in 0.1 L of air. 
      sprintf(output, "%d %d %d %d %d %d",
          dustsensor.getRawGreaterThan_0_3(),
          dustsensor.getRawGreaterThan_0_5(),
          dustsensor.getRawGreaterThan_1_0(),
          dustsensor.getRawGreaterThan_2_5(),
          dustsensor.getRawGreaterThan_5_0(),
          dustsensor.getRawGreaterThan_10_0());
      Serial.println(output);
      sprintf(output, "CO2: %d ppm\n %2dC", adjustedCO2, CO2temp);
      Serial.println(output);
    }
  }
}
