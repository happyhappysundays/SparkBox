// Defines
#ifndef _UI_h
#define _UI_h

#include "config.h"
#include "Banks.h"
#include "SparkStructures.h"

#ifdef SSD1306
  #include "SSD1306Wire.h"          // https://github.com/ThingPulse/esp8266-oled-ssd1306
#endif
#ifdef SH1106
  #include "SH1106Wire.h"
#endif
#include "OLEDDisplayUi.h"          // Include the UI lib

#ifdef SSD1306
  SSD1306Wire oled(0x3c, 4, 15);    // Default OLED Screen Definitions - ADDRESS, SDA, SCL
#endif
#ifdef SH1106
  SH1106Wire oled(0x3c, 4, 15);     // or this line if you are using SSH1106
#endif
OLEDDisplayUi ui(&oled);            // Create UI instance for the display (slightly advanced frame based GUI)

#include "ESPxWebFlMgr.h"

#define NUM_FRAMES 8      // How many UI frames we have
#define CYCLE_MODES 2     // How many pedal modes (first N frames) switch in cycle when button 1 is long-pressed
#define VBAT_AIN 32       // Vbat sense GPIO (2:1 divider)

#ifdef ALTERNATE_CHARGE_AIN
  #define CHRG_AIN 35       // Charge pin sense (10k pull-up)         // PH EDIT
#else
  #define CHRG_AIN 33       // Charge pin sense GPIO (10k pull-up)
#endif

#define EXP_AIN 34        // Expression pedal input GPIO (a pot (usually 10-50kOhm) connected via an additional 1kOhm resistor to 3V3)
#define BT_MAX_ATTEMPTS 5 // Bluetooth (re-)connection attempts before going to sleep
#define MILLIS_PER_ATTEMPT 6000 // milliseconds per connection attempts, this is used when reconnecting, not quite as expected though
#define WIFI_MAX_ATTEMPTS 10 //WiFi connection attempts before giving up

#define HARD_PRESETS 24   // number of hard-coded presets in SparkPresets.h
#define HW_PRESETS 5      // 4 hardware presets + 1 temporary in amp presets
//
#define ADC_COEFF 573     // Multiplier to get ADC value out of voltage

#define BATTERY_0 3.69
#define BATTERY_20 3.73
#define BATTERY_40 3.8
#define BATTERY_87 4.1
#define BATTERY_100 4.17  // Theoretical max LiPo voltage is 4.20 but under under the smallest load I see 4.17

#define BATTERY_OFF BATTERY_0  // Voltage when we decide to force the pedal to switch to stand-by mode
#define BATTERY_FUDGE 3.70  // Fudge factor to speed up initial averaging calcs

#define GAUGE_0   5       // Battery % when the meter is showing 0%
#define GAUGE_100 95      // Battery % when the meter is showing 100%
#define CHRG_LOW 3.47     // Charging voltage detection
#define VBAT_NUM 10       // Number of vbat readings to average

//GUI settings
#define TRANSITION_TIME 350 // Frame transition time, ms
#define BATT_WIDTH 26        // Battery icon width
#define CONN_ICON_WIDTH 11  // Connection status icons width
#define FX_ICON_WIDTH 18    // Exxects icons width
#define STATUS_HEIGHT 16    // Status line height
#define FRAME_TIMEOUT 3000  // (ms) to return to main UI from temporary UI frame 

#ifdef TWOCOLOUR
// Two-colour displays
#define X1 64 // Please wait, Version, Connecting, SparkBox, Reconnecting
#define Y1 22
#define Y2 47
#define Y3 16
#define Y4 41
#define Y5 16
#define tuner_share 5
#define note_y 0
#else

// Single colour displays
#define X1 64 // Please wait, Version, Connecting, SparkBox, Reconnecting
#define Y1 16
#define Y2 42
#define Y3 16 // was 13
#define Y4 41 // was 36
#define Y5 0
#define tuner_share 4
#define note_y 16
#endif

// font aliases for quick modding
//#define SMALL_FONT Lato_Hairline_11
#define SMALL_FONT ArialMod_Plain_10 // It has some mods, making it look a bit better
//#define SMALL_FONT ArialMT_Plain_10
#define MEDIUM_FONT ArialMT_Plain_16
#define BIG_FONT ArialMT_Plain_24
//#define HUGE_FONT Roboto_Mono_Bold_52
#define HUGE_FONT Roboto_Mono_Medium_52

// Globals
int vbat_result = 0;                            // For battery monitoring
int express_ring_count = 0;
int express_ring_sum = 0;
int express_result = 0;                         // For expression pedal monitoring
int old_exp_result = 0;
float effect_volume = 0.0;
int chrg_result = 0;                            // For charge state monitoring
int attempt_count = 0;                          // Connection attempts counter
int pendingBankNum = -1;
int localBankNum = 0;
String bankName = "";                           // Name of the 4-presets bank to display in the BANKS mode
uint8_t localPresetNum;                             // actual slot number on the pedal
uint8_t remotePresetNum;                            // preset slot number on the amp
int8_t pendingPresetNum = -1;
unsigned long idleBankCounter = 0;
const uint8_t RTC_pins[]{0,2,4,12,13,14,15,25,26,27,32,33,34,35,36,37,38,39}; // These are RTC enabled GPIOs of ESP32, this is hardware, and if you choose to connect buttons to at least one of this list, deep sleep will be enabled
bool sw_RTC[NUM_SWITCHES];
int RTC_present = 0;                            // Number of RTC pins present in the config
int RTC_1st = -1;
// BUTTONS SECTION ====================================================================
const unsigned long longPressThreshold = 800;   // the threshold (in milliseconds) before a long press is detected
const unsigned long autoFireDelay = 100;        // the threshold (in milliseconds) between clicks if autofire is enabled
bool autoFireEnabled = false;                   // should the buttons generate continious clicks when pressed longer than a longPressThreshold
const unsigned long debounceThreshold = 20;     // the threshold (in milliseconds) for a button press to be confirmed (i.e. not "noise")
uint8_t ActiveFlags = 0;                        // Write buttons states to one binary mask; it's global so that we can check it anywhere

// UI SECTION =========================================================================
int scroller = 0;                               // Variable to keep scrolling offset 
String msgCaption = "", msgText = "", msgText1 = "";
bool tempUI = false;                            // If we are in the temporary frame which returns after a given timeout
boolean isOLEDUpdate;                           // Flag OLED needs refresh
unsigned long actual_timeout = FRAME_TIMEOUT;
uint64_t timeToGoBack = 0, time_to_sleep = 0;

// Presets section
bool fxState[] = {false,false,false,false,false,false,false}; // Array to store FX's on/off state before total bypass is ON

// Type for Coordinates of Fx Params
typedef struct {
  int fxSlot;
  int fxNumber;
} s_fx_coords;


char str[STR_LEN];                  // Used for processing Spark commands from amp
char param_str[50];                 // 
int param = -1;
float val = 0.0;
bool expression_target = false;     // False = parameter change, true = effect on/off
bool effectstate = false;           // Current state of the effect controller by the expression pedal
bool setting_modified = false;      // Flag that user has modifed a setting
bool inWifi = false;
bool wifi_connected = false;
String bankPresetFiles[4];
ulong loopTime;                     // millis per loop (performance measure)


// Yeah, we need to maintain this list (((

const char spark_noisegates[][STR_LEN+1]{"bias.noisegate"};
const char spark_compressors[][STR_LEN+1]{"BassComp","BBEOpticalComp","BlueComp","Compressor","JH.Vox846","LA2AComp"};
const char spark_drives[][STR_LEN+1]{"BassBigMuff","Booster","DistortionTS9","Fuzz","GuitarMuff","KlonCentaurSilver","MaestroBassmaster",
  "Overdrive","ProCoRat","SABdriver","TrebleBooster"};
const char spark_amps[][STR_LEN+1]{"6505Plus","94MatchDCV2","AC Boost","Acoustic","AcousticAmpV2","ADClean","AmericanHighGain","Bassman",
  "BE101","BluesJrTweed","Bogner","Checkmate","Deluxe65","EVH","FatAcousticV2","FlatAcoustic","GK800","Hammer500","Invader","JH.JTM45",
  "JH.SuperLead100","ODS50CN","OrangeAD30","OverDrivenJM45","OverDrivenLuxVerb","Plexi","Rectifier","RolandJC120","SLO100","Sunny3000",
  "SwitchAxeLead","Twin","TwoStoneSP50","W600","YJM100"};
const char spark_modulations[][STR_LEN+1]{"ChorusAnalog","Cloner","Flanger","GuitarEQ6","JH.VoodooVibeJr","MiniVibe","Phaser",
  "Tremolator","Tremolo","TremoloSquare","UniVibe","Vibrato01"};
const char spark_delays[][STR_LEN+1]{"DelayEchoFilt","DelayMono","DelayMultiHead","DelayRe201","DelayReverse","VintageDelay"};
const char spark_reverbs[][STR_LEN+1]{"bias.reverb"};

// knob, fx, param
const char spark_knobs[7][5][12] {
  {"THRESHOLD","DECAY","N.GT-2","N.GT-3","N.GT-4"},         //noise gate
  {"COMP-0","COMP-1","COMP-2","COMP-3","COMP-4"},           //compressor
  {"DRIVE","DRV-1","DRV-2","DRV-3","DRV-4"},                //drive
  {"GAIN","TREBLE","MID","BASS","MASTER"},                  //amp
  {"MODULATION","MOD-INTENS.","MOD-2","MOD-3","MOD-4"},     //modulation
  {"DELAY","DLY-1","DLY-2","DLY-3","BPM"},                  //delay
  {"REVERB","REV-1","REV-2","REV-3","REV-4"}                //reverb
};

const char hints[NUM_FRAMES][NUM_SWITCHES][10]{  // [MODE],[ICON],[char_len]
  {"", "", "", ""},                   // MODE_PRESETS
  {"", "", "", ""},                   // MODE_EFFECTS
  {"rcv", "-", "send", "+"},          // MODE_BANKS
  {"rcv", "-", "send", "+"},          // MODE_CONFIG
  {"", "", "", ""},                   // MODE_TUNER
  {"", "", "", ""},                   // MODE_BYPASS  
  {"yes", "no", "", ""},              // MODE_MESSAGE  
  {"sel.", "-", "save", "+"},         // MODE_LEVEL
};

const s_fx_coords knobs_order[] = {
  {3,0}, //gain
  {3,3}, //bass
  {3,2}, //mid
  {3,1}, //treble
  {3,4}, //master
  {4,0}, //modulation
  {5,0}, //delay
  {6,0}, //reverb
  {2,0}  //drive
};

const uint8_t knobs_number = 9;

const uint8_t MAX_LEVEL = 100; // maximum level of effect, actual value in UI is level divided by 100
int curKnob=4, curFx=3, curParam=4;
int level = 0;
String fxCaption=spark_knobs[curFx][curParam];  // Effect caption for displaying at the top of the screen when adjusting

// forward declarations
void doPushButtons();
void refreshUI();
void showMessage(const String &capText, const String &text1, const String &text2,  const ulong msTimeout) ;
void loadConfiguration(const String filename, tBankConfig (&conf)[NUM_BANKS+1]);
#endif
