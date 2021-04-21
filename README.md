# MAM
MAM Air Monitor - very dirty air monitor with ESP32.

Simple code to test PM and CO2 sensors, based on:

- MCU: ESPRESSIF **ESP32-WROOM-32D** 
- Temperature and Humidity: **HTU21**
- particulate sensor: **PMS7003** 
- CO2 sensor: **MH-Z19B**
- Display: SPI oled display 128x64


Read the wifi credentials from wifi.txt: first row ssid, second password.

Dump to pm_log.csv :

| date | pm1 [ug/m3] | pm2.5 [ug/m3] | pm10 [ug/m3] | hum [%] | temp ['C] | CO2 [ppm] |
| ---- | ---- | ---- | ---- | ---- | ---- | ---- |


TBD:
- some kind control (a few buttons?)
- data dump through bluetooth, compatible with github.com/HabitatMap
- clean code, always

External code used:
- https://github.com/WifWaf/MH-Z19
- https://github.com/jmstriegel/Plantower_PMS7003
- https://forum.arduino.cc/index.php?topic=650384.0

HTU21 and OLED dotscreen display uses Adafruit libraries (easy intallable from Arduino environment)

PMS7003 pinout:
- PIN1 -> 5V
- PIN3 -> GND 
- PIN7 RX -> TX2
- PIN9 TX -> RX2

MH-Z19B pinout:
- VCC -> 5V
- GND -> GND
- RX -> GPIO15
- TX -> GPIO4

HTU21 pinout:
- VCC -> 3.3V
- GND -> GND
- SCL -> GPIO22
- SDA -> GPIO21

SD card pinout:
- VCC -> 3.3V
- GND -> GND
- CS -> GPIO5
- MOSI -> GPIO23
- SCK -> GPIO18
- MISO -> GPIO19

OLED Display pinout:
- VCC -> 3.3V
- GND -> GND
- SCL -> GPIO22
- SDA -> GPIO21


