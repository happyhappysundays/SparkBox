// Defines
#define DEBUG_ON          // Comment this line out for a production build

#ifdef DEBUG_ON
#define DEBUG(...) Serial.println(__VA_ARGS__) // found this hint with __VA_ARGS__ on the web, it accepts different sets of arguments /Copych
#else
#define DEBUG(...)
#endif

#define NUM_MODES 2       // How many pedal modes will switch in cycle when button 1 is long-pressed
#define VBAT_AIN 32       // Vbat sense (2:1 divider)
#define CHRG_AIN 33       // Charge pin sense (10k pull-up)
#define EXP_AIN 34        // Expression pedal input (3V3)
#define MAX_ATTEMPTS 5    // (Re-)Connection attempts before going to sleep
#define MILLIS_PER_ATTEMPT 6000 // milliseconds per connection attempts, this is used when reconnecting, not quite as expected though

#define HARD_PRESETS 24   // number of hard-coded presets in SparkPresets.h
//
#define ADC_COEFF 576.66  // Multiplier to get ADC value out of voltage
#define BATTERY_LOW 3.30  // Noise floor of 0%, if we measure such voltage it's better to go to sleep 
#define BATTERY_100 4.20  // Noise floor of 100%
#define BATTERY_CHRG 4.26 // Totally empirical, adjust as required. Currently 4.26V-ish
#define CHRG_LOW 3.47     // Charging voltage detection
#define VBAT_NUM 10       // Number of vbat readings to average

//GUI settings
#define TRANSITION_TIME 350 // Frame transition time, ms
#define BAT_WIDTH 26        // Battery icon width
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
#define Y3 13
#define Y4 36
#define Y5 0
#define tuner_share 4
#define note_y 16
#endif

// font aliases for quick modding
#define SMALL_FONT ArialMT_Plain_10
#define MEDIUM_FONT ArialMT_Plain_16
#define BIG_FONT ArialMT_Plain_24
#define HUGE_FONT Roboto_Mono_Bold_52

// Globals
// static int iRSSI = 0;                           // BLE signal strength
int vbat_ring_count = 0;
int vbat_ring_sum = 0;
int vbat_result = 0;                            // For battery monitoring
int express_ring_count = 0;
int express_ring_sum = 0;
int express_result = 0;                         // For expression pedal monitoring
int old_exp_result = 0;
float effect_volume = 0.0;
int chrg_result = 0;                            // For charge state monitoring
int attempt_count = 0;                          // Connection attempts counter
int RTC_pins[]{0,2,4,12,13,14,15,25,26,27,32,33,34,35,36,37,38,39}; // These are RTC enabled GPIOs of ESP32, this is hardware, and if you choose to connect buttons to at least one of this list, deep sleep will be enabled
bool sw_RTC[NUM_SWITCHES];
int RTC_present = 0;                            // Number of RTC pins present in the config
int RTC_1st = -1;
// BUTTONS SECTION ====================================================================
const unsigned long longPressThreshold = 800;   // the threshold (in milliseconds) before a long press is detected
const unsigned long autoFireDelay = 120;        // the threshold (in milliseconds) between clicks if autofire is enabled
bool autoFireEnabled = true;                    // should the buttons generate continious clicks when pressed longer than a longPressThreshold
const unsigned long debounceThreshold = 20;     // the threshold (in milliseconds) for a button press to be confirmed (i.e. not "noise")
uint8_t ActiveFlags = 0;                        // Write buttons states to one binary mask; it's global so that we can check it anywhere

// UI SECTION =========================================================================
int scroller = 0;                               // Variable to keep scrolling offset 
String msgCaption, msgText;
bool tempUI = false;                            // If we are in the temporary frame which returns after a given timeout
boolean isOLEDUpdate;                           // Flag OLED needs refresh
unsigned long actual_timeout=FRAME_TIMEOUT;
uint64_t timeToGoBack = 0, time_to_sleep = 0;

// Presets section
bool fxState[] = {false,false,false,false,false,false,false}; // Array to store FX's on/off state before total bypass is ON

// Type for Coordinates of Fx Params
typedef struct {
  int fxSlot;
  int fxNumber;
} s_fx_coords;

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
  {"0-0","0-1","0-2","0-3","0-4"},                  //noise gate
  {"1-0","1-1","1-2","1-3","1-4"},                  //compressor
  {"DRIVE","2-1","2-2","2-3","2-4"},                //drive
  {"GAIN","TREBLE","MID","BASS","MASTER"},          //amp
  {"MODULATION","MOD-INTENS.","4-2","4-3","4-4"},   //modulation
  {"DELAY","5-1","5-2","5-3","BPM"},                //delay
  {"REVERB","6-1","6-2","6-3","6-4"}                //reverb
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