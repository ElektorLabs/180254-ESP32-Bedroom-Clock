#  180254 ESP32 MQTT Sensor Hub / Bedroomclock and Temperatur and Humiditysensor
# Software for the ESP32 Bedroomclock 

This will display the Time and MQTT Messages with the current temperature 

## Getting Started

Download the source and open it with the arduino ide ( >= 1.8.x ) and select ESP32-PICO-KIT as target.
If not done jet install the ESP32 through the boardmanager. For details about doing that see https://github.com/espressif/arduino-esp32
for more information. You need to install the libaries from the prerequiest to be ready to compile the code.  


### Prerequisites

You need the following libraries:

 * Time by Michael Margolis ( Arduino )
 * Arduino JSON 6.x
 * CRC32 by Christopher Baker 
 * PubSubClient
 * Adafruit BME280 Library 1.0.7 by Adafruit
 * Adafruit Unified Sensor 1.0.2 by Adafruit
 * Adafruit TSL2561 by Adafruit
 * Adafruit VEML6070 by Adafruit
 * WebSockets by Markus Sattler
 * NTP client libary from https://github.com/gmag11/NtpClient/
 * DHTesp
 
Aftwerwards you are ready to compile the code. If oyu like to connect a DHT11/DHT22 use Pin 15

Also you need to the sensor to compile additionally: 

 * ( optional U8glib )


For more information about the project have a look at:
https://www.elektormagazine.com/labs/bedroom-clock-with-out-side-temperature-based-on-esp32
