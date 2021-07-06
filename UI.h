// Defines
#define NUM_SWITCHES 4
#define VBAT_AIN 32       // Vbat sense (2:1 divider)
#define CHRG_AIN 33       // Charge pin sense (10k pull-up)
#define CHRG_LOW 2000
#define BATTERY_LOW 2288
#define BATTERY_MID 2331
#define BATTERY_HIGH 2399
#define BATTERY_MAX 2526
#define BATTERY_CHRG 2560 // Maybe make this higher?

// Globals
int vbat_result;                                // For battery monitoring
int chrg_result;                                // For charge state monitoring
int sw_val[NUM_SWITCHES];     
int sw_pin[]{17,5,18,23};                       // Switch gpio numbers

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
boolean latchpress;                             // latch to stop repeating the long press state

// Flags
boolean isBTConnected;                          // Duh
boolean isStatusReceived;                       // Status received from Spark
boolean isOLEDUpdate;                           // Flag OLED needs refresh
boolean isPedalMode;                            // Pedal mode: 0 = preset, 1 = effect
boolean isHWpresetgot;                          // Flag to show that the hardware preset number has been processed
