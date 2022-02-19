//******************************************************************************************
// SparkBox - BT pedal board for the Spark 40 amp - David Thompson 2022
// Supports four-switch pedals. Hold any of the effect buttons down for 1s to switch
// between Preset mode (1 to 4) and Effect mode (Drive, Mod, Delay, Reverb).
// Added an expression pedal to modify the cuirrent selected effect or toggle an effect. 
//******************************************************************************************
//
// Battery charge function defines. Please uncomment just one.
// You have no mods to monitor the battery, so it will show empty
#define BATT_CHECK_0
//
// You are monitoring the battery via a 2:1 10k/10k resistive divider to GPIO23
// You can see an accurate representation of the remaining battery charge and a kinda-sorta
// indicator of when the battery is charging. Maybe.
//#define BATT_CHECK_1
//
// You have the battery monitor mod described above AND you have a connection between the 
// CHRG pin of the charger chip and GPIO 33. Go you! Now you have a guaranteed charge indicator too.
//#define BATT_CHECK_2
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
// Uncomment if two-colour OLED screens are used. Offsets some text and alternate tuner
//#define TWOCOLOUR
//
//******************************************************************************************
#include "SSD1306Wire.h"            // https://github.com/ThingPulse/esp8266-oled-ssd1306
#include "BluetoothSerial.h"
#include "Spark.h"                  // Paul Hamshere's SparkIO library https://github.com/paulhamsh/Spark/tree/main/Spark
#include "SparkIO.h"                // "
#include "SparkComms.h"             // "
#include "font.h"                   // Custom large font
#include "bitmaps.h"                // Custom bitmaps (icons)
#include "UI.h"                     // Any UI-related defines
//
//******************************************************************************************

#define PGM_NAME "SparkBox"
#define VERSION "V0.65" 

SSD1306Wire oled(0x3c, 4, 15);        // Default OLED Screen Definitions - ADDRESS, SDA, SCL 

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

hw_timer_t * timer = NULL;            // Timer variables
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile boolean isTimeout = false;   // Update battery icon flag
volatile boolean isRSSIupdate = false;// Update RSSI display flag

//******************************************************************************************

// Timer interrupt handler
void IRAM_ATTR onTime() {
   portENTER_CRITICAL_ISR(&timerMux);
   isTimeout = true;
   isRSSIupdate = true;
   portEXIT_CRITICAL_ISR(&timerMux);
}

void setup() {
  display_preset_num = 0;

  // Manually toggle the /RST pin to add Heltec module functionality
  // but without the Heltec library
  pinMode(16,OUTPUT);
  digitalWrite(16, LOW);
  delay(50);
  digitalWrite(16, HIGH);

  // Initialize device OLED display, and flip screen, as OLED library starts upside-down
  oled.init();
  oled.flipScreenVertically();

  // Set pushbutton inputs to pull-downs
  for (i = 0; i < NUM_SWITCHES; i++) {
    pinMode(sw_pin[i], INPUT_PULLDOWN);
  }
  
  // Read Vbat input
  vbat_result = analogRead(VBAT_AIN);

  // Show welcome message
  oled.clear();
  oled.setFont(ArialMT_Plain_24);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(X1, Y3, PGM_NAME);
  oled.setFont(ArialMT_Plain_16);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(X1, Y4, VERSION);
  oled.display();
  delay(1000);                                // Wait for splash screen

  Serial.begin(115200);                       // Start serial debug console monitoring via ESP32
  while (!Serial);

  Serial.println("Connecting...");

  // Show connection message
  oled.clear();
  oled.setFont(ArialMT_Plain_24);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(X1, Y3, "Connecting");
  oled.setFont(ArialMT_Plain_16);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(X1, Y4, "Please wait");
  oled.display();

  isPedalMode = false;                        // Default to Preset mode

  timer = timerBegin(0, 80, true);            // Setup timer
  timerAttachInterrupt(timer, &onTime, true); // Attach to our handler
  timerAlarmWrite(timer, 500000, true);       // 500ms, autoreload
  timerAlarmEnable(timer);                    // Start timer

  spark_state_tracker_start();                // Set up data to track Spark and app state
}

void loop() {

#ifdef EXPRESSION_PEDAL
  // Only handle the pedal if the app is connected
  if (conn_status[APP]){
    // Read expression pedal
    // It can be sometimes difficult to get to zero, which we need,
    // so we subtract an offset and expand the scale to cover the full range
    express_result = (analogRead(EXP_AIN)/ 45) - 10;
  
    // Rolling average to remove noise
    if (express_ring_count < 10) {
      express_ring_sum += express_result;
      express_ring_count++;
      express_result = express_ring_sum / express_ring_count;
    }
    // Once enough is gathered, do a decimating average
    else {
      express_ring_sum *= 0.9;
      express_ring_sum += express_result;
      express_result = express_ring_sum / 10;
    }  
  
    // Reduce noise and only respond to deliberate changes
    if ((abs(express_result - old_exp_result) > 4))
    {
      old_exp_result = express_result;
      effect_volume = float(express_result);
      effect_volume = effect_volume / 64;
      if (effect_volume > 1.0) effect_volume = 1.0;
      if (effect_volume < 0.0) effect_volume = 0.0;
  #ifdef DUMP_ON
      Serial.print("Pedal data: ");
      Serial.print(express_result);
      Serial.print(" : ");
      Serial.println(effect_volume);
  #endif
      // If effect on/off
      if (expression_target) {
         // Send effect ON state to Spark and App only if OFF
         if ((effect_volume > 0.5)&&(!effectstate)) {
            change_generic_onoff(get_effect_index(msg.str1),true);
            Serial.print("Turning effect ");
            Serial.print(msg.str1);
            Serial.println(" ON via pedal");
            effectstate = true;
         }
         // Send effect OFF state to Spark and App only if ON, also add hysteresis
         else if ((effect_volume < 0.3)&&(effectstate))
         {
            change_generic_onoff(get_effect_index(msg.str1),false);
            Serial.print("Turning effect ");
            Serial.print(msg.str1);
            Serial.println(" OFF via pedal");
            effectstate = false;
         }
      }
      // Parameter change
      else
      {
        // Send expression pedal value to Spark and App
        change_generic_param(get_effect_index(msg.str1), msg.param1, effect_volume);
      }
    }
  }
#endif //EXPRESSION_PEDAL

  // Process user input
  dopushbuttons();

  // In Preset mode, use the four buttons to select the four HW presets
  if ((sw_val[0] == HIGH)&&(!isPedalMode)) {  
    change_hardware_preset(0);
    display_preset_num = 0; 
  }
  else if ((sw_val[1] == HIGH)&&(!isPedalMode)) {  
    change_hardware_preset(1);
    display_preset_num = 1; 
  }
  else if ((sw_val[2] == HIGH)&&(!isPedalMode)) {  
    change_hardware_preset(2);
    display_preset_num = 2; 
  }
  else if ((sw_val[3] == HIGH)&&(!isPedalMode)) {  
    change_hardware_preset(3);
    display_preset_num = 3; 
  }

  // Effect mode (SW1-4 switch effects on/off)
  // Drive
  else if ((sw_val[0] == HIGH)&&(isPedalMode)) {    
    change_drive_toggle();
    setting_modified = true;
  } 
  
  // Modulation
  else if ((sw_val[1] == HIGH)&&(isPedalMode)) {    
    change_mod_toggle();
    setting_modified = true;
  }

  // Delay
  else if ((sw_val[2] == HIGH)&&(isPedalMode)) {   
    change_delay_toggle();
    setting_modified = true;
  }

  // Reverb
  else if ((sw_val[3] == HIGH)&&(isPedalMode)) {   
    change_reverb_toggle();
    setting_modified = true;
  } 

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
    isOLEDUpdate = true;        // Flag screen update
  }

  // Refresh screen
  refreshUI();
  
} // loop()}
