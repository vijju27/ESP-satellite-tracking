# ESP satellite tracking

Using a ESP8266/ESP32 to fetch the satellite pass info from n2yo website using API and display the parameters on the E-Paper display.

## Images
<img src="https://github.com/vijju27/ESP-satellite-tracking/blob/master/fetching.jpg" alt="fetching logo" width="200" height="200"> | <img src="https://github.com/vijju27/ESP-satellite-tracking/blob/master/Pass.jpg" alt="passes" width="200" height="200">

## Hardware used
1. ESP32 Dev Board.
2. 1.54 inch B/W E-paper display from waveshare.

## Steps for getting n2yo API KEY

1. Register on [n2yo.com](https://www.n2yo.com)
2. Grab your [API key](https://www.n2yo.com/login/edit/)
3. API Documentation can be found [here](https://www.n2yo.com/api/)

## Software required
1. Download and install [Arduino IDE](https://www.arduino.cc/en/main/software).
2. Arduino core for ESP8266 or ESP32 depending on which microcontroller you are using. Installation guide for [ESP8266](https://github.com/adafruit/Adafruit-GFX-Library) and [ESP32](https://github.com/espressif/arduino-esp32/blob/master/README.md). 

## Required libraries
1. [GxEPD2](https://github.com/ZinggJM/GxEPD2) library for driving 1.54 inch B/W E-Paper Display.
2. [ArduinoJSON](https://github.com/bblanchon/ArduinoJson) library for parsing JSON data.
3. [Adafruit RTCLib](https://github.com/adafruit/RTClib) to convert EPOCH time to Local Time.
4. [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) library for fonts and graphics.

Note : All the above libraries can be installed from Arduino library manager.
## Code Tweaks
### 1. Board
The code was written for ESP32,
If you are using ESP8266 then replace the following lines
```c
#include<WiFi.h>
GxEPD2_BW<GxEPD2_154, GxEPD2_154::HEIGHT> display(GxEPD2_154(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));
```
with 
```c
#include<ESP8266WiFi.h>
GxEPD2_BW<GxEPD2_154, GxEPD2_154::HEIGHT> display(GxEPD2_154(/*CS=D8*/ SS, /*DC=D3*/ 0, /*RST=D4*/ 2, /*BUSY=D2*/ 4));
```
I'm using a B/W 1.54 inch E-paper display. if you are using other display's make sure to change the above line with your display supported constructor from the examples found in GxEPD2 library.

### Variables
Replace the NORAD ID of the satellite that you want to track. list can be found [here](https://www.n2yo.com/satellites/)
```C
#define Satellite_ID 27607 // SO-50
```
Enter your GPS co-ordinates and altitude
```C
float Observer_Lat = 41.702;
float Observer_Lon = -76.014;
#define Altitude 0
```
Number of days pass prediction data you want to fetch from n2yo
```C
#define Days 1
```
Minimum amount of time for the satellite passing above the horizon. Considered as a threshold
```C
#define Minimum_Seconds 300
```
Enter your API key and WiFi information here
```C
String API_KEY = "PASTE N2YO API KEY HERE";
#define SSID "Your WiFi Name"
#define Password "Your WiFI Password"
```
change it to true if you want to display only the next pass and false to display all the next passes fetched.
Note : this line doesn't effect the data fetching.
```C
boolean only_next_pass = false;//
```
## Credits
This code uses Open Source components. I am thankful to these developers for their contributions to open source.



## To-Do
1. Store the passes for next 'x' days in the SD Card. So we no longer need to connect to wifi all the time.
2. Servos to adjust antenna Azimuth and Elevation. Additional microcontroller may be needed so this will be a seperate unit.

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

Please make sure to update tests as appropriate.

## License
[MIT License](https://choosealicense.com/licenses/mit/)
