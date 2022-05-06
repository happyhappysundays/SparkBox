/*
    Простой менеджер WiFi для esp8266 для задания логина-пароля WiFi и режима работы
    GitHub: https://github.com/GyverLibs/SimplePortal
    
    AlexGyver, alex@alexgyver.ru
    https://alexgyver.ru/
    MIT License

    Версии:
    v1.0
    v1.1 - совместимость с ESP32
*/

#ifndef _SimplePortal_h
#define _SimplePortal_h

#include <Arduino.h>
#include "config.h"
#include "ESPxWebFlMgr.h"
#include <DNSServer.h>

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
#else
  #include <WiFi.h>
  #include "WebServer.h"
#endif

#ifdef SSD1306
  #include "SSD1306Wire.h"          // https://github.com/ThingPulse/esp8266-oled-ssd1306
#endif
#ifdef SH1106
  #include "SH1106Wire.h"
#endif
#include "OLEDDisplayUi.h"
#include "ESPxWebFlMgr.h"

#ifdef SSD1306
  extern SSD1306Wire oled;
#endif
#ifdef SH1106
  extern SH1106Wire oled;
#endif
extern OLEDDisplayUi ui;

extern ESPxWebFlMgr filemgr;

extern bool wifi_connected;

extern void refreshUI();
extern void doPushButtons();
extern void showMessage(const String &capText, const String &text1, const String &text2,  const ulong msTimeout) ;

#define SP_ERROR 0
#define SP_SUBMIT 1
#define SP_SWITCH_AP 2
#define SP_SWITCH_LOCAL 3
#define SP_EXIT 4
#define SP_TIMEOUT 5

struct PortalCfg {
  char SSID[32] = "";
  char pass[32] = "";
  uint8_t mode = WIFI_AP;    // (1 WIFI_STA, 2 WIFI_AP)
  bool succeed = false;
  bool tried = false;
};

extern PortalCfg portalCfg;

void portalStart();     // startup the portal
void portalStop();      // stop the portal
bool portalTick();      // to call in a loop
void portalRun(uint32_t prd = 60000);   // blocking call
byte portalStatus();    // status: 1 connect, 2 ap, 3 local, 4 exit, 5 timeout
void listNetworks();
void SP_handleConnect();
void SP_handleAP();
void SP_handleLocal();
void SP_handleExit();
#endif