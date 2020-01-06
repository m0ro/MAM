# MAM
MAM Air Monitor - dirty air monitor with ESP32

Simple code to test PM and CO2 sensors

TBD:
- clean code/uniform serial controls
- temperature/humidity sensor
- some kind control
- SD data dump
- BT data dump, compatible with github.com/HabitatMap
- clean code

External code used:
- https://github.com/WifWaf/MH-Z19
- https://github.com/jmstriegel/Plantower_PMS7003

Libraries from Adafroid for OLED screen

- MCU: ESPRESSIF **ESP32-WROOM-32D**
- particulate sensor: **PMS7003** 
- CO2 sensor: **MH-Z19B**
- Display: SPI oled display 128x64

PMS7003 pinout:
- PIN1 5V
- PIN2 5V
- PIN3 GND 
- PIN4 GND 
- PIN5 RESET
- PIN6 NC 
- PIN7 RX -> GPIO17 (Serial2)
- PIN8 NC 
- PIN9 TX -> GPIO16 (Serial2)
- PIN10 SET (while low level is sleeping mode)

MH-Z19B pinout:
- analog out -> GPIO5 (not used)
- RX -> GPIO18
- TX -> GPIO19

Display pinout:
- SCL -> GPIO22
- SDA -> GPIO21
