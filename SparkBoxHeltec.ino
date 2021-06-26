//******************************************************************************************
// SparkBox - BT pedal board for the Spark 40 amp - David Thompson 2021 - Current version v0.43
// Supports four-switch pedals. Hold any of the effect buttons down for 1s to switch
// between Preset mode (1 to 4) and Effect mode (Drive, Mod, Delay, Reverb)
//******************************************************************************************
#include "heltec.h"                 // Heltec's proprietary library :/
#include "BluetoothSerial.h"
#include "Spark.h"                  // Paul Hamshere's SparkIO library https://github.com/paulhamsh/SparkIO
#include "SparkIO.h"                // "
#include "SparkComms.h"             // "
#include "font.h"                   // Custom font
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
// CHRG pin (1) of the TP4054 chip and GPIO 33. Go you! now you have a guaranteed charge indicator too.
//#define BATT_CHECK_2
//
//******************************************************************************************

#define PGM_NAME "SparkBox"
#define VERSION "0.43"
#define MAXNAME 20

SparkIO spark_io(false);              // Non-passthrough Spark IO (per Paul)
SparkComms spark_comms;
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

hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile boolean isTimeout = false;   // Update battery icon flag
volatile boolean isRSSIupdate = false;// Update RSSI display flag

// Timer interrupt handler
void IRAM_ATTR onTime() {
   portENTER_CRITICAL_ISR(&timerMux);
   isTimeout = true;
   isRSSIupdate = true;
   portEXIT_CRITICAL_ISR(&timerMux);
}

//Part of the 'stay-alive' function
void flush_in() {
  bt_byte = spark_comms.bt->read();
  while (bt_byte != 0xf7)
  bt_byte = spark_comms.bt->read();
}

void setup() {
  // Initialize device OLED display, and flip screen, as OLED library starts upside-down
  Heltec.display->init();
  Heltec.display->flipScreenVertically();

  // Set pushbutton inputs to pull-ups
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

  Serial.begin(115200);                 // Start serial debug console monitoring via ESP32
  while (!Serial);

  spark_io.comms = &spark_comms;        // Create SparkIO comms and connect
  spark_comms.start_bt();
  spark_comms.bt->register_callback(btEventCallback); // Register BT disconnect handler

  isPedalMode = false;                  // Preset mode

  timer = timerBegin(0, 80, true);            // Setup timer
  timerAttachInterrupt(timer, &onTime, true); // Attach to our handler
  timerAlarmWrite(timer, 1000000, true);      // Once per second, autoreload
  timerAlarmEnable(timer);                    // Start timer

  delay(1000);
}

void loop() {
  // Check if amp is connected to device
  if(!isBTConnected) {
    Serial.println("Connecting...");
    //isRestarted = true;
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
    
    spark_comms.connect_to_spark();
    isBTConnected = true;
    Serial.println("Connected");

    delay(500); //debug
    spark_io.get_hardware_preset_number();   // Try to use get_hardware_preset_number() to pre-load the correct number
    spark_io.get_preset_details(0x7F);       // Get the preset details for 0-3
    spark_io.get_preset_details(0x0100);     // Get the current preset details
  } 
  // Device connected
  else {
    // We'd like to update the screen even before any user input, but only once
    // To do this reliably we have to interrogate the hardware preset number until we've recieved it
 /*   if (!isHWpresetgot){
      spark_io.get_hardware_preset_number();   // Try to use get_hardware_preset_number() to pre-load the correct number
      delay(500);
    }
    */
    spark_io.process();
  
    // Messages from the amp
    if (spark_io.get_message(&cmdsub, &msg, &preset)) { //there is something there
      
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

    // Below is part of the 'stay-alive' function
    if (millis() - count > 10000) {
      // Request serial number and read returned bytes and discard - stay-alive link to Spark
      count = millis();
      spark_io.get_serial();
    }
  
  } // Connected
} // loop()

// BT disconnect callback
void btEventCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param){
  if(event == ESP_SPP_CLOSE_EVT ){    // On BT connection close
    isBTConnected = false;            // Clear connected flag
    Serial.println("Lost BT connection");
    pre = 0; //debug
  }
}
