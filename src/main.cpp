/*----------------------------------------------------------------------------------------
  30/12/2021
  Author: R WILSON
  Platforms: ESP8266 with OLED
  Version: 3.0.0 - 07 Feb 2022
  Language: C/C++/Arduino
  Working
  ----------------------------------------------------------------------------------------
  Description:
  ESP32 connected to NeoPixel WS2812B LED matrix display (32x8 - 4 Panels)
  MATRIX_TYPE      VERTICAL_MATRIX
  RTC DS3231 I2C - SDA(21)gray and SCL(22)purple
  TEMP SENSOR BUILD INTO DS3231
  ----------------------------------------------------------------------------------------
  Libraries:
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  Adafruit RTC Lib: https://github.com/adafruit/RTClib              (lib version 2.0.2)
  ArduinoJson Benoit Blanchon: https://arduinojson.org              (lib version 6.18.5)
  me-no-dev: https://github.com/me-no-dev/AsyncTCP
  me-no-dev: https://github.com/me-no-dev/ESPAsyncWebServer
  Rui Santos: https://RandomNerdTutorials.com - information resource
  
  FastLED Daniel Garcia: https://fastled.io/                        (lib version
  LEDText A Liddiment https://github.com/AaronLiddiment/LEDText     (lib version 3.4.0)
  LEDMatrix A Liddiment https://github.com/AaronLiddiment/LEDMatrix (lib version )
    modified: J Skrotzky https://github.com/Jorgen-VikingGod/LEDMatrix Dec'21)
   
  LOAD TO SPIFFS THESE EXTERNAL FILES:
    >> index.html notfound.html settings.html time.html timepicker.html 
  Connect to ESP32MessageBoard WIFI AP created by ESP32  
  Open browser to http://192.168.4.1/ or www.message.com !not working at this time
  Open browser to http://1.2.3.4/ 
  Password: 12345678 or password
  Enter message to be displayed on the NeoMatrix scrolling display
----------------------------------------------------------------------------------------*/

#include <Arduino.h>
#include <SPI.h>
#include <ESP8266WiFi.h>                    // * for esp32 use <WiFi.h>
#include <ESPAsyncTCP.h>                    // * for esp32 use <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "EEPROMHandler.h"                  // Storing message into permanent memory
#include <FS.h>                             // * for esp32 use "SPIFFS.h"

#include <FastLED.h>
#include <LEDMatrix.h>
#include <LEDText.h>
#include "FontRobert.h"                     // for 5x7 font use <FontMatriseRW.h>

#include <Wire.h>
#include "RTClib.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128                    // OLED display width, in pixels
#define SCREEN_HEIGHT 32                    // OLED display height, in pixels
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BUF_SIZE   400                      // 400 out of 512 used
#define PASS_BSIZE 40                       // 40 out of 512 used
#define PASS_EXIST 450                      // password $ address
#define PASS_BEGIN 460                      // password value stored
#define P_CHAR     '`'
#define BRT_BEGIN  505                      // Brightness value stored

//#define LED_BUILTIN 26
#define LED_PIN     7                       // * for ESP32 use 27
#define VOLTS       5
#define MAX_MA      400

#define MATRIX_WIDTH  -32
#define MATRIX_HEIGHT -8
#define MATRIX_TYPE VERTICAL_MATRIX

#define  EFF_CHAR_UP          0xd8          // in sprintf change EFFECT_CHAR_UP to EFF_CHAR_UP in loop
#define  EFF_CHAR_DOWN        0xd9
#define  EFF_CHAR_LEFT        0xda
#define  EFF_CHAR_RIGHT       0xdb

#define  EFF_SCROLL_LEFT      0xdc
#define  EFF_SCROLL_RIGHT     0xdd
#define  EFF_SCROLL_UP        0xde
#define  EFF_SCROLL_DOWN      0xdf

#define  EFF_RGB              0xe0
#define  EFF_HSV              0xe1
#define  EFF_RGB_CV           0xe2
#define  EFF_HSV_CV           0xe3
#define  EFF_RGB_AV           0xe6
#define  EFF_HSV_AV           0xe7
#define  EFF_RGB_CH           0xea
#define  EFF_HSV_CH           0xeb
#define  EFF_RGB_AH           0xee
#define  EFF_HSV_AH           0xef
#define  EFF_COLR_EMPTY       0xf0
#define  EFF_COLR_DIMMING     0xf1

#define  EFF_BACKGND_ERASE    0xf4
#define  EFF_BACKGND_LEAVE    0xf5
#define  EFF_BACKGND_DIMMING  0xf6

#define  EFF_FRAME_RATE       0xf8
#define  EFF_DELAY_FRAMES     0xf9
#define  EFF_CUSTOM_RC        0xfa

int BRIGHTNESS = 30;

int rc;                                     // custom return char for ledMatrix lib

char ssid[] = "ESP32MessageBoard";          // Change to your name
char password[PASS_BSIZE] = "12345678";     // dont change password here, change using web app

uint16_t h = 0;
uint16_t m = 0;
uint16_t d = 0;
uint16_t mnth = 0;
uint16_t yr = 0;
RTC_Millis RTC; //! RTC_DS3231 RTC;

IPAddress ip(1, 2, 3, 4);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer server(80);
DateTime now;                               // Decalre global variable for time
char szTime[15];                             // hh:mm\0  "Time: %02d:%02d"
char szDate[15];                             //          "Date: 01/01/22"
char daysOfTheWeek[7][4] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

char curMessage[BUF_SIZE] = "Vostro";
char newMessage[BUF_SIZE] = "Vostro";
char newTime[17] = "01.01.2022 12:00";
bool newMessageAvailable = true;
bool newTimeAvailable = false;

cLEDMatrix<MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_TYPE> leds;
cLEDText ScrollingMsg, StaticgMsg, RTCErrorMessage;

CRGB fleds[256];

char txtDateA[] = { EFFECT_HSV_AH "\x00\xff\xff\xff\xff\xff" "12|30" };
char txtDateB[] = { EFFECT_HSV_AH "\x00\xff\xff\xff\xff\xff" "12:30" };
char szMesg[BUF_SIZE] = { EFFECT_FRAME_RATE "\x00" EFFECT_HSV_AH "\x00\xff\xff\xff\xff\xff" EFFECT_SCROLL_LEFT "     ESP32 MESSAGE BOARD BY R WILSON     "  EFFECT_CUSTOM_RC "\x01" };

String handleTimeUpdate(uint8_t *data, size_t len){
  data[len] = '\0';
  String json = (char*)data;

  for (int i = 0; i < len; i++) {
    newTime[i] = data[i];
  }
  
  if (data[0] == '\0'){ newTimeAvailable = false;}          // if not a blank field
  else {newTimeAvailable = true;}

  Serial.print("new time: ");
  Serial.println(json);

  return "{\"status\" : \"ok\", \"time\" : \"" + json + "\"}";
}

String handleMessageUpdate(uint8_t *data, size_t len){
  data[len] = '\0';
  String json = (char*)data;
  
  DynamicJsonDocument doc(JSON_OBJECT_SIZE(2) + 350);
  DeserializationError error = deserializeJson(doc, json);  // Deserialize the JSON document

  if (error){                                                // Test if parsing succeeds.
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return "deserializeJson error";
  }

  BRIGHTNESS = int(doc["brightness"]);
  strcpy(newMessage, doc["message"]);

  Serial.print("message and brightness handle: ");
  Serial.println(json);
  
  newMessageAvailable = true;

  return json;
}

String handleSettingsUpdate(uint8_t *data, size_t len){
  String returnstring;

  data[len] = '\0';
  String json = (char*)data;
  
  DynamicJsonDocument doc(JSON_OBJECT_SIZE(3) + 130);
  DeserializationError error = deserializeJson(doc, json);   // Deserialize the JSON document

  if (error){                                                // Test if parsing succeeds.
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return "deserializeJson error";
  }

  const char *oldpassword = doc["oldpassword"];
  const char *newpassword = doc["newpassword"];
  const char *renewpassword = doc["renewpassword"];

  if (String(oldpassword) == String(password) && String(newpassword) == String(renewpassword))
  {
    returnstring = "password:" + String(password) + " newpassword:" + String(newpassword) + " renewpassword:" + String(renewpassword);
    Serial.print("password handle: ");
    Serial.println(json);
    eepromWriteChar(BUF_SIZE, '\0');        // wall so message doesnt display password at max buffer pos
    eepromWriteChar(PASS_EXIST, P_CHAR);    // user password now exists
    eepromWriteString(PASS_BEGIN, String(newpassword));
    Serial.println("new password saved");
    Serial.println(eepromReadChar(PASS_BEGIN));
    WiFi.softAPdisconnect();
    delay(8000);
    ESP.restart();
  }
  else
  {
    returnstring = "error, passwords don't match";
    Serial.print("password handle error: ");
    Serial.println(json);
    Serial.println("\nerror, passwords don't match\n");
    delay(1000);
  }

  return returnstring;
}

void updateDefaultAPPassword(){
  Serial.println("updateDefaultAPPassword");
  if (eepromReadChar(PASS_EXIST) == P_CHAR)
  {
    delay(200);
    Serial.print("user pwd found: \"");
  }
  else
  {
    delay(200);
    Serial.println("\nuser pwd is not found\nrestoring default pwd to ");

    eepromWriteChar(BUF_SIZE, '\0');           // wall so message doesnt display password at max buffer pos
    eepromWriteChar(PASS_EXIST, P_CHAR);       // user password now exists
    eepromWriteString(PASS_BEGIN, "password"); // default password is "password"
  }

  String eeString = eepromReadString(PASS_BEGIN, PASS_BSIZE);
  eeString.toCharArray(password, eeString.length() + 1);

  Serial.print(eeString);
  Serial.print("\" from EEPROM.\n\"");
  Serial.print(password);
  Serial.println("\" is used as the WIFI pwd");
  delay(1000);
}

void rtcErrorHandler(){
  char txtRTCError[BUF_SIZE] = { EFFECT_FRAME_RATE "\x00" EFFECT_HSV_AH "\x00\xff\xff\xff\xff\xff" EFFECT_SCROLL_LEFT "     RTC NOT FOUND     "  EFFECT_CUSTOM_RC "\x99" };
  RTCErrorMessage.SetFont(RobertFontData);
  RTCErrorMessage.Init(&leds, leds.Width(), RTCErrorMessage.FontHeight() + 1, 0, 0); //? change to +2 for 5x7 font
  RTCErrorMessage.SetText((unsigned char *)txtRTCError, sizeof(txtRTCError) - 1);
  RTCErrorMessage.SetTextColrOptions(COLR_RGB | COLR_SINGLE, 0x00, 0x00, 0xff);

  while(1) {
    if(RTCErrorMessage.UpdateText() != 0x99) {
      FastLED.show();
      delay(30);
    }
    else {
      RTCErrorMessage.SetText((unsigned char *)txtRTCError, sizeof(txtRTCError) - 1);
      RTCErrorMessage.UpdateText();
      FastLED.show();
    }
  }
}

void fxSinlon() //* Startup effects
{
  FastLED.addLeds<WS2812B,LED_PIN,GRB>(fleds, 250).setCorrection(TypicalLEDStrip);  //std fastled for effects
  FastLED.clear(true);
  FastLED.setBrightness(150);
  int gHue = 0, NUM_LEDS = 250, FRAMES_PER_SECOND = 120;
  while(gHue <201){
    fadeToBlackBy( fleds, NUM_LEDS, 20); // a colored dot sweeping back and forth, with fading trails
    int pos = beatsin16( 13, 0, NUM_LEDS-1 );
    fleds[pos] += CHSV( gHue, 255, 192);
    FastLED.show();  
    FastLED.delay(1000/FRAMES_PER_SECOND); 
    
    EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  }
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds[0], leds.Size()).setCorrection(TypicalLEDStrip); // back to Matrixled
  FastLED.setBrightness(BRIGHTNESS);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("");
  Serial.println("\n\nScrolling display from your Internet Browser");

  //  EEPROM
  EEPROM.begin(512);
  Serial.println("\n\nEEPROM STARTED");
  curMessage[0] = newMessage[0] = '\0';
  eepromReadString(0,BUF_SIZE).toCharArray(curMessage,BUF_SIZE);  // Read stored msg from EEPROM address 0
  newMessageAvailable = false;
  Serial.print("Message: ");
  Serial.println(curMessage);

  BRIGHTNESS = eepromReadChar(BRT_BEGIN);                          // read Neomatrix brightness value
  if(BRIGHTNESS > 255) { BRIGHTNESS = 255;}
  Serial.print("NeoMatrix Brightness set to ");                   // done in line 318
  Serial.println(BRIGHTNESS);
  
  //  START DISPLAY
  Serial.println("\nNEOMATRIX DIPLAY STARTED");
  FastLED.setMaxPowerInVoltsAndMilliamps(VOLTS, MAX_MA);
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds[0], leds.Size()).setCorrection(TypicalLEDStrip); //TypicalSMD5050
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear(true);

  ScrollingMsg.SetFont(RobertFontData);
  ScrollingMsg.Init(&leds, leds.Width(), ScrollingMsg.FontHeight() + 1, 0, 0); //? change to +2 for 5x7 font
  ScrollingMsg.SetText((unsigned char *)szMesg, sizeof(szMesg) - 1);
  ScrollingMsg.SetTextColrOptions(COLR_RGB | COLR_SINGLE, 0x00, 0x00, 0xff);

  StaticgMsg.SetFont(RobertFontData);
  StaticgMsg.Init(&leds, leds.Width(), ScrollingMsg.FontHeight() + 1, 1, 0); // >> 1 pixel //? change to +2 for 5x7 font
  StaticgMsg.SetText((unsigned char *)txtDateA, sizeof(txtDateA) - 1);
  StaticgMsg.SetTextColrOptions(COLR_RGB | COLR_SINGLE, 0x00, 0x00, 0xff);

  //  RTC
  Wire.begin(D1, D2);                             // DS3231 RTC I2C - SDA(21) and SCL(22) //! RTC  ??
  Serial.print("\nRTC STARTING >>> ");     
  RTC.begin(DateTime(F(__DATE__), F(__TIME__))); //! uncomment below dete this line
  /*                             
  if (! RTC.begin()) {
    Serial.println("RTC NOT FOUND");
    rtcErrorHandler();                      // Blocking call, dont move on
  }*/
  Serial.println("RTC STARTED\n");

  now = RTC.now(); 
  sprintf(szTime, "Time: %02d:%02d", now.hour(), now.minute());
  Serial.println(szTime);

  pinMode(LED_BUILTIN, OUTPUT);             // Heartbeat
  digitalWrite(LED_BUILTIN, LOW);

  //  SPIFFS
  if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
  }

  //  WIFI 2
  Serial.print("\nWIFI >> Connecting to ");
  Serial.println(ssid);  

  updateDefaultAPPassword();                // get Wifi password from EEPROM

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ip, ip, subnet);
  WiFi.softAP(ssid, password);


  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  server.on("/message", HTTP_POST, [](AsyncWebServerRequest * request){},NULL, 
      [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
          String myResponse = handleMessageUpdate(data,len);
          request->send(200, "text/plain", myResponse);
  });

  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/settings.html", "text/html");
  });
  server.on("/settings/send", HTTP_POST, [](AsyncWebServerRequest * request){},NULL, 
      [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
          String myResponse = handleSettingsUpdate(data,len);
          request->send(200, "text/plain", myResponse);
  });

  server.on("/time", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/time.html", "text/html");
  });
  server.on("/time/send", HTTP_POST, [](AsyncWebServerRequest * request){},NULL, 
      [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
          String myResponse = handleTimeUpdate(data,len);
          request->send(200, "text/plain", myResponse);
  });

  server.on("/timepicker", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/timepicker.html", "text/html");
  });
  server.on("/timepicker/send", HTTP_POST, [](AsyncWebServerRequest * request){},NULL, 
      [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
          String myResponse = handleTimeUpdate(data,len);
          request->send(200, "text/plain", myResponse);
  });

  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/notfound.html", "text/html");
  });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/favicon.png", "image/png");
  });
 
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  
  server.begin();
  Serial.println("SERVER STARTED");

  //  DISPLAY WELCOME MESSAGE
  fxSinlon();                                 //* Display special startup effect
  while(ScrollingMsg.UpdateText() != 1)
  {
    FastLED.show();
    delay(30);
  }
  ScrollingMsg.SetText((unsigned char *)szMesg, sizeof(szMesg) - 1);   // reset to start of string


  //Serial.println(TIMER_BASE_CLK);
  // START OLED
  Wire.begin(D1, D2);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
}

void loop()
{
  static uint32_t timeLast = 0;               // Heartbeat
  static uint8_t t = 0;                       // temperature
  static uint8_t updatetemp = 11;             // so updates temp at startup
  
  if (millis() - timeLast >= 1000){
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    timeLast = millis();

    now = RTC.now();                          // Update the global var with current time 
    m = now.minute();
    h = now.hour();
    d = now.day();
    mnth = now.month();
    yr = now.year();

    //txtDateA[7] = '2';// HRS//txtDateA[8] = '3';// HRS//txtDateA[10] = '5';// MIN//txtDateA[11] = '9';// MIN
    sprintf(txtDateA, "%c%c%c%c%c%c%c%02d%c%02d", EFF_HSV_AH,0x00,0xff,0xff,0xff,0xff,0xff,h,'|',m);
    sprintf(txtDateB, "%c%c%c%c%c%c%c%02d%c%02d", EFF_HSV_AH,0x00,0xff,0xff,0xff,0xff,0xff,h,':',m);

    sprintf(szMesg, "%c%c%c%c%c%c%c%c%c%c%s%02d%c%02d%c%c%c%c%c%c%c%c%c%c%s%02d%c%c%c%c%c%c%c%c%c%c%s%s%c%c%c%c%c%c%c%c%c%s%02d%c%02d%c%c%c%c%s%c%c%c%c%c%c%c%c%c%c%s%s%s%c%c%c%c",
                                    EFF_FRAME_RATE,0x00,EFF_HSV_AH,0x00,0xff,0xff,0xff,0xff,0xff,
                                    EFF_SCROLL_LEFT,"     ",h,':',m,EFF_DELAY_FRAMES,0x00,0x2c,EFF_CUSTOM_RC,0x02,
                                    EFF_RGB,0x00,0xc8,0x64,EFF_SCROLL_LEFT,"      ",t,'^',' ',EFF_DELAY_FRAMES,0x00,0xee,
                                    EFF_RGB,0xd3,0x54,0x00,EFF_SCROLL_LEFT,"      ",daysOfTheWeek[now.dayOfTheWeek()],' ',EFF_DELAY_FRAMES,0x00,0xee,
                                    EFF_RGB,0x00,0x80,0x80,EFF_SCROLL_LEFT,"     ",d,'-',mnth,EFF_DELAY_FRAMES,0x00,0xee,
                                    EFF_SCROLL_LEFT,"     ",
                                    EFF_HSV_AH,0x00,0xff,0xff,0xff,0xff,0xff,EFF_SCROLL_LEFT,EFF_FRAME_RATE,0x02," ",curMessage,"     ",EFF_FRAME_RATE,0x00,
                                    EFF_CUSTOM_RC,0x01);
                  
    if(++updatetemp > 10){                    // Update temperature every 10 sec, visual glitch
      //! t = RTC.getTemperature();               // +or- from this for calibration
      t=25;
      updatetemp = 0;
    }
  }

  if (newMessageAvailable){
    strcpy(curMessage, newMessage);           // Copy new message to display
    eepromWriteString(0, newMessage);         // Write String to EEPROM Address 0
    newMessageAvailable = false;
    Serial.println("new message received, updated EEPROM\n");
    delay(100);
    eepromWriteChar(BRT_BEGIN, BRIGHTNESS);    // White new brightness value
    FastLED.setBrightness(BRIGHTNESS);
    Serial.print("NeoMatrix Brightness set to ");
    Serial.println(BRIGHTNESS);
  }

  if (newTimeAvailable){
    int tY, tM, tD, th, tm;
    sscanf(newTime, "%2d.%2d.%4d %2d:%2d", &tD, &tM, &tY, &th, &tm); // extract received dd.mm.yyy hh:ss
    RTC.adjust(DateTime(tY, tM, tD, th, tm, 30));                    // rtc.adjust(DateTime(yyyy, m, d, h, m, s));
    newTimeAvailable = false;
    Serial.println(newTime);
    Serial.println("new time received, updated RTC");
  }

  rc = ScrollingMsg.UpdateText();
  if (rc == -1 || rc == 1)  // -1 means end of char array, 1 means end of msg because custom rc is received
  {
    ScrollingMsg.SetText((unsigned char *)szMesg, sizeof(szMesg) - 1);
  }
  else if (rc == 2)                               // EFFECT_CUSTOM_RC "\x02"
  {
    for (int j = 2; j < 10; j++)                  // want to start on even number to run drawline
    {
      if(j % 2 == 0){ //even
        StaticgMsg.SetText((unsigned char *)txtDateA, sizeof(txtDateA) - 1);
        StaticgMsg.UpdateText();
        leds.DrawLine(0, 0, 0, 7, CRGB(0, 0, 0)); // blank column 1, due to visual glitch text shifting << 1 pixel
      }
      else{
        StaticgMsg.SetText((unsigned char *)txtDateB, sizeof(txtDateB) - 1);
        StaticgMsg.UpdateText();
      }
      FastLED.show();
      delay(1000);
    }
  }
  else
  {
    FastLED.show();
  }
  delay(10);
  display.clearDisplay();

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(ssid);
  display.print("Pass: ");
  display.println(password);
  sprintf(szTime, "Time: %02d:%02d", h, m);
  display.println(szTime);
  sprintf(szDate, "Date: %02d/%02d/%02d", d, mnth, yr);
  display.println(szDate);
  display.display();
}