//******************************************************************************************
// SparkBox - BLE pedal board for the Spark 40 amp - David Thompson June 2022
// Supports four-switch pedals. Added an expression pedal to modify the current selected effect or toggle an effect. 
// Long press (more than 1s) BUTTON 1 to switch between Effect mode and Preset mode
// Long press BUTTON 3 to adjust current effect parameters: use BUTTONS 2 and 4 to decrement/increment, 
// BUTTON 1 to cycle thru parameters and long press BUTTON 3 to save your edits back to the amp.
// Guitar Tuner: Long press BUTTONS 1 and 2 simultaneously, or turn it ON from the app or on the amp.
// Bypass mode, invoked by long pressing BUTTONS 3 and 4 simultaneously, allows adjusting effect levels to match your raw pickup output.
// Holding BUTTON 1 during boot will switch the pedal to WiFi mode.
//
//******************************************************************************************
// Extra enchancements by copych 2022:
// *OLED display library changed to ThingPulse's, supporting most common DIY display types;
// *minor button routines improvements
// *minor interface improvements;
// *ESP32 deep/light sleep ability added;
// *LittleFS support added;
// *Preset banks functionality added;
// *WiFi support added with AP/WLAN config, stored in EEPROM (hold BUTTON1 while booting);
// *Web-based preset file manager added.
//******************************************************************************************
//
// #define CONFIG_LITTLEFS_SPIFFS_COMPAT 1
#define CONFIG_LITTLEFS_HUMAN_READABLE 1
// #define CONFIG_LITTLEFS_FOR_IDF_3_2 1
#define FORMAT_LITTLEFS_IF_FAILED true
#define CONFIG_LITTLEFS_FLUSH_FILE_EVERY_WRITE 1
#include "config.h"
#include "Banks.h"
extern tBankConfig bankConfig[NUM_BANKS+1];
extern String bankConfigFile;
//
#include "Spark.h"                  // Paul Hamshere's SparkIO library https://github.com/paulhamsh/Spark/tree/main/Spark
#include "SparkIO.h"                // "
#include "SparkComms.h"             // "
#include "font.h"                   // Custom fonts
#include "bitmaps.h"                // Custom bitmaps (icons)
#include "SparkPresets.h"           // Some hard-coded presets
#include "UI.h"                     // Any UI-related defines
//
#include "FS.h"
#include "LittleFS.h"
#include "BluetoothSerial.h"
#define ARDUINOJSON_USE_DOUBLE 1
#include "ArduinoJson.h"            // Should be installed already https://github.com/bblanchon/ArduinoJson
#include "driver/rtc_io.h"
//
#include <WiFi.h>
#include <EEPROM.h>
#include "WebServer.h"
#include "SimplePortal.h"
#include "RequestHandlersImpl.h"

//******************************************************************************************
#define PGM_NAME "SparkBox"
#define VERSION "V0.99" 

extern eMode_t curMode;
extern eMode_t oldMode;
extern eMode_t returnMode;
extern eMode_t mainMode;
extern tPedalCfg pedalCfg;

// Variables required to track spark state and also for communications generally
bool got_presets = false;
uint8_t display_preset_num;         // Referenced preset number on Spark
int8_t active_led_num = -1;
int i, j, p;
int count;                          // "
bool flash_GUI;                     // Flash GUI items if true
bool isTunerMode;                   // Tuner mode flag
bool scan_result = false;           // Connection attempt result
enum ePresets_t {HW_PRESET_0, HW_PRESET_1, HW_PRESET_2, HW_PRESET_3, TMP_PRESET, CUR_EDITING, TMP_PRESET_ADDR=0x007f};
enum eEffects_t {FX_GATE, FX_COMP, FX_DRIVE, FX_AMP, FX_MOD, FX_DELAY, FX_REVERB};
#ifdef ACTIVE_HIGH
  uint8_t logicON = HIGH;
  uint8_t logicOFF = LOW;
#else
  uint8_t logicON = LOW;
  uint8_t logicOFF = HIGH;
#endif

// interrupts
hw_timer_t * timer = NULL;          // Timer variables
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool isTimeout = false; // Update battery icon flag

// SWITCHES Init ===========================================================================
typedef struct {
  const uint8_t pin;
  const String fxLabel;
  uint8_t fxSlotNumber;   // [0-6] number in fx chain
  bool fxOnOff; // Effect onOff
} s_switches ;

s_switches SWITCHES[NUM_SWITCHES] = {
  {switchPins[0], "DRIVE", FX_DRIVE, false},
  {switchPins[1], "MOD", FX_MOD, false},
  {switchPins[2], "DELAY", FX_DELAY, false},
  {switchPins[3], "REVERB", FX_REVERB, false},
};

//******************************************************************************************

// Timer interrupt handler
void IRAM_ATTR onTime() {
   portENTER_CRITICAL_ISR(&timerMux);
    isTimeout = true;
   // isRSSIupdate = true;
   portEXIT_CRITICAL_ISR(&timerMux);
}
// array of frame drawing functions
FrameCallback frames[] = { frPresets, frEffects, frBanks, frConfig, frTuner, frBypass, frMessage, frLevel };
// number of frames in UI
const uint8_t frameCount = NUM_FRAMES;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { screenOverlay };
const uint8_t overlaysCount = 1;

//******************************************************************************************

void setup() {
  
  time_to_sleep = millis() + (1000*60); // Preset timeout 
  setCpuFrequencyMhz(180);                      // Hopefully this will let the battery last a bit longer
  #ifdef DEBUG_ON
    Serial.begin(115200);                       // Start serial debug console monitoring via ESP32
    while (!Serial);
  #endif
  display_preset_num = 0;
  
  // Check FS
  if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
    DEBUG("LittleFS Mount Failed");
    return;
  } else {
    DEBUG("LittleFS Mount Completed");    
  }

  // This will create bank folders if needed
  createFolders(); 

  // First launch init for bankConfig
  if (bankConfig[1].start_chan == 255) {                        // It is 255 (-1) by default and 0/1 after initializing
    for (int i = 0; i <= NUM_BANKS; i++) {
      bankConfig[i].start_chan = 0;
      strlcpy(bankConfig[i].bank_name , ("Bank " + lz(i, 3) ).c_str(), sizeof(bankConfig[i].bank_name));
    }
  }

  loadConfiguration(bankConfigFile, bankConfig);                // Load, or init w/default values
  strlcpy(bankConfig[0].bank_name , "SPARK", sizeof("SPARK"));  // Always be by that name
  saveConfiguration(bankConfigFile, bankConfig);                // Save to FS

  // Setup GPIOs for led if those are defined
  setup_leds();
  
  // Debug - On my Heltec module, leaving this unconnected pin hanging causes
  // a display issue where the screen dims, returning if touched.
  pinMode(21,OUTPUT);
  
  // Manually toggle the /RST pin to add Heltec module functionality
  // but without the Heltec library
  pinMode(16,OUTPUT);
  digitalWrite(16, LOW);
  delay(50);
  digitalWrite(16, HIGH);

  // Initialize device OLED display, and flip screen, as OLED library starts upside-down
  ui.setTargetFPS(35);
  ui.disableAllIndicators();
  ui.setFrames(frames, frameCount);
  ui.setFrameAnimation(SLIDE_UP);
  ui.setTimePerTransition(TRANSITION_TIME);
  ui.setOverlays(overlays, overlaysCount);
  ui.disableAutoTransition();

  // Initialising the UI will init the display too.
  ui.init();
  oled.flipScreenVertically();  // This allows to flip all the UI upside down depending on your h/w components palacement
  ESP_on();                     // Show startup animation
  
  // Set pushbutton inputs to pull-downs or pull-ups depending on h/w variant
  for (i = 0; i < NUM_SWITCHES; i++) {    
    #ifdef ACTIVE_HIGH
      pinMode(switchPins[i], INPUT_PULLDOWN);
    #else
      pinMode(switchPins[i], INPUT_PULLUP);
    #endif
  }

  // Accumulate battery voltage measurements to average the result
  for(i=0; i<VBAT_NUM; ++i) {
    delay(10);                                // Warm analog delay
    readBattery();
  }

  timer = timerBegin(0, 80, true);            // Setup timer
  timerAttachInterrupt(timer, &onTime, true); // Attach to our handler
  timerAlarmWrite(timer, 500000, true);       // 500ms, autoreload
  timerAlarmEnable(timer);                    // Start timer

  // Show welcome message
  oled.clear();
  oled.setFont(BIG_FONT);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(X1, Y3, PGM_NAME);
  oled.setFont(MEDIUM_FONT);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(X1, Y4, VERSION);
  oled.setFont(SMALL_FONT);
  mainIcons();
  oled.display();
  delay(1000);                                // Wait for splash screen
  
  EEPROM.begin(512);
  EEPROM.get(0, portalCfg); // try to get our stored values
  if ((digitalRead(switchPins[0]) == logicON)) {
    inWifi = true;
    filemanagerRun();
    inWifi = false;
  }
  EEPROM.get(255, pedalCfg);
  DEB("pedalCfg.active_bank = ");
  DEBUG(pedalCfg.active_bank);
  EEPROM.end();
  
  DEBUG("Connecting...");
  while (!scan_result && attempt_count < BT_MAX_ATTEMPTS) {     // Trying to connect
    attempt_count++;
    //
    readBattery();
    // Process user input
    doPushButtons();
    // Show connection message
    oled.clear();
    oled.setFont(BIG_FONT);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    oled.drawString(X1, Y3, "Connecting ");
    oled.setFont(MEDIUM_FONT);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    oled.drawString(X1, Y4, "Please wait " + (String)(BT_MAX_ATTEMPTS - attempt_count + 1) + "...");
    mainIcons();
    oled.display();
    DEBUG("Scanning and connecting");
    scan_result = spark_state_tracker_start();
  }
  attempt_count = 0;

  if (!scan_result) {
#ifndef NOSLEEP
    ESP_off();
    esp_restart(); // if it sleeps deep, then we never get here, but if light, then we need to restart the unit
#else
    esp_restart();
#endif
  }
  // Proceed if connected
  ui.switchToFrame(curMode);
  time_to_sleep = millis() + (BT_MAX_ATTEMPTS * MILLIS_PER_ATTEMPT); // Preset timeout 
}

//******************************************************************************************

void loop() {
  static ulong tBefore, tAfter;
  if(spark_state == SPARK_SYNCED){
    if (!got_presets) {
      // If it's the first run with banks, then save current presets to Bank_000
      File root = LittleFS.open("/bank_000/");
      if (pedalCfg.active_bank == 255 || !root.openNextFile() ) {
        localBankNum = 0;
        pedalCfg.active_bank = localBankNum;
        for (int i = 0; i < 4; i++) {
          DEBUG("Saving h/w preset " + String(i));
          savePresetToFile(presets[i], "/bank_000/hw_" + String(i) + ".json");
        }
      }
      got_presets = true;
    }
    int remainingTimeBudget = ui.update();
  }
#ifdef EXPRESSION_PEDAL
  // Only handle the pedal if the app is connected
  if (conn_status[APP]){
    doExpressionPedal();
  }
#endif //EXPRESSION_PEDAL

  // Process user input
  doPushButtons();

  // Process leds
  doLeds();

  // Check if a message needs to be processed
  if (update_spark_state()) {
    if (cmdsub == 0x0170) {
      String sLicense = "";            
      for (int i=0; i < 64; i++) {
        if (license_key[i] < 16) sLicense += "0";
        sLicense += String(license_key[i], HEX);
      }
      DEBUG(sLicense);
      change_hardware_preset(display_preset_num); // Refresh app preset when first connected
    }

    // Work out if the user is touching a switch or a knob
    if (cmdsub == 0x0115 || cmdsub == 0x0315){
      expression_target = true;
    }
    else if (cmdsub == 0x0104 || cmdsub == 0x0337 || cmdsub == 0x0106){
      expression_target = false;
    }
    else {
      expression_target = true; 
    }
    
    // do your own checks and processing here    
    if (cmdsub == 0x0337 || cmdsub == 0x0104) {
      DEBUG("Change parameter ");
      int fxSlot = fxNumByName(msg.str1).fxSlot;
      presets[CUR_EDITING].effects[fxSlot].Parameters[msg.param1] = msg.val;
      fxCaption = spark_knobs[fxSlot][msg.param1];
      if (fxSlot==5 && msg.param1==4){
        //suppress the message "BPM=10.0"
      } else {
        level = msg.val * MAX_LEVEL;
        tempFrame(MODE_LEVEL, curMode, FRAME_TIMEOUT);
      }
    }

    if (cmdsub == 0x0363) {
      DEBUG("Tap Tempo " + (String)(msg.val));
      level = msg.val * 10;
      tempFrame(MODE_LEVEL, curMode, 1000);
    }
  }

  // Refresh screen
  refreshUI();

} // loop()
