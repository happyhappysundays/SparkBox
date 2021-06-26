// Update Icons across top of screen
void updateIcons() {
  
  // Show BT icon if connected
  if(isBTConnected){
    Heltec.display->drawXbm(btlogo_pos, 0, bt_width, bt_height, bt_bits);
    // ToDo: measure BT RSSI
    Heltec.display->drawXbm(rssi_pos, 0, rssi_width, rssi_height, rssi_bits);
  }
  // Update drive status icons once data available
  if(isStatusReceived || isTimeout){  
    // Drive icon
    if (presets[5].effects[2].OnOff){
      Heltec.display->drawXbm(drive_pos, 0, icon_width, icon_height, drive_on_bits);
    }
    else {
      Heltec.display->drawXbm(drive_pos, 0, icon_width, icon_height, drive_off_bits);   
    }
    // Mod icon
    if (presets[5].effects[4].OnOff) {
      Heltec.display->drawXbm(mod_pos, 0, icon_width, icon_height, mod_on_bits);
    }
    else {
       Heltec.display->drawXbm(mod_pos, 0, icon_width, icon_height, mod_off_bits);   
    }
    // Delay icon
    if (presets[5].effects[5].OnOff) {
      Heltec.display->drawXbm(delay_pos, 0, icon_width, icon_height, delay_on_bits);
    }
    else {
       Heltec.display->drawXbm(delay_pos, 0, icon_width, icon_height, delay_off_bits);   
    }
    // Reverb icon
    if (presets[5].effects[6].OnOff) {
      Heltec.display->drawXbm(rev_pos, 0, icon_width, icon_height, rev_on_bits);
    }
    else {
       Heltec.display->drawXbm(rev_pos, 0, icon_width, icon_height, rev_off_bits);
    }
  }
  // Battery icon control
  // Measure battery periodically via a timer
  if (isTimeout) {
    vbat_result = analogRead(VBAT_AIN); // Read battery voltage
    //Serial.print("Vbat = ");
    //Serial.print(vbat_result);
    chrg_result = analogRead(CHRG_AIN); // Check state of /CHRG output
    //Serial.print(", /CHRG = ");
    //Serial.println(chrg_result);
    isTimeout = false;
  }

// No battery monitoring mods available so just show empty icon
#ifdef BATT_CHECK_0
  Heltec.display->drawXbm(bat_pos, 0, bat_width, bat_height, bat00_bits);
#endif

// If advanced charge detection available, and charge detected
#ifdef BATT_CHECK_2
  if (chrg_result < CHRG_LOW) {
    Heltec.display->drawXbm(bat_pos, 0, bat_width, bat_height, batcharging_bits);
  } else
#endif

// Basic battery detection available. Coarse cut-offs for visual 
// guide to remaining capacity
#if defined(BATT_CHECK_1) || defined(BATT_CHECK_2)
  if (vbat_result < BATTERY_LOW) {
    Heltec.display->drawXbm(bat_pos, 0, bat_width, bat_height, bat00_bits);
  }
  else if (vbat_result < BATTERY_MID) {
    Heltec.display->drawXbm(bat_pos, 0, bat_width, bat_height, bat33_bits);
  }
  else if (vbat_result < BATTERY_HIGH) {
    Heltec.display->drawXbm(bat_pos, 0, bat_width, bat_height, bat66_bits);
  }
  else if (vbat_result < BATTERY_MAX) {
    Heltec.display->drawXbm(bat_pos, 0, bat_width, bat_height, bat100_bits);
  } 
  else {
    Heltec.display->drawXbm(bat_pos, 0, bat_width, bat_height, batcharging_bits);
  } 
#endif
}

// Print out the requested preset data
void dump_preset(SparkPreset preset) {
  int i,j;

  Serial.print(preset.curr_preset); Serial.print(" ");
  Serial.print(preset.preset_num); Serial.print(" ");
  Serial.print(preset.Name); Serial.print(" ");
  Serial.println(preset.Description);

  for (j=0; j<7; j++) {
    Serial.print("    ");
    Serial.print(preset.effects[j].EffectName); Serial.print(" ");
    if (preset.effects[j].OnOff == true) Serial.print(" On "); else Serial.print (" Off ");
    for (i = 0; i < preset.effects[j].NumParameters; i++) {
      Serial.print(preset.effects[j].Parameters[i]); Serial.print(" ");
    }
    Serial.println();
  }
}

// Pushbutton handling
void dopushbuttons(void)
{
  // Debounce and long press code
  for (i = 0; i < NUM_SWITCHES; i++) {
    // If the button pin reads HIGH, the button is pressed (VCC)
    if (digitalRead(sw_pin[i]) == HIGH)
    {
      // If button was previously off, mark the button as active, and reset the timer
      if (buttonActive[i] == false){
          buttonActive[i] = true;
          buttonTimer[i] = millis();
       }
       
      // Calculate the button press duration by subtracting the button time from the boot time
      buttonPressDuration[i] = millis() - buttonTimer[i];
      
      // Mark the button as long-pressed if the button press duration exceeds the long press threshold
      if ((buttonPressDuration[i] > longPressThreshold) && (longPressActive[i] == false)) {
          longPressActive[i] = true;
          latchpress = true;
      }
    }
      
    // The button either hasn't been pressed, or has been released
    else {
      // Reset switch register here so that switch is not repeated    
      sw_val[i] = LOW; 
      
      // If the button was marked as active, it was recently pressed
      if (buttonActive[i] == true){
        // Reset the long press active state
        if (longPressActive[i] == true){
          longPressActive[i] = false;
          latchpress = false;
        }
        
        // The button was previously active. We either need to debounce the press (noise) or register a normal/short press
        else {
          // if the button press duration exceeds our bounce threshold, then we register a short press
          if (buttonPressDuration[i] > debounceThreshold){
            sw_val[i] = HIGH;
          }
        }
        
        // Reset the button active status
        buttonActive[i] = false;
      }
    }
  }    

  // OR all the long press flags so any of the four main footswitches can switch modes
  AnylongPressActive = (longPressActive[0] || longPressActive[1] || longPressActive[2] || longPressActive[3]);
 
  // Has any button been held down
  if (AnylongPressActive && (latchpress == true)){
      Serial.println("Switching pedal mode");
      isOLEDUpdate = true;
      latchpress = false;
      if (isPedalMode) isPedalMode = false;   // Toggle pedal mode
      else isPedalMode = true;
  }
}

// Refresh UI
void refreshUI(void)
{
  // if a change has been made or the timer timed out and we have the preset...
  // if ((isOLEDUpdate || isTimeout) && isHWpresetgot){
  if (isOLEDUpdate || isTimeout){  
    isOLEDUpdate = false;  
    Heltec.display->clear();
    Heltec.display->setFont(ArialMT_Plain_16);
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    if (!isPedalMode) {
      Heltec.display->drawString(0, 20, "Preset mode");
    }
    else {
      Heltec.display->drawString(0, 20, "Effect mode");    
    }
    Heltec.display->setFont(Roboto_Mono_Bold_52);
    Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
    Heltec.display->drawString(110, 12, String(selected_preset+1));
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);

    // Sanity length check on name string
    if (strlen(presets[5].Name) > MAXNAME){
       presets[5].Name[MAXNAME - 1]  = '\0';  // Rudely truncate
    }
    Heltec.display->drawString(0, 50, presets[5].Name);
    
    updateIcons();
    Heltec.display->display();
  }
}
