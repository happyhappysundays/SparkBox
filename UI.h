// Defines
#define MAXNAME 24        // Truncate preset names
#define NUM_SWITCHES 4
#define VBAT_AIN 32       // Vbat sense (2:1 divider)
#define CHRG_AIN 33       // Charge pin sense (10k pull-up)
#define EXP_AIN 34        // Expression pedal input (3V3)
#define MAX_ATTEMPTS 5    // (Re-)Connection attempts before going to sleep
#define CHRG_LOW 2000
//
#define BATTERY_LOW 2082  // Noise floor of 3.61V (<5%)
#define BATTERY_10 2127   // Noise floor of 3.69V (<10%)
#define BATTERY_20 2151   // Noise floor of 3.73V (<20%)
#define BATTERY_30 2174   // Noise floor of 3.77V (<30%)
#define BATTERY_40 2191   // Noise floor of 3.80V (<40%)
#define BATTERY_50 2214   // Noise floor of 3.84V (<50%)
#define BATTERY_60 2231   // Noise floor of 3.87V (<60%)
#define BATTERY_70 2277   // Noise floor of 3.95V (<70%)
#define BATTERY_80 2318   // Noise floor of 4.02V (<80%)
#define BATTERY_90 2352   // Noise floor of 4.08V (<85%) (was 4.11V for 90%)
#define BATTERY_100 2422  // Noise floor of 4.20V (100%)
//
#define BATTERY_HIGH 2256 // Noise floor of 3.91V (>66%)
#define BATTERY_MAX 2290  // Noise floor of 3.97V (>80%)
#define BATTERY_CHRG 2451 // Totally empirical, adjust as required. Currently 4.25V-ish
#define VBAT_NUM 10       // Number of vbat readings to average

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
static int iRSSI = 0;                           // BLE signal strength
int vbat_ring_count = 0;
int vbat_ring_sum = 0;
int vbat_result = 0;                            // For battery monitoring
int express_ring_count = 0;
int express_ring_sum = 0;
int express_result = 0;                         // For expression pedal monitoring
int old_exp_result = 0;
float effect_volume = 0.0;
int temp = 0;   
int chrg_result = 0;                             // For charge state monitoring
int attempt_count = 0;                         // Connection attempts counter

int RTC_pins[]{0,2,4,12,13,14,15,25,26,27,32,33,34,35,36,37,38,39};
bool sw_RTC[NUM_SWITCHES];
int sw_val[NUM_SWITCHES];
int sw_pin[]{17,5,18,23};                       // Switch gpio numbers (for those who already has built a pedal with these pins)
//int sw_pin[]{25,26,27,14};                       // Switch gpio numbers (for those who is building a pedal, these pins allow deep sleep)
                                                // SW1 Toggle Drive 
                                                // SW2 Toggle Modulation
                                                // SW3 Toggle Delay
                                                // SW4 Toggle Reverb
                                                // SW5 Decrement preset
                                                // SW6 Increment preset
                                            
const unsigned long longPressThreshold = 1000;  // the threshold (in milliseconds) before a long press is detected
const unsigned long debounceThreshold = 50;     // the threshold (in milliseconds) for a button press to be confirmed (i.e. not "noise")
unsigned long buttonTimer[NUM_SWITCHES];        // stores the time that the button was pressed (relative to boot time)
unsigned long buttonPressDuration[NUM_SWITCHES];// stores the duration (in milliseconds) that the button was pressed/held down for
boolean buttonActive[NUM_SWITCHES];             // indicates if the button is active/pressed
boolean longPressActive[NUM_SWITCHES];          // indicate if the button has been long-pressed
boolean AnylongPressActive = false;             // OR of any longPressActive states
boolean AllPressActive = false;                 // AND of any longPressActive states
boolean latchpress;                             // latch to stop repeating the long press state
char TempString[STR_LEN];
int16_t metervalue = 0;
int16_t meter_target = 0;
int16_t hubvalue = 0;
int16_t meter_x = 0;
int16_t meter_y = 0;
int16_t hub_x = 0;
int16_t hub_y = 0;



// Flags
boolean isOLEDUpdate;                           // Flag OLED needs refresh
boolean isPedalMode;                            // Pedal mode: 0 = preset, 1 = effect
