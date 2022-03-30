// Defines
#define MAXNAME 24        // Truncate preset names
#define NUM_SWITCHES 4    // How many switches do we have
#define NUM_MODES 2       // How many pedal modes does it support 
#define VBAT_AIN 32       // Vbat sense (2:1 divider)
#define CHRG_AIN 33       // Charge pin sense (10k pull-up)
#define EXP_AIN 34        // Expression pedal input (3V3)
#define MAX_ATTEMPTS 5    // (Re-)Connection attempts before going to sleep
#define MILLIS_PER_ATTEMPT 6000 // milliseconds per connection attempts, this is used when reconnecting, not quite as expected though
#define CHRG_LOW 2000
//
#define BATTERY_LOW 2082  // Noise floor of 3.61V (<5%)
#define BATTERY_100 2422  // Noise floor of 4.20V (100%)
//
#define BATTERY_HIGH 2256 // Noise floor of 3.91V (>66%)
#define BATTERY_MAX 2290  // Noise floor of 3.97V (>80%)
#define BATTERY_CHRG 2451 // Totally empirical, adjust as required. Currently 4.25V-ish
#define VBAT_NUM 10       // Number of vbat readings to average

//GUI settings
#define TRANSITION_TIME 600 // Frame transition time, ms
#define STATUS_HEIGHT 16  // Status line height
#define BAT_WIDTH 26      // Battery icon width
#define CONN_ICON_WIDTH 11 // Connection status icons width
#define FX_ICON_WIDTH 18  // Exxects icons width
// Text offsets for different types of display

#ifdef TWOCOLOUR
// Two-colour displays
#define X1 64 // Please wait, Version, Connecting, SparkBox, Reconnecting
#define Y1 22
#define Y2 47
#define Y3 16
#define Y4 41
#define Y5 16
#define tuner_scale 2304 // 48 * 48
#else

// Single colour displays
#define X1 64 // Please wait, Version, Connecting, SparkBox, Reconnecting
#define Y1 16
#define Y2 42
#define Y3 10
#define Y4 35
#define Y5 0
#define tuner_scale 4096
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
int sw_pin[]{17,5,18,23};                     // Switch gpio numbers (for those who already has built a pedal with these pins)
//int sw_pin[]{25,26,27,14};                      // Switch gpio numbers (for those who is building a pedal, these pins allow deep sleep)
                                                // SW1 Toggle Drive 
                                                // SW2 Toggle Modulation
                                                // SW3 Toggle Delay
                                                // SW4 Toggle Reverb
                                                // SW5 Decrement preset
                                                // SW6 Increment preset

const unsigned long longPressThreshold = 800;   // the threshold (in milliseconds) before a long press is detected
const unsigned long debounceThreshold = 20;     // the threshold (in milliseconds) for a button press to be confirmed (i.e. not "noise")
unsigned long buttonTimer[NUM_SWITCHES];        // stores the time that the button was pressed (relative to boot time)
unsigned long buttonPressDuration[NUM_SWITCHES];// stores the duration (in milliseconds) that the button was pressed/held down for
bool buttonActive[NUM_SWITCHES];                // indicates if the button is active/pressed
bool longPressActive[NUM_SWITCHES];             // indicates if the button has been long-pressed
bool buttonClick[NUM_SWITCHES];                 // indicates if the button has been clicked
bool longPressFired = false;                    // indicates if the button has been long-pressed
bool AnylongPressActive = false;                // OR of any longPressActive states
bool AllPressActive = false;                    // AND of any longPressActive states
int scroller = 0;                               // Variable to keep scrolling offset 

uint64_t time_to_sleep = 0;
bool fxState[] = {false,false,false,false,false,false,false}; // Array to store FX's on/off state before total bypass is ON

// Flags
boolean isOLEDUpdate;                           // Flag OLED needs refresh
uint8_t ActiveFlags = 0;                        // Write buttons states to one binary mask variable
uint8_t ClickFlags = 0;                         // Write buttons states to one binary mask variable
uint8_t LongPressFlags = 0;                     // Write buttons states to one binary mask variable
