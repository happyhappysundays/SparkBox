//******************************************************************************************
// SparkBox - BT pedal board for the Spark 40 amp - David Thompson 2021
// Supports four-switch pedals. Hold any of the effect buttons down for 1s to switch
// between Preset mode (1 to 4) and Effect mode (Drive, Mod, Delay, Reverb).
// Add an expression pedal to modify the cuirrent selected effect or toggle an effect.
//******************************************************************************************
#include "heltec.h"                 // Heltec's proprietary library :/
#include "BluetoothSerial.h"
#include "Spark.h"                  // Paul Hamshere's SparkIO library https://github.com/paulhamsh/SparkIO
#include "SparkIO.h"                // "
#include "SparkComms.h"             // "
#include "font.h"                   // Custom large font
#include "bitmaps.h"                // Custom bitmaps (icons)
#include "UI.h"                     // Any UI-related defines

//******************************************************************************************
// Battery charge function defines. Please uncomment just one.
//
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
//******************************************************************************************
// Expression pedal define. Comment this out if you DO NOT have the expression pedal mod
//#define EXPRESSION_PEDAL
//
//******************************************************************************************
// Dump preset define. Comment out if you'd prefer to not see so much text output
//#define DUMP_ON
//
//******************************************************************************************
#define PGM_NAME "SparkBox"
#define VERSION "BLE 0.53"
#define MAXNAME 20

char str[STR_LEN];                    // Used for processing Spark commands from amp
char param_str[50]; //debug
int param = -1;
unsigned long last;
float val = 0.0;
bool expression_target = false;       // False = parameter change, true = effect on/off
bool effectstate = false;
bool setting_modified = false;

// Variables required to track spark state and also for communications generally
unsigned int cmdsub;
SparkMessage msg;
SparkPreset preset;
SparkPreset presets[6];
int selected_preset;
bool got_presets;
uint8_t current_preset_num;           // Current preset number in Sparkbox
uint8_t new_preset_num;               // Reported current preset number incoming from App or Spark
uint8_t display_preset_num;           // Referenced preset number on Spark
int i, j, p;                          // Makes these local later...
byte bt_byte;                         // Stay-alive variables
int count;                            // "
bool flash_GUI;                       // Flash GUI items if true

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

int get_effect_index(char *str) {
  int ind, i;

  ind = -1;
  for (i = 0; ind == -1 && i <= 6; i++) {
    if (strcmp(presets[5].effects[i].EffectName, str) == 0) {
      ind  = i;
    }
  }
  return ind;
}

void  spark_state_tracker_start() {
  selected_preset = 0;
  got_presets = false;

  // Send commands to get preset details for all presets and current state (0x0100)
  spark_msg_out.get_preset_details(0x0000);
  spark_msg_out.get_preset_details(0x0001);
  spark_msg_out.get_preset_details(0x0002);
  spark_msg_out.get_preset_details(0x0003);
  spark_msg_out.get_preset_details(0x007f);
  spark_msg_out.get_preset_details(0x0100);
  spark_msg_out.get_hardware_preset_number();
}

// Get changes from app or Spark and update internal state to reflect this
// this function has the side-effect of loading cmdsub, msg and preset which can be used later

bool  update_spark_state() {
  int j, p, ind;

  connect_spark();  // reconnects if any disconnects happen
  spark_process();
  app_process();
  
  // K&R: Expressions connected by && or || are evaluated left to right, and it is guaranteed that evaluation will stop as soon as the truth or falsehood is known.
  if (spark_msg_in.get_message(&cmdsub, &msg, &preset) || app_msg_in.get_message(&cmdsub, &msg, & preset)) {
    Serial.printf("Message: %4X  ", cmdsub);

    isOLEDUpdate = true;        // Flag screen update

    switch (cmdsub) {
      // full preset details
      case 0x0101:
        isAppConnected = true;
      case 0x0301:  
        p = preset.preset_num;
        j = preset.curr_preset;
        if (p == 0x7f)       
          p = 4;
        if (j == 0x01)
          p = 5;
        presets[p] = preset;
        new_preset_num = p;  
        if (p==5) {
          got_presets = true;
        }
        // Only update the displayed preset number for HW presets
        else if (p < 4){
          display_preset_num = p; 
        }
        Serial.printf("Send / receive new preset: %x\n", p);      
        #ifdef DUMP_ON
          dump_preset(preset);
        #endif
        break;
      // change of amp  
      case 0x0306:
        strcpy(presets[5].effects[3].EffectName, msg.str2);
        Serial.printf("Change to new amp %s -> %s\n", msg.str1, msg.str2);
        break;
      // change of effect
      case 0x0106:
        expression_target = false;
        isAppConnected = true;
        Serial.printf("Change to new effect %s -> %s\n", msg.str1, msg.str2);
        ind = get_effect_index(msg.str1);
        if (ind >= 0) {
          strcpy(presets[5].effects[ind].EffectName, msg.str2);
        }
        // Debug - force a get preset here
        //spark_msg_out.get_preset_details(0x0100);     // Get the current preset details 
        setting_modified = true;
        break;
      // effect on/off  
      case 0x0115:
        isAppConnected = true;
      case 0x0315:
        expression_target = true;
        Serial.printf("Turn effect %s %s\n", msg.str1, msg.onoff ? "On " : "Off");
        ind = get_effect_index(msg.str1);
        if (ind >= 0) {
          presets[5].effects[ind].OnOff = msg.onoff;
        }
        // Debug - force a get preset here
        //spark_msg_out.get_preset_details(0x0100);     // Get the current preset details   
        setting_modified = true;        
        break;
      // change parameter value  
      case 0x0104:
        isAppConnected = true;
      case 0x0337:
         expression_target = false;
        Serial.printf("Change model parameter %s %d %0.3f\n", msg.str1, msg.param1, msg.val);
        ind = get_effect_index(msg.str1);
        if (ind >= 0) {
          presets[5].effects[ind].Parameters[msg.param1] = msg.val;
        }
        strcpy(param_str, msg.str1);
        param = msg.param1;
         // Debug - force a get preset here
        //spark_msg_out.get_preset_details(0x0100);     // Get the current preset details   
        setting_modified = true;
        break;  
        
      // Send licence key   
      case 0x0170:
        isAppConnected = true;
        break; 
        
      // change to preset  
      case 0x0138:
        isAppConnected = true;
      case 0x0338:
        selected_preset = msg.param2;
        current_preset_num = selected_preset;
        if (selected_preset == 0x7f) 
          selected_preset=4;
        presets[5] = presets[selected_preset];
        Serial.printf("Change to preset: %x\n", selected_preset);
        if (msg.param1 == 0x01) Serial.println("** Got a change to preset 0x100 from the app **");
        setting_modified = false;
        // Only update the displayed preset number for HW presets
        if (selected_preset < 4){
          display_preset_num = selected_preset; 
        }
        //spark_msg_out.get_preset_details(0x0100);     // Get the current preset details   
        break;
      // store to preset  
      case 0x0327:
        selected_preset = msg.param2;
        if (selected_preset == 0x7f) 
          selected_preset=4;
        presets[selected_preset] = presets[5];
        Serial.printf("Store in preset: %x\n", selected_preset);
        setting_modified = false;
        // Only update the displayed preset number for HW presets
        if (selected_preset < 4){
          display_preset_num = selected_preset; 
        }        
        break;
      // current selected preset
      case 0x0310:
        selected_preset = msg.param2;
        if (selected_preset == 0x7f) 
          selected_preset = 4;
        if (msg.param1 == 0x01) 
          selected_preset = 5;
        presets[5] = presets[selected_preset];
        Serial.printf("Hardware preset is: %x\n", selected_preset);
        current_preset_num = new_preset_num;
        // Only update the displayed preset number for HW presets
        if (selected_preset < 4){
          display_preset_num = selected_preset; 
        }
        isHWpresetgot = true;
        break;
      // Refresh preset info based on app-requested change
      case 0x0438:
        spark_msg_out.get_hardware_preset_number();   // Try to use get_hardware_preset_number() to pre-load the correct number
        spark_msg_out.get_preset_details(0x0100);     // Get the current preset details   
        setting_modified = false;
        break;
 
      default:
        Serial.println();
    }  
    if (got_presets) {
      #ifdef DUMP_ON
        dump_preset(presets[5]);
      #endif
    }
    return true;
  }
  else
    return false;
}

void setup() {
  current_preset_num = 0;
  new_preset_num = 0;
  display_preset_num = 0;
  isBTConnected = false;
  
  // Initialize device OLED display, and flip screen, as OLED library starts upside-down
  Heltec.display->init();
  Heltec.display->flipScreenVertically();

  // Set pushbutton inputs to pull-downs
  for (i = 0; i < NUM_SWITCHES; i++) {
    pinMode(sw_pin[i], INPUT_PULLDOWN);
  }
  
  // Read Vbat input
  vbat_result = analogRead(VBAT_AIN);

  // Show welcome message
  Heltec.display->clear();
  Heltec.display->setFont(ArialMT_Plain_24);
  Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
  Heltec.display->drawString(64, 10, PGM_NAME);
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
  Heltec.display->drawString(64, 35, VERSION);
  Heltec.display->display();
  delay(1000);                                 // Wait for splash screen

  Serial.begin(115200);                       // Start serial debug console monitoring via ESP32
  while (!Serial);

  Serial.println("Connecting...");
  isHWpresetgot = false;

  // Show connection message
  Heltec.display->clear();
  Heltec.display->setFont(ArialMT_Plain_24);
  Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
  Heltec.display->drawString(64, 10, "Connecting");
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
  Heltec.display->drawString(64, 35, "Please wait");
  Heltec.display->display();
  
  connect_to_all();             // sort out bluetooth connections
  spark_start(true);            // set up the classes to communicate with Spark and app
  spark_state_tracker_start();  // set up data to track Spark and app state

  isBTConnected = true; 
  Serial.println("Connected");  
  
  isPedalMode = false;                        // Default to Preset mode

  timer = timerBegin(0, 80, true);            // Setup timer
  timerAttachInterrupt(timer, &onTime, true); // Attach to our handler
  timerAlarmWrite(timer, 500000, true);       // Once per second, autoreload
  timerAlarmEnable(timer);                    // Start timer
}


void loop() {


  // do your own checks and processing here
  if (update_spark_state()) {    // just by calling this we update the local stored state which can be used here
    switch (cmdsub) {
      case 0x0115:    // just an example
        // do something with messages and presets[5]
        break;
    }
  }
  // do your own checks and processing here
#ifdef EXPRESSION_PEDAL
  // Read expression pedal
  // It can be sometimes difficult to get to zero, which we need,
  // so we subtract an offset and expend the scale to cover the full range
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
          app_msg_out.turn_effect_onoff(msg.str1,true);
          spark_msg_out.turn_effect_onoff(msg.str1,true);
          Serial.print("Turning effect ");
          Serial.print(msg.str1);
          Serial.println(" ON via pedal");
          effectstate = true;
       }
       // Send effect OFF state to Spark and App only if ON, also add hysteresis
       else if ((effect_volume < 0.3)&&(effectstate))
       {
          app_msg_out.turn_effect_onoff(msg.str1,false);
          spark_msg_out.turn_effect_onoff(msg.str1,false);
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
      app_msg_out.change_effect_parameter(msg.str1, msg.param1, effect_volume);
      spark_msg_out.change_effect_parameter(msg.str1, msg.param1, effect_volume);      
    }
  }
#endif

  // Process user input
  dopushbuttons();

  // In Preset mode, use the four buttons to select the four HW presets
  if ((sw_val[0] == HIGH)&&(!isPedalMode)) {  
    new_preset_num = 0;
    spark_msg_out.change_hardware_preset(0x00,new_preset_num);
    app_msg_out.change_hardware_preset(0x00,new_preset_num);         // Relay the same change to the app
    spark_msg_out.get_preset_details(0x0100);
  }
  else if ((sw_val[1] == HIGH)&&(!isPedalMode)) {  
    new_preset_num = 1;
    spark_msg_out.change_hardware_preset(0x00,new_preset_num);
    app_msg_out.change_hardware_preset(0x00,new_preset_num); 
    spark_msg_out.get_preset_details(0x0100);
  }
  else if ((sw_val[2] == HIGH)&&(!isPedalMode)) {  
    new_preset_num = 2;
    spark_msg_out.change_hardware_preset(0x00,new_preset_num);
    app_msg_out.change_hardware_preset(0x00,new_preset_num); 
    spark_msg_out.get_preset_details(0x0100);
  }
  else if ((sw_val[3] == HIGH)&&(!isPedalMode)) {  
    new_preset_num = 3;
    spark_msg_out.change_hardware_preset(0x00,new_preset_num);
    app_msg_out.change_hardware_preset(0x00,new_preset_num); 
    spark_msg_out.get_preset_details(0x0100);
  }

  // Effect mode (SW1-4 switch effects on/off)
  // Drive
  else if ((sw_val[0] == HIGH)&&(isPedalMode)) {    
    if (presets[5].effects[2].OnOff == true) {
      spark_msg_out.turn_effect_onoff(presets[5].effects[2].EffectName,false);
      app_msg_out.turn_effect_onoff(presets[5].effects[2].EffectName,false);
      presets[5].effects[2].OnOff = false;
    }
    else {
      spark_msg_out.turn_effect_onoff(presets[5].effects[2].EffectName,true);
      app_msg_out.turn_effect_onoff(presets[5].effects[2].EffectName,true);
      presets[5].effects[2].OnOff = true;
    }
  } 
  
  // Modulation
  else if ((sw_val[1] == HIGH)&&(isPedalMode)) {    
    if (presets[5].effects[4].OnOff == true) {
      spark_msg_out.turn_effect_onoff(presets[5].effects[4].EffectName,false);
      app_msg_out.turn_effect_onoff(presets[5].effects[4].EffectName,false);
      presets[5].effects[4].OnOff = false;
      setting_modified = true;
    }
    else {
      spark_msg_out.turn_effect_onoff(presets[5].effects[4].EffectName,true);
      app_msg_out.turn_effect_onoff(presets[5].effects[4].EffectName,true);
      presets[5].effects[4].OnOff = true;
      setting_modified = true;
    }
  }

  // Delay
  else if ((sw_val[2] == HIGH)&&(isPedalMode)) {   
    if (presets[5].effects[5].OnOff == true) {
      spark_msg_out.turn_effect_onoff(presets[5].effects[5].EffectName,false);
      app_msg_out.turn_effect_onoff(presets[5].effects[5].EffectName,false);
      presets[5].effects[5].OnOff = false;
      setting_modified = true;
    }
    else {
      spark_msg_out.turn_effect_onoff(presets[5].effects[5].EffectName,true);
      app_msg_out.turn_effect_onoff(presets[5].effects[5].EffectName,true);
      presets[5].effects[5].OnOff = true;
      setting_modified = true;
    }
  }

  // Reverb
  else if ((sw_val[3] == HIGH)&&(isPedalMode)) {   
    if (presets[5].effects[6].OnOff == true) {
      spark_msg_out.turn_effect_onoff(presets[5].effects[6].EffectName,false);
      app_msg_out.turn_effect_onoff(presets[5].effects[6].EffectName,false);
      presets[5].effects[6].OnOff = false;
      setting_modified = true;
    }
    else {
      spark_msg_out.turn_effect_onoff(presets[5].effects[6].EffectName,true);
      app_msg_out.turn_effect_onoff(presets[5].effects[6].EffectName,true);
      presets[5].effects[6].OnOff = true;
      setting_modified = true;
    }
  } 
 
  // Refresh screen when necessary
  refreshUI();

  // Request serial number every 10s as a 'stay-alive' function.
  if (millis() - count > 10000) {
    count = millis();
    //spark_msg_out.get_serial(); // debug
    //Serial.println("Tick..."); // debug
  }
  
} // loop()}
