// MCU: ESPRESSIF ESP32-WROOM-32D
//
// particulate sensor: PMS7003 
// 
// UART2 (Serial2): (GPIO 17 and GPIO 16)
//PIN1 5V
//PIN2 5V
//PIN3 GND 
//PIN4 GND 
//PIN5 RESET
//PIN6 NC 
//PIN7 RX 3.3V logic
//PIN8 NC 
//PIN9 TX 3.3V logic
//PIN10 SET while low level is sleeping mode
//
// CO2 sensor: MH-Z19B
// analog out to PIN 5
// RX 18
// TX 19
//
// Display: SPI oled display 128x64
// 22 SCL
// 21 SDA
//

#define DIAGNOSTIC false
#define BAUDRATE 9600

// code from https://github.com/jmstriegel/Plantower_PMS7003
#include "Plantower_PMS7003.h"
char output[256];
Plantower_PMS7003 pms7003 = Plantower_PMS7003();

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// code from https://github.com/WifWaf/MH-Z19
#include "MHZ19.h"
#include <Arduino.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define SCREEN_SDA 21
#define SCREEN_SCL 22
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#define CO2_RX_PIN 18                                        
#define CO2_TX_PIN 19                                         

MHZ19 myMHZ19;                                             
HardwareSerial mySerial(0);                              // ESP32 example

unsigned long getDataTimer = 0;

void setup()
{
  Serial.begin(BAUDRATE);

  Serial2.begin(BAUDRATE);
  pms7003.init(&Serial2);

  mySerial.begin(BAUDRATE, SERIAL_8N1, CO2_RX_PIN, CO2_TX_PIN); // ESP32 example
  myMHZ19.begin(mySerial); 
  
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
  pms7003.updateFrame();

  if (pms7003.hasNewData()) {
    display.clearDisplay();
    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    // all in (ug/m3)
    sprintf(output, "PM1.0 : %2d / %2d\nPM2.5 : %2d / %2d\nPM10  : %2d / %2d",
      pms7003.getPM_1_0(), pms7003.getPM_1_0_atmos(),
      pms7003.getPM_2_5(), pms7003.getPM_2_5_atmos(),
      pms7003.getPM_10_0(), pms7003.getPM_10_0_atmos());
    display.setCursor(0, 0);
    display.println(output);  
    
    double adjustedCO2 = myMHZ19.getCO2Raw();
    int8_t CO2temp = myMHZ19.getTemperature();  
    
    // Exponential equation for Raw & CO2 relationship
    adjustedCO2 = 6.60435861e+15 * exp(-8.78661228e-04 * adjustedCO2);
    sprintf(output, "CO2: %.2f ppm, %2d'C", adjustedCO2, CO2temp);
    display.println(output);
    display.display();
    
    if (DIAGNOSTIC) {
//       indicates the number of particles with diameter beyond *um in 0.1 L of air. 
      sprintf(output, "%d %d %d\%d %d %d",
          pms7003.getRawGreaterThan_0_3(),
          pms7003.getRawGreaterThan_0_5(),
          pms7003.getRawGreaterThan_1_0(),
          pms7003.getRawGreaterThan_2_5(),
          pms7003.getRawGreaterThan_5_0(),
          pms7003.getRawGreaterThan_10_0());
      display.println(output);
      display.display();

    }
  }
}
