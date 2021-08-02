//******************************************************************************************
// SparkBox - BT pedal board for the Spark 40 amp - David Thompson 2021
// Supports four-switch pedals. Hold any of the effect buttons down for 1s to switch
// between Preset mode (1 to 4) and Effect mode (Drive, Mod, Delay, Reverb)
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

#define PGM_NAME "SparkBox"
#define VERSION "BLE 0.48"
#define MAXNAME 20

SparkIO spark_io(false);              // Non-passthrough Spark IO (per Paul)
char str[STR_LEN];                    // Used for processing Spark commands from amp
unsigned int cmdsub;
SparkMessage msg;                     // SparkIO messsage/preset variables
SparkPreset preset;
SparkPreset presets[6];               // [5] = current preset
int8_t pre;                           // Internal current preset number
int8_t selected_preset;               // Reported current preset number
int i, j, p;                          // Makes these local later...
byte bt_byte;                         // Stay-alive variables
int count;                            // "

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

  Serial.begin(115200);                       // Start serial debug console monitoring via ESP32
  while (!Serial);

  start_bt(true);                             // Set up BLE
    
  isPedalMode = false;                        // Default to Preset mode

  timer = timerBegin(0, 80, true);            // Setup timer
  timerAttachInterrupt(timer, &onTime, true); // Attach to our handler
  timerAlarmWrite(timer, 1000000, true);      // Once per second, autoreload
  timerAlarmEnable(timer);                    // Start timer

  delay(1000);                                // Give a moment to display logo
}

void loop() {
  // Check if amp is connected to device
  if(!isBTConnected) {
    Serial.println("Connecting...");
    isHWpresetgot = false;

    // Show reconnection message
    Heltec.display->clear();
    Heltec.display->setFont(ArialMT_Plain_24);
    Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
    Heltec.display->drawString(64, 10, "Connecting");
    Heltec.display->setFont(ArialMT_Plain_16);
    Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
    Heltec.display->drawString(64, 35, "Please wait");
    Heltec.display->display();
    
    connect_to_spark();                      // Connect to SPark via BLE
    isBTConnected = true;
    Serial.println("Connected");

    delay(500); //debug
    spark_io.get_hardware_preset_number();   // Try to use get_hardware_preset_number() to pre-load the correct number
    spark_io.get_preset_details(0x7F);       // Get the preset details for 0-3
    spark_io.get_preset_details(0x0100);     // Get the current preset details
  } 
  // Device connected
  else {

    spark_io.process();           // Process commands from Spark
  
    // Messages from the amp
    if (spark_io.get_message(&cmdsub, &msg, &preset)) { 
      
      isStatusReceived = true;    // Hopefully this means we have the status
      isOLEDUpdate = true;        // Flag screen update
  
      Serial.print("From Spark: ");
      Serial.println(cmdsub, HEX);
      sprintf(str, "< %4.4x", cmdsub);
  
      // Generic ack from Spark
      if (cmdsub == 0x0301) {
        p = preset.preset_num;
        j = preset.curr_preset;
        if (p == 0x7f)       
          p = 4;
        if (j == 0x01)
          p = 5;
        presets[p] = preset;
        dump_preset(preset);
      }
  
      // Preset changed on amp
      if (cmdsub == 0x0338) {
        selected_preset = msg.param2;
        presets[5] = presets[selected_preset];
        Serial.print("Change to preset: ");
        Serial.println(selected_preset, HEX);
        spark_io.get_preset_details(0x0100);
      }      
      
      // Store current preset in amp
      if (cmdsub == 0x0327) {
        selected_preset = msg.param2;
        if (selected_preset == 0x7f) 
          selected_preset=4;
        presets[selected_preset] = presets[5];
        Serial.print("Store in preset: ");
        Serial.println(selected_preset, HEX);
      }
  
      // Current hardware preset from amp
      if (cmdsub == 0x0310) {
        selected_preset = msg.param2;
        j = msg.param1;
        if (selected_preset == 0x7f) 
          selected_preset = 4;
        if (j == 0x01) 
          selected_preset = 5;
        presets[5] = presets[selected_preset];
        Serial.print("Hardware preset is: ");
        Serial.println(selected_preset, HEX);
        isHWpresetgot = true;
      }
    } // get-message
  
    // Update button-led preset number on receipt of hardware preset number
    pre = selected_preset;
  
    // Process user input
    dopushbuttons();

    // In Preset mode, use the four buttons to select the four HW presets
    if ((sw_val[0] == HIGH)&&(!isPedalMode)) {  
      pre = 0;
      spark_io.change_hardware_preset(pre);
      spark_io.get_preset_details(0x0100);
    }
    else if ((sw_val[1] == HIGH)&&(!isPedalMode)) {  
      pre = 1;
      spark_io.change_hardware_preset(pre);
      spark_io.get_preset_details(0x0100);
    }
    else if ((sw_val[2] == HIGH)&&(!isPedalMode)) {  
      pre = 2;
      spark_io.change_hardware_preset(pre);
      spark_io.get_preset_details(0x0100);
    }
    else if ((sw_val[3] == HIGH)&&(!isPedalMode)) {  
      pre = 3;
      spark_io.change_hardware_preset(pre);
      spark_io.get_preset_details(0x0100);
    }
    
    // Effect mode (SW1-4 switch effects on/off)
    // Drive
    else if ((sw_val[0] == HIGH)&&(isPedalMode)) {    
      if (presets[5].effects[2].OnOff == true) {
        spark_io.turn_effect_onoff(presets[5].effects[2].EffectName,false);
        presets[5].effects[2].OnOff = false;
      }
      else {
        spark_io.turn_effect_onoff(presets[5].effects[2].EffectName,true);
        presets[5].effects[2].OnOff = true;
      }
    } 
    
    // Modulation
    else if ((sw_val[1] == HIGH)&&(isPedalMode)) {    
      if (presets[5].effects[4].OnOff == true) {
        spark_io.turn_effect_onoff(presets[5].effects[4].EffectName,false);
        presets[5].effects[4].OnOff = false;
      }
      else {
        spark_io.turn_effect_onoff(presets[5].effects[4].EffectName,true);
        presets[5].effects[4].OnOff = true;
      }
    }
  
    // Delay
    else if ((sw_val[2] == HIGH)&&(isPedalMode)) {   
      if (presets[5].effects[5].OnOff == true) {
        spark_io.turn_effect_onoff(presets[5].effects[5].EffectName,false);
        presets[5].effects[5].OnOff = false;
      }
      else {
        spark_io.turn_effect_onoff(presets[5].effects[5].EffectName,true);
        presets[5].effects[5].OnOff = true;
      }
    }
  
    // Reverb
    else if ((sw_val[3] == HIGH)&&(isPedalMode)) {   
      if (presets[5].effects[6].OnOff == true) {
        spark_io.turn_effect_onoff(presets[5].effects[6].EffectName,false);
        presets[5].effects[6].OnOff = false;
      }
      else {
        spark_io.turn_effect_onoff(presets[5].effects[6].EffectName,true);
        presets[5].effects[6].OnOff = true;
      }
    } 
    
    // Update hardware preset number with button-led preset number
    selected_preset = pre;
  
    // Refresh screen when necessary
    refreshUI();

    // Request serial number every 10s as a 'stay-alive' function.
    if (millis() - count > 10000) {
      count = millis();
      spark_io.get_serial();
    }
  
  } // Connected
} // loop()
