//******************************************************************************************
// SparkBox - BT pedal board for the Spark 40 amp - David Thompson 2022
// Supports four-switch pedals. Hold the 1st button down for 1s to switch
// between Preset mode (1 to 4) and Effect mode (Drive, Mod, Delay, Reverb).
// Hold down all four buttons to activate/deactivate the tuner.
// Added an expression pedal to modify the current selected effect or toggle an effect. 
//******************************************************************************************
// A bit of enchancements by copych 2022:
// *OLED display library changed to ThingPulse's, supporting most common DIY display types;
// *minor button routines improvements
// *minor interface improvements;
// *ESP32 deep/light sleep ability added;
// *LITTLEFS support added;
// *Preset banks functionality added;
// *WiFi support added with AP/WLAN config, stored in EEPROM (hold BUTTON1 while booting);
// *Web-based preset file manager added.
//******************************************************************************************

#define FORMAT_LITTLEFS_IF_FAILED true
#include "config.h"
#include "Banks.h"
extern tBankConfig bankConfig[NUM_BANKS+1];
extern String bankConfigFile;

#include "Spark.h"                  // Paul Hamshere's SparkIO library https://github.com/paulhamsh/Spark/tree/main/Spark
#include "SparkIO.h"                // "
#include "SparkComms.h"             // "
#include "font.h"                   // Custom fonts
#include "bitmaps.h"                // Custom bitmaps (icons)
#include "SparkPresets.h"           // Some hard-coded presets
#include "UI.h"                     // Any UI-related defines
//
#include "FS.h"
#include "LITTLEFS.h"
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
#define VERSION "V0.92 wifi" 


// Variables required to track spark state and also for communications generally
bool got_presets;
uint8_t display_preset_num;         // Referenced preset number on Spark
int i, j, p;
int count;                          // "
bool flash_GUI;                     // Flash GUI items if true
bool isTunerMode;                   // Tuner mode flag
bool scan_result = false;           // Connection attempt result
enum eMode_t {MODE_PRESETS, MODE_EFFECTS, MODE_BANKS, MODE_CONFIG, MODE_TUNER, MODE_BYPASS, MODE_MESSAGE, MODE_LEVEL};
eMode_t curMode = MODE_PRESETS;
eMode_t oldMode = MODE_PRESETS;
eMode_t returnMode = MODE_PRESETS;
eMode_t mainMode = MODE_PRESETS;
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
// UI **************************************************************************************
// array of frame drawing functions
FrameCallback frames[] = { frPresets, frEffects, frBanks, frConfig, frTuner, frBypass, frMessage, frLevel };
// number of frames in UI
const uint8_t frameCount = NUM_FRAMES;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { screenOverlay };
const uint8_t overlaysCount = 1;

// SETUP() *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void setup() {
  setCpuFrequencyMhz(180);                      // Hopefully this will let the battery last a bit longer
  #ifdef DEBUG_ON
    Serial.begin(115200);                       // Start serial debug console monitoring via ESP32
    while (!Serial);
  #endif
  display_preset_num = 0;
  
  // Check FS
  if(!LITTLEFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
    Serial.println("LITTLEFS Mount Failed");
    return;
  } else {
    DEBUG("LITTLEFS Mount Completed");    
  }
  // A bit of cheating here, we only check one last folder 
   if( !LITTLEFS.exists("/bank_" + lz(NUM_BANKS, 3)) ) {
    createFolders();
  } else {
    DEBUG("Bank folders exist");
  }

  // First launch init for bankConfig
  if (bankConfig[1].active_chan == 255) {                        // It is 255 (-1) by default and 0/1 after initializing
    for (int i = 0 ; i <= NUM_BANKS; i++) {
      bankConfig[i].active_chan = 0;
      strlcpy(bankConfig[i].bank_name , ("Bank " + lz(i, 3) ).c_str(), sizeof(bankConfig[i].bank_name));
    }
  }

  loadConfiguration(bankConfigFile, bankConfig);                // Load, or init w/default values
  strlcpy(bankConfig[0].bank_name , "SPARK", sizeof("SPARK"));  // Always be by that name
  saveConfiguration(bankConfigFile, bankConfig);                // Save to FS

  
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
 // ui.enableAutoTransition();
  // Initialising the UI will init the display too.
  ui.init();
  //oled.init();
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
  EEPROM.end();

  DEBUG("Connecting...");

  while (!scan_result && attempt_count < BT_MAX_ATTEMPTS) {     // Trying to connect
    attempt_count++;
  
    readBattery();
    
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

void loop() {
  static ulong tBefore, tAfter;
  if(spark_state == SPARK_SYNCED){
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

  // Check if a message needs to be processed
  if (update_spark_state()) {
    if (cmdsub == 0x0170) {
      String sLicense = "";            
      for (int i=0; i < 64; i++) {
        if (license_key[i] < 16) sLicense += "0";
        sLicense += (String)(license_key[i], HEX);
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
  /*
  tAfter = micros();
  loopTime = loopTime*99/100+ tAfter-tBefore;
  tBefore = tAfter;
  */
} // loop()
