//******************************************************************************************
// SparkBox - BT pedal board for the Spark 40 amp - David Thompson 2022
// Supports four-switch pedals. Hold any of the effect buttons down for 1s to switch
// between Preset mode (1 to 4) and Effect mode (Drive, Mod, Delay, Reverb).
// Hold down all four buttons to activate/deactivate the tuner.
// Added an expression pedal to modify the current selected effect or toggle an effect. 
//******************************************************************************************
//
// Battery charge function defines. Please uncomment just one.
// You have no mods to monitor the battery, so it will show empty
//#define BATT_CHECK_0
//
// You are monitoring the battery via a 2:1 10k/10k resistive divider to GPIO23
// You can see an accurate representation of the remaining battery charge and a kinda-sorta
// indicator of when the battery is charging. Maybe.
//#define BATT_CHECK_1
//
// You have the battery monitor mod described above AND you have a connection between the 
// CHRG pin of the charger chip and GPIO 33. Go you! Now you have a guaranteed charge indicator too.
#define BATT_CHECK_2
//
// Expression pedal define. Comment this out if you DO NOT have the expression pedal mod
//#define EXPRESSION_PEDAL
//
// Dump preset define. Comment out if you'd prefer to not see so much text output
//#define DUMP_ON
//
// Uncomment for better Bluetooth compatibility with Android devices
#define CLASSIC
//
// Uncomment when using a Heltec module as their implementation doesn't support setMTU()
#define HELTEC_WIFI
//
// Choose and uncomment the type of OLED display used: 0.96" SSD1306 or 1.3" SH1106 
#define SSD1306
//#define SH1106
//
// Uncomment if two-colour OLED screens are used. Offsets some text and alternate tuner
#define TWOCOLOUR
//
// Uncomment if you don't want the pedal to sleep to save power, this also prevents low-battery sleep
//#define NOSLEEP
//
// When adjusting the level of effects, always start with Master level settings. Comment this line out if you like it to remember your last choice
#define RETURN_TO_MASTER
//
//******************************************************************************************
#ifdef SSD1306
#include "SSD1306Wire.h"            // https://github.com/ThingPulse/esp8266-oled-ssd1306
#endif
#ifdef SH1106
#include "SH1106Wire.h"
#endif
#include "OLEDDisplayUi.h"          // Include the UI lib
#include "FS.h"
#include "SPIFFS.h"
#include "BluetoothSerial.h"
#include "Spark.h"                  // Paul Hamshere's SparkIO library https://github.com/paulhamsh/Spark/tree/main/Spark
#include "SparkIO.h"                // "
#include "SparkComms.h"             // "
#include "font.h"                   // Custom large font
#include "bitmaps.h"                // Custom bitmaps (icons)
#include "SparkPresets.h"           // Custom bitmaps (icons)
#include "UI.h"                     // Any UI-related defines
#define ARDUINOJSON_USE_DOUBLE 1
#include "ArduinoJson.h"            // Should be installed already https://github.com/bblanchon/ArduinoJson
#include "driver/rtc_io.h"
//
//******************************************************************************************

#define PGM_NAME "SparkBox"
#define VERSION "V0.83 alpha" 

#ifdef SSD1306
SSD1306Wire oled(0x3c, 4, 15);        // Default OLED Screen Definitions - ADDRESS, SDA, SCL
#endif
#ifdef SH1106
SH1106Wire oled(0x3c, 4, 15);      // or this line if you are using SSH1106
#endif
OLEDDisplayUi ui(&oled);              // Create UI instance for the display (slightly advanced frame based GUI)

char str[STR_LEN];                    // Used for processing Spark commands from amp
char param_str[50]; //debug
int param = -1;
float val = 0.0;
bool expression_target = false;       // False = parameter change, true = effect on/off
bool effectstate = false;             // Current state of the effect controller by the expression pedal
bool setting_modified = false;        // Flag that user has modifed a setting

// Variables required to track spark state and also for communications generally
bool got_presets;
uint8_t display_preset_num;           // Referenced preset number on Spark
int i, j, p;
int count;                            // "
bool flash_GUI;                       // Flash GUI items if true
bool isTunerMode;                     // Tuner mode flag
bool scan_result = false;             // Connection attempt result
enum eMode_t {MODE_PRESETS, MODE_EFFECTS, MODE_SCENES, MODE_TUNER, MODE_BYPASS, MODE_MESSAGE, MODE_LEVEL};
eMode_t curMode = MODE_PRESETS;
eMode_t oldMode = MODE_PRESETS;
eMode_t returnMode = MODE_PRESETS;
enum ePresets_t {HW_PRESET_0, HW_PRESET_1, HW_PRESET_2, HW_PRESET_3, TMP_PRESET, CUR_EDITING, TMP_PRESET_ADDR=0x007f};
enum eEffects_t {FX_GATE, FX_COMP, FX_DRIVE, FX_AMP, FX_MOD, FX_DELAY, FX_REVERB};

hw_timer_t * timer = NULL;            // Timer variables
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile boolean isTimeout = false;   // Update battery icon flag
// volatile boolean isRSSIupdate = false;// Update RSSI display flag

// SWITCHES Init ===========================================================================
typedef struct {
  const uint8_t pin;
  const String fxLabel;   // Sorry for using Strings here
  uint8_t fxSlotNumber;   // [0-6] number in fx chain
  bool fxOnOff; // Effect onOff
} s_switches ;

s_switches SWITCHES[NUM_SWITCHES] = {
  {sw_pin[0], "DRIVE", FX_DRIVE, false},
  {sw_pin[1], "MOD", FX_MOD, false},
  {sw_pin[2], "DELAY", FX_DELAY, false},
  {sw_pin[3], "REVERB", FX_REVERB, false},
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
FrameCallback frames[] = { frPresets, frEffects, frScenes, frTuner, frBypass, frMessage, frLevel };
// number of frames in UI
int frameCount = 7;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { screenOverlay };
int overlaysCount = 1;

// SETUP() *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void setup() {
  Serial.begin(115200);                       // Start serial debug console monitoring via ESP32
  while (!Serial);

  display_preset_num = 0;
  int tmp_batt;

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
  ui.setTargetFPS(30);
  ui.disableAllIndicators();
  ui.setFrameAnimation(SLIDE_LEFT);
  ui.setFrames(frames, frameCount);
  ui.setTimePerTransition(TRANSITION_TIME);
  ui.setOverlays(overlays, overlaysCount);
  ui.disableAutoTransition();
  // Initialising the UI will init the display too.
  ui.init();
  //oled.init();
  oled.flipScreenVertically();
  ESP_on();
  
  // Set pushbutton inputs to pull-downs
  for (i = 0; i < NUM_SWITCHES; i++) {
    pinMode(sw_pin[i], INPUT_PULLDOWN);
  }

  // Avg battery voltage
  for(i=0; i<VBAT_NUM; ++i) {
    delay(10);
    readBattery();
  }

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

  DEBUG("Connecting...");  

  timer = timerBegin(0, 80, true);            // Setup timer
  timerAttachInterrupt(timer, &onTime, true); // Attach to our handler
  timerAlarmWrite(timer, 500000, true);       // 500ms, autoreload
  timerAlarmEnable(timer);                    // Start timer

  while (!scan_result && attempt_count < MAX_ATTEMPTS) {     // Trying to connect
    attempt_count++;
  
    readBattery();
    
    // Show connection message
    oled.clear();
    oled.setFont(BIG_FONT);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    oled.drawString(X1, Y3, "Connecting ");
    oled.setFont(MEDIUM_FONT);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    oled.drawString(X1, Y4, "Please wait " + String(MAX_ATTEMPTS - attempt_count + 1) + "...");
    mainIcons();
    oled.display();    
    DEBUG("Scanning and connecting");
    scan_result = spark_state_tracker_start();
  }
  
  attempt_count = 0;

  if (!scan_result) {
#ifndef NOSLEEP
    ESP_off();
#else
    esp_restart();
#endif
  }
  // Proceed if connected
  ui.switchToFrame(curMode);
  time_to_sleep = millis() + (MAX_ATTEMPTS * MILLIS_PER_ATTEMPT); // Preset timeout 
}

void loop() {
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
    if (cmdsub == 0x170) {
      Serial.println();
      for (int i=0; i < 64; i++) {
        if (license_key[i] < 16)
          Serial.print("0");
        Serial.print(license_key[i], HEX);
      }
      Serial.println();
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
      DEBUG("Tap Tempo " + String(msg.val));
      level = msg.val * 10;
      tempFrame(MODE_LEVEL, curMode, 1000);
    }   
   
    //isOLEDUpdate = true;        // Flag screen update
  }

  // Refresh screen
  refreshUI();
  
} // loop()}
