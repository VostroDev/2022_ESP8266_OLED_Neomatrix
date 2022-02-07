# 2022_ESP32MessageBoard_Neomatrix

## Description

This project is intended for use with an ESP32 microcontroller, DS3231 I2C RTC module and 4 8x8 NeoMatrix boards.
The display will scroll through a sequence displaying:

* Current Time
* Temperature
* Day of the week
* Day and Month
* Custom Message
The time and message can be set using the build in webserver.

## Getting Started

Project build with Visual Studio Code and PlatformIO.

Click the image below to watch the video.

[![YouTube](http://img.youtube.com/vi/KXobKjZ4cho/0.jpg)](https://www.youtube.com/watch?v=KXobKjZ4cho "Dot Matrix Clock")

![Web Server](https://github.com/VostroDev/2022_ESP32MessageBoard_Neomatrix/blob/V2.0/docs/webserver_v2.png)

### Dependencies

* Visual Studio Code: <https://code.visualstudio.com/>
* PlatformIO: <https://platformio.org/>
* NeoPixel NeoMatrix: <https://www.adafruit.com/product/1487>
* DS3231 RTC Module: <https://www.adafruit.com/product/3013>
* ESP32 Dev Board: <https://www.adafruit.com/product/3405>
* index.html, notfound.html, settings.html, time.html, timepicker.html
* Main.cpp, FontRobert.h, EEPROMHandler.h

## Authors

Contributors names and contact info

* [R Wilson](vostrodev@gmail.com)  

## Version History

* 1.0
  * Initial Release using default webserver
* 1.1
  * Added brightness control
* 2.0
  * Moved to AsyncWebserver with SPIFFS
  * Minor visual update
* 2.0.1
  * Webpage updates

## Acknowledgments

All credit attributes of the various authors of the libraries used.

* [Adafruit - NeoMatrix Boards](https://www.adafruit.com/product/1487)
* [Adafruit - RTC Lib](https://github.com/adafruit/RTClib)
* [ArduinoJson - Benoit Blanchon](https://arduinojson.org)
* [FastLED - Daniel Garcia](https://fastled.io)
* [LEDText - A Liddiment](https://github.com/AaronLiddiment/LEDText)
* [LEDMatrix - A Liddiment](https://github.com/AaronLiddiment/LEDMatrix)
* [me-no-dev - AsyncTCP](https://github.com/me-no-dev/AsyncTCP)
* [me-no-dev - ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
* [Rui Santos - Information & tutorials](https://RandomNerdTutorials.com)
