void updateIcons() {
  
  // Read RSSI from Spark
  iRSSI = ble_getRSSI();
            
  // Show BT icon if connected
  // Use graduated scale based on the following
  // 0 bars (very poor) < -95db
  // 1 bar (poor) = -75db to -95db
  // 2 bars (fair) = -60db to -75db
  // 3 bars (good) = -40db to -60db
  // 4 bars (excellent) = > -40db
  if(spark_state == SPARK_SYNCED){
    oled.drawXbm(btlogo_pos, 0, bt_width, bt_height, bt_bits);
    // Display BT RSSI icon depending on actual signal
    if (iRSSI > -40) {
      oled.drawXbm(rssi_pos, 0, rssi_width, rssi_height, rssi_4);
    }
    else if (iRSSI > -60) {
      oled.drawXbm(rssi_pos, 0, rssi_width, rssi_height, rssi_3);
    }
    else if (iRSSI > -75) {
      oled.drawXbm(rssi_pos, 0, rssi_width, rssi_height, rssi_2);
    }
     else if (iRSSI > -95) {
      oled.drawXbm(rssi_pos, 0, rssi_width, rssi_height, rssi_1);
    }
    // else no bars 
    
    // Update drive status icons once data available
    // Drive icon
    if (presets[5].effects[2].OnOff){
      oled.drawXbm(drive_pos, 0, icon_width, icon_height, drive_on_bits);
    }
    else {
      oled.drawXbm(drive_pos, 0, icon_width, icon_height, drive_off_bits);   
    }
    // Mod icon
    if (presets[5].effects[4].OnOff) {
      oled.drawXbm(mod_pos, 0, icon_width, icon_height, mod_on_bits);
    }
    else {
       oled.drawXbm(mod_pos, 0, icon_width, icon_height, mod_off_bits);   
    }
    // Delay icon
    if (presets[5].effects[5].OnOff) {
      oled.drawXbm(delay_pos, 0, icon_width, icon_height, delay_on_bits);
    }
    else {
       oled.drawXbm(delay_pos, 0, icon_width, icon_height, delay_off_bits);   
    }
    // Reverb icon
    if (presets[5].effects[6].OnOff) {
      oled.drawXbm(rev_pos, 0, icon_width, icon_height, rev_on_bits);
    }
    else {
       oled.drawXbm(rev_pos, 0, icon_width, icon_height, rev_off_bits);
    }
  }
  // Battery icon control - measured periodically via a 1s timer
  // Average readings to reduce noise
  if (isTimeout) {
    readBattery();
    isTimeout = false;
  }

  // Start by showing the empty icon. drawXBM writes OR on the screen so care
  // must be taken not to graphically block out some symbols. This is why the
  // battery full but not charging is the last in the chain.

   // No battery monitoring so just show the empty symbol
  if (batteryCharging()==-1) {
    oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat00_bits);
  }
  
  if (batteryCharging()==1) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, batcharging_bits);
  }
  
  
  // Basic battery detection available. Coarse cut-offs for visual 
  // guide to remaining capacity. Surprisingly complex feedback to user.
  // No bars = 0% (should not be discharged further)
  // Full symbol = >85%
  #ifndef BATT_CHECK_0
    else if (vbat_result < BATTERY_LOW) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat00_bits);
    }
    else if (vbat_result < BATTERY_10) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat10_bits);
    }
    else if (vbat_result < BATTERY_20) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat20_bits);
    }
    else if (vbat_result < BATTERY_30) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat30_bits);
    }    
    else if (vbat_result < BATTERY_40) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat40_bits);
    }
    else if (vbat_result < BATTERY_50) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat50_bits);
    }
    else if (vbat_result < BATTERY_60) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat60_bits);
    }
    else if (vbat_result < BATTERY_70) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat70_bits);
    }        
    else if (vbat_result < BATTERY_80) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat80_bits);
    }
    else if (vbat_result < BATTERY_90) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat90_bits);
    }
    else {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat100_bits);
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
       
      // Calculate the button press duration by subtracting the button time from the local time
      buttonPressDuration[i] = millis() - buttonTimer[i];
      
      // Mark the button as long-pressed if the button press duration exceeds the long press threshold
      // and is not already flagged as such
      if ((buttonPressDuration[i] > longPressThreshold) && (longPressActive[i] == false)) {
          longPressActive[i] = true;
          latchpress = true;
      }
    }
    // Else the button reads as LOW, so the button is released which means that  
    // the button either hasn't been pressed, or has just been released
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
        
        // Long press wasn't active. We either need to debounce/reject the press or register a normal/short press
        else {
          // if the button press duration exceeds our bounce threshold, then we register a short press
          if (buttonPressDuration[i] > debounceThreshold){
            sw_val[i] = HIGH;
          }
      }
        
        // Reset the button active status
        buttonActive[i] = false;
      }
      
    }  // The button either hasn't been pressed, or has just been released
    
  }  // Debounce and long press code loop

  // OR all the long press flags so any of the four main footswitches can switch modes
  AnylongPressActive = (longPressActive[0] || longPressActive[1] || longPressActive[2] || longPressActive[3]);
  AllPressActive = (longPressActive[0] && longPressActive[1] && longPressActive[2] && longPressActive[3]);

  // Have all buttons been held down? - toggle tuner mode
  if (AllPressActive && (latchpress == true)){
      Serial.println("Tuner mode");
      if (isTunerMode) {
        spark_msg_out.tuner_on_off(false);
      }
      else {
        spark_msg_out.tuner_on_off(true);
      }/*
      longPressActive[0] = false; // This doesn't work
      longPressActive[1] = false;
      longPressActive[2] = false;
      longPressActive[3] = false;*/
      latchpress = false;
  }
   
  // Has any button been held down?
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
    // Flip GUI flash bool
    if (isTimeout){
      flash_GUI = !flash_GUI;
    }
  
    // if a change has been made or the timer timed out and fully synched...
    if ((isOLEDUpdate || isTimeout) && (spark_state == SPARK_SYNCED)){
    isOLEDUpdate = false;  
    oled.clear();

    // Show tuner screen when requested by Spark
    if (isTunerMode) {
      
      // Default display - draw meter bitmap and label
      oled.drawXbm(0, Y5, tuner_width, tuner_height, tuner_bits); 
      oled.setFont(SMALL_FONT);
      oled.setTextAlignment(TEXT_ALIGN_LEFT);
      oled.drawString(0,0,"Tuner");
      
      // If nothing to show
      if (msg.val == -1.0) {
        oled.setTextAlignment(TEXT_ALIGN_RIGHT);
        oled.drawString(127,0,"...");
      }
      // If something to show
      else {
        // Work out start and end-points of meter needle
        metervalue = int16_t(msg.val * 128);            // Span tuner's 0-1.0, to 0-127
        if (metervalue > 127) metervalue = 127;         // Bounds check
        if (metervalue < 0) metervalue = 0;
        hubvalue = metervalue / 4;                      // Hub is 1/4 the size, so spans 0-32

        // Work out the Y-values of the needle (both start and end points)
        // Right-side calculations
        if (metervalue > 64)
        {
          hubvalue = hubvalue - 16;                     // Scale hub RHS from 16-0 to 0-16 
          hub_y = sqrt(256 - (hubvalue * hubvalue));    // Work out Y-value based on 16 pixel radius
          meter_x = metervalue - 64;                    // Scale meter RHS from 64-128 to 0-64
          meter_y = sqrt(tuner_scale - (meter_x * meter_x)); // Work out Y-value based on 64 (or 48) pixel radius
          oled.drawLine((hubvalue + 64), (64-hub_y), metervalue, (64-meter_y)); // Draw line from hub to meter edge
        }
        // Left-side calculations
        else
        {
          hubvalue = 16 - hubvalue;                     // Scale hub from 0-16 to 16-0
          hub_y = sqrt(256 - (hubvalue * hubvalue));    // Work out Y-value based on 16 pixel radius (0-16)
          meter_x = 64 - metervalue;                    // Scale meter RHS from 0-64 to 64-0
          meter_y = sqrt(tuner_scale - (meter_x * meter_x)); // Work out Y-value based on 64 (or 48) pixel radius (0-64)
          oled.drawLine(((16-hubvalue) + 48), (64-hub_y), metervalue, (64-meter_y)); // Draw line from hub to meter edge
        }

        meter_target = msg.param1; // Get note data

        // Show note names
        oled.setFont(MEDIUM_FONT);
        oled.setTextAlignment(TEXT_ALIGN_RIGHT);
        switch (meter_target) {
        case 0:
          oled.drawString(127, 0, "C");
          break;
        case 1:
          oled.drawString(127, 0, "C#");
          break;    
        case 2:
          oled.drawString(127, 0, "D");
          break;
        case 3:
          oled.drawString(127, 0, "D#");
          break;
        case 4:
          oled.drawString(127, 0, "E");
          break;
        case 5:
          oled.drawString(127, 0, "F");
          break;
        case 6:
          oled.drawString(127, 0, "F#");
          break;         
        case 7:
          oled.drawString(127, 0, "G");
          break;
        case 8:
          oled.drawString(127, 0, "G#");
          break;
        case 9:
          oled.drawString(127, 0, "A");
          break;
        case 10:
          oled.drawString(127, 0, "A#");
          break;
        case 11:
          oled.drawString(127, 0, "B");
          break;
        default:
          oled.drawString(127,0,"...");
          break;
        }
      }     
    }
    
    // Otherwise normal display
    else {
      oled.setFont(MEDIUM_FONT);
      oled.setTextAlignment(TEXT_ALIGN_LEFT);
      if (!isPedalMode) {
        oled.drawString(0, 20, "Preset mode");
      }
      else {
        oled.drawString(0, 20, "Effect mode");    
      }
      oled.setFont(HUGE_FONT);
      oled.setTextAlignment(TEXT_ALIGN_CENTER);
      
      if (flash_GUI || !setting_modified) {
        // In-joke to the "I saw 5 on the display!" folk
        if (display_preset_num > 3) {
          display_preset_num = 3; 
        }
        oled.drawString(110, 12, String(display_preset_num+1));
      }
      oled.setFont(SMALL_FONT);
      oled.setTextAlignment(TEXT_ALIGN_LEFT);

      // Truncate string so that it never extends into preset number
      strcpy(TempString, presets[5].Name);
      i = STR_LEN;
      while (oled.getStringWidth(TempString) > 90) {
        TempString[i-1]= '\0';  // Rudely truncate
        i--;
      }

      oled.drawString(0, 50, TempString);
  
      // Flash "Connect App" message when no app connected
      if (flash_GUI && !conn_status[APP]){
        oled.setFont(SMALL_FONT);
        oled.setTextAlignment(TEXT_ALIGN_LEFT);
        oled.drawString(15, 37, "Connect App");
      } 
    
      updateIcons();
    }
    oled.display();
  }
  
  if (!connected_sp) {
    // Show reconnection message
    oled.clear();
    oled.setFont(MEDIUM_FONT);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    oled.drawString(X1, Y3, "Reconnecting");
    oled.setFont(MEDIUM_FONT);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    oled.drawString(X1, Y4, "Please wait");
    oled.display();
    if (attempt_count > MAX_ATTEMPTS) {
      ESP_off();
    }
    attempt_count++;
  } else {
    attempt_count = 0;
  }
}

void readBattery(){
vbat_result = analogRead(VBAT_AIN); // Read battery voltage

    // To speed up the display when a battery is connected from scratch
    // ignore/fudge any readings below the lower threshold
    if (vbat_result < BATTERY_LOW) vbat_result = BATTERY_LOW;
    temp = vbat_result;

    // While collecting data
    if (vbat_ring_count < VBAT_NUM) {
      vbat_ring_sum += vbat_result;
      vbat_ring_count++;
      vbat_result = vbat_ring_sum / vbat_ring_count;
    }
    // Once enough is gathered, do a decimating average
    else {
      vbat_ring_sum *= 0.9;
      vbat_ring_sum += vbat_result;
      vbat_result = vbat_ring_sum / VBAT_NUM;
    }

    chrg_result = analogRead(CHRG_AIN); // Check state of /CHRG output  
}

int batteryPercent(int vbat_value) {
  return constrain(map(vbat_value, BATTERY_LOW, BATTERY_100, 0, 100), 0, 100);  
}

int batteryCharging() {
  #ifdef BATT_CHECK_0
    return -1; //unsupported
  #endif
  
  // For level-based charge detection (not very reliable)
  #ifdef BATT_CHECK_1
    if (vbat_result >= BATTERY_CHRG) {
      return 1;
    } else {
      return 0;
    }
  #endif
  
  // If advanced charge detection available, and charge detected
  #ifdef BATT_CHECK_2
    if (chrg_result < CHRG_LOW) {
      return 1;
    } else {
      return 0;
    }
  #endif  
}

void textAnimation(const String &s, ulong msDelay, int yShift=0, bool show=true) {  
    oled.clear();
    oled.drawString(oled.width()/2, oled.height()/2-6 + yShift, s);
    if (show) {
      oled.display();
      delay(msDelay);
    }
}

void ESP_off(){
  bool can_deep_sleep;
  can_deep_sleep = Check_RTC(); // Find out if we have RTC pins assigned to buttons allowing deep sleep, otherwise we use light sleep
  //  CRT-off effect =) or something
  String s = "_________________";
  oled.clear();
  oled.display();
  oled.setFont(MEDIUM_FONT);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  if (can_deep_sleep) {
  //  Only GPIOs which have RTC functionality can be used: 0,2,4,12-15,25-27,32-39
    int k;
    for (k = 0; k < NUM_SWITCHES; ++k) {
      if (sw_RTC[k]) {
        break;        
      }
    }
    textAnimation("Button" + String(k+1) + " wakes up",5000);
    textAnimation("... z-z-Z-Z",1000);
    for (int i=0; i<8; i++) {
      s = s.substring(i);
      textAnimation(s,70,-6);
    }
    for (int i=0; i<10; i++) {
      textAnimation("\\",30);
      textAnimation("|",30);
      textAnimation("/",30);
      textAnimation("--",30);
    }
    DEBUG("deep sleep");
    oled.displayOff();                // turn it off, otherwise oled remains active
    esp_sleep_enable_ext0_wakeup(static_cast <gpio_num_t> (sw_pin[0]), HIGH); // wake up if BUTTON 1 pressed
    esp_deep_sleep_start();
  } else {
    textAnimation("Button1 wakes up",5000);
    textAnimation("..z-Z-Z",1000);
    for (int i=0; i<8; i++) {
      s = s.substring(i);
      textAnimation(s,70,-6);
    }
    for (int i=0; i<10; i++) {
      textAnimation("\\",30);
      textAnimation("|",30);
      textAnimation("/",30);
      textAnimation("--",30);
    }
    DEBUG("light sleep");
    oled.displayOff();                // turn it off, otherwise oled remains active
    esp_sleep_enable_gpio_wakeup();
    gpio_wakeup_enable(static_cast <gpio_num_t> (sw_pin[0]), GPIO_INTR_HIGH_LEVEL );
    esp_light_sleep_start();
  }
};

void ESP_on () {
  oled.setFont(MEDIUM_FONT);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  //oled.setContrast(100);
  textAnimation(".",200,-4);
  textAnimation("*",100,5);
  textAnimation("X",100,2);
  textAnimation("-}|{-",100);
  textAnimation("- -X- -",100,2);
  textAnimation("x",100,0);
  textAnimation(".",200,-4);
}

bool Check_RTC() {
  bool RTC_present = false;
  for(int i = 0; i < NUM_SWITCHES; ++i) {
    sw_RTC[i] = false;
    for(int j = 0; j < (sizeof(RTC_pins)/sizeof(RTC_pins[0])); ++j) {
      if(sw_pin[i] == RTC_pins[j]) {
        sw_RTC[i] = true;
        RTC_present = true;
        DEBUG("RTC pin: " + String(sw_pin[i]));
        break;
      }
    }    
  }
  DEBUG ("RTC pin present: " + String(RTC_present));
  return RTC_present;
}