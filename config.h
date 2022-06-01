#ifndef _CONFIG_H
#define _CONFIG_H
//
//
//
// Comment this line out for a production build
#define DEBUG_ON
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
//#define CLASSIC
//
// Uncomment when using a Heltec module as their implementation doesn't support setMTU()
//#define HELTEC_WIFI
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
// Logical level of a button being pressed. If your buttons connect to GND, then comment this setting out.
// This setting also affects Pull-up/down, and waking source settings. 
//#define ACTIVE_HIGH
//
// How many pieces do you wish?
#define NUM_BANKS 12
//
// How many switches do we have
#define NUM_SWITCHES 4
//
// GPIOs of the buttons in your setup in the form of switchPins[]{GPIO_for_button1, GPIO_for_button2, GPIO_for_button3, GPIO_for_button4, ... }
//const uint8_t switchPins[]{17,5,18,23};                     // Switch gpio numbers (for those who already has built a pedal with these pins)
const uint8_t switchPins[]{33,14,27,26};  // PH EDIT
//const uint8_t switchPins[]{25,26,27,14};                      // Switch gpio numbers (recommended for those who is building a pedal, these pins allow deep sleep)
//
// Startup splash animation
#define ANIMATION_1
//#define ANIMATION_2
//
//
#define SP_AP_NAME "SparkBox"     // WiFi Access Point (AP) name
#define SP_AP_IP 192,168,4,1        // IP Address of the web page for setting up WiFi credentials
//
#endif
