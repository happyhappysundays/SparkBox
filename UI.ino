// Overlay static graphics ============================================================
void screenOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  int visibleLeft = (CONN_ICON_WIDTH+1)*2;    // calculate the place to show compact scrolling name at the top line
  int visibleW = display->width() - BAT_WIDTH - visibleLeft - 1;
  if (isTimeout) {
    readBattery();  // Read analog voltage and average it
    isTimeout = false;
  }  
  if ( curMode==MODE_PRESETS || curMode==MODE_BYPASS) {
    display->setColor(BLACK);
    display->fillRect(visibleLeft, 0, display->width()-BAT_WIDTH-visibleLeft, STATUS_HEIGHT);
    fxIcons();
  }
  display->setColor(BLACK);
  display->fillRect(0, 0, visibleLeft, STATUS_HEIGHT);
  display->fillRect(display->width()-BAT_WIDTH-1, 0, BAT_WIDTH+1, STATUS_HEIGHT);
  mainIcons();
  
}

// frSomething functions are frame drawing of the UI

// PRESETS MODE =======================================================================
void frPresets(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  static int scrollStep = -2; // speed of horiz scrolling tone names
  static ulong scrollCounter;
#ifndef STALE_NUMBER
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->setFont(HUGE_FONT);
    int numW = display->getStringWidth(String(display_preset_num + 1))+5;
    display->setFont(BIG_FONT);
    int nameW = display->getStringWidth(presets[CUR_EDITING].Name)+5;
    if (numW+nameW <= display->width()) {
      scroller = ( display->width() - numW - nameW ) / 2;
    } else {
      if ( millis() > scrollCounter ) {
        scroller = scroller + scrollStep;
        if (scroller < (int)(display->width())-numW-nameW-numW-nameW) {
          scroller = scroller + numW + nameW;
        }
        scrollCounter = millis() + 20;
      }
      display->setFont(HUGE_FONT);
      display->drawString( x + scroller + numW + nameW, 11 + y, String(display_preset_num + 1) ); // +1 for humans
      display->setFont(BIG_FONT);
      display->drawString(x + scroller + numW + nameW + numW, y + display->height()/2 - 6 ,presets[CUR_EDITING].Name);
    }
    display->setFont(HUGE_FONT);
    display->drawString( x + scroller, 11 + y, String(display_preset_num + 1) ); // +1 for humans
    display->setFont(BIG_FONT);
    display->drawString(x + scroller + numW, y + display->height()/2 - 6 ,presets[CUR_EDITING].Name);
#else
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->setColor(WHITE);
    display->setFont(HUGE_FONT);
    int numW = display->getStringWidth(String(display_preset_num + 1))+5;
    display->setFont(BIG_FONT);
    int nameW = display->getStringWidth(presets[CUR_EDITING].Name)+5;
    if (numW+nameW <= display->width()) {
      scroller = ( display->width() - numW ) / 2;
      display->setTextAlignment(TEXT_ALIGN_CENTER);
    } else {
      display->setTextAlignment(TEXT_ALIGN_LEFT);
      if ( millis() > scrollCounter ) {
        scroller = scroller + scrollStep;
        if (scroller < 0) {
          scroller = scroller + nameW;
        }
        scrollCounter = millis() + 20;
      }
      display->setFont(BIG_FONT);
      display->drawString( x + scroller - nameW , y + display->height()/2 - 6 ,presets[CUR_EDITING].Name);
    }
    display->setFont(BIG_FONT);
    display->drawString( x + scroller , y + display->height()/2 - 6 ,presets[CUR_EDITING].Name);
    display->setColor(BLACK);
    display->fillRect(display->width()-numW+x+2, STATUS_HEIGHT+y, numW-2, display->height()-STATUS_HEIGHT);
    display->setFont(HUGE_FONT);
    display->setColor(WHITE);
    display->setTextAlignment(TEXT_ALIGN_RIGHT);
    display->drawString( display->width()+x, STATUS_HEIGHT-5+y, String(display_preset_num + 1) ); // +1 for humans
#endif
}

// EFFECTS MODE =======================================================================
void frEffects(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  static int scrollStep = -1;     // speed of horiz scrolling tone names
  static ulong scrollCounter;
  int visibleLeft = (CONN_ICON_WIDTH+1)*2;    // calculate the place to show compact scrolling name at the top line
  int visibleW = display->width() - BAT_WIDTH - visibleLeft - 1;
  
  fxHugeIcons(x,y);                              // Big FX icons
  
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(SMALL_FONT);
  int numW = display->getStringWidth(String(display_preset_num + 1))+5;  // Width of the 1st string representing the preset number
  int nameW = display->getStringWidth(presets[CUR_EDITING].Name)+5;       // Width of the 2nd string representing the name of the preset
  if (numW+nameW <= visibleW) {
    scroller = ( visibleW - numW - nameW ) / 2;
  } else {
    if ( millis() > scrollCounter ) {
      scroller = scroller + scrollStep;
      if (scroller < (int)(visibleW)-numW-nameW-numW-nameW) {
        scroller = scroller + numW + nameW;
      }
      scrollCounter = millis() + 20;
    }
    
    display->drawString( visibleLeft + x + scroller + numW + nameW,  y, String(display_preset_num + 1) ); // +1 for humans
    display->drawString( visibleLeft + x + scroller + numW + nameW + numW, y, presets[CUR_EDITING].Name);
  }
  display->drawString( visibleLeft + x + scroller,   y, String(display_preset_num + 1) ); // +1 for humans
  display->drawString( visibleLeft + x + scroller + numW, y, presets[CUR_EDITING].Name);
  // Mask left and right space for the top line status icons
  display->setColor(BLACK);

}

// SCENES MODE =======================================================================
void frScenes(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
}


// BYPASS MODE =======================================================================
void frBypass(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(BIG_FONT);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(display->width()/2 + x, 20 + y, "BYPASS" );
}

// MESSAGE MODE =======================================================================
void frMessage(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // splash screen with a text message
  display->setColor(WHITE);
  display->setFont(SMALL_FONT);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(display->width()/2 + x,  y, msgCaption);
  
  display->setFont(HUGE_FONT);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  if(display->getStringWidth(msgText)>display->width()) {
    display->setFont(BIG_FONT);
    if(display->getStringWidth(msgText)>display->width()) {
      display->setFont(MEDIUM_FONT);
    }
  }
  display->drawString((display->width())/2 + x, Y1 + y , String(msgText) );
}

// FX LEVEL MODE ======================================================================
void frLevel(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // indicates parameter change
  display->setColor(WHITE);
  display->setFont(SMALL_FONT);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(display->width()/2 + x,  y, fxCaption);
  
  display->setFont(HUGE_FONT);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  sprintf(str,"%3.1f",(float)(level)/10);
  if(display->getStringWidth(str)>display->width()) {
    display->setFont(BIG_FONT);
  }
  display->drawString((display->width())/2 + x, 11 + y , String(str) );
}

// TUNER MODE =======================================================================
void frTuner(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  String note_names[] = {"...","C","C#","D","D#","E","F","F#","G","G#","A","A#","B","..."};
  //String note_names[] = {"...","Do","Do#","Re","Re#","Mi","Fa","Fa#","Sol","Sol#","La","La#","Si","..."};
  int16_t val_deg = 0;
  int16_t meter_x = 0;
  int16_t meter_y = 0;
  int16_t hub_x = 0;
  int16_t hub_y = 0;
  float    test_val;
  // Show tuner screen when requested by Spark
  display->clear();
  // Default display - draw meter bitmap and label
  display->drawXbm(0+x, Y5+y, tuner_width, tuner_height, tuner_bits); 
  //display->setTextAlignment(TEXT_ALIGN_LEFT);
  //display->drawString(0,0,"Tuner");
  test_val = msg.val;
  //test_val = (millis() % 2000)/2000.0;
  display->setFont(MEDIUM_FONT);
  display->setTextAlignment(TEXT_ALIGN_CENTER);

  if (test_val != -1.0) {
    // Show note names
    display->drawString(display->width()/2+x,note_y+y,note_names[constrain(msg.param1+1,0,13)]); // The first and the last note_names[] are '...' such we name everything outside the bounds
    display->drawString(display->width()/2+x-1,note_y+y,note_names[constrain(msg.param1+1,0,13)]); // Fake bold
    // If something to show
    // Work out start and end-points of meter needle
    val_deg = constrain(int16_t(test_val * 180.0), 0, 180);            // Span tuner's 0-1.0, to 0-180 degrees
    meter_x = (tuner_width/2) - (tuner_width/2)*sin(radians(90-val_deg));
    meter_y = (display->height()) - (tuner_height)*cos(radians(90-val_deg))+ (display->height()-tuner_height*tuner_share/4);
    hub_x = (tuner_width/2) - (tuner_width/2/4)*sin(radians(90-val_deg));
    hub_y = (display->height()) - (display->height()/4)*cos(radians(90-val_deg)) ;
    display->drawLine(meter_x+x, meter_y+y, hub_x+x, hub_y+y); // Draw line from hub to meter edge
  } else {
    // Nothing to show
    display->drawString(display->width()/2+x,note_y+y,"..."); // Not detected
  }
}

void mainIcons() {
  // Spark Amp BT connection icon
  drawStatusIcon(s_bt_bits, s_bt_width, STATUS_HEIGHT, 0,                 0, CONN_ICON_WIDTH, STATUS_HEIGHT, spark_state==SPARK_SYNCED);
  // App connection icon
  drawStatusIcon(a_bt_bits, a_bt_width, STATUS_HEIGHT, CONN_ICON_WIDTH+1, 0, CONN_ICON_WIDTH, STATUS_HEIGHT, conn_status[APP]);
  // Battery icon
  drawBatteryH(oled.width()-BAT_WIDTH, 0, BAT_WIDTH, STATUS_HEIGHT, batteryPercent(vbat_result), batteryCharging()); 
}

void fxIcons() {
  // Drive icon    
  drawStatusIcon(dr_bits, dr_width, STATUS_HEIGHT, 2*(CONN_ICON_WIDTH+1)+1,                     0, FX_ICON_WIDTH,  STATUS_HEIGHT, presets[CUR_EDITING].effects[FX_DRIVE].OnOff);
  // Mod icon
  drawStatusIcon(md_bits, md_width, STATUS_HEIGHT, 2*(CONN_ICON_WIDTH+1)+1+FX_ICON_WIDTH+1,     0, FX_ICON_WIDTH,  STATUS_HEIGHT, presets[CUR_EDITING].effects[FX_MOD].OnOff);
  // Delay icon
  drawStatusIcon(dy_bits, dy_width, STATUS_HEIGHT, 2*(CONN_ICON_WIDTH+1)+1+(FX_ICON_WIDTH+1)*2, 0, FX_ICON_WIDTH,  STATUS_HEIGHT, presets[CUR_EDITING].effects[FX_DELAY].OnOff);
  // Reverb icon
  drawStatusIcon(rv_bits, rv_width, STATUS_HEIGHT, 2*(CONN_ICON_WIDTH+1)+1+(FX_ICON_WIDTH+1)*3, 0, FX_ICON_WIDTH,  STATUS_HEIGHT, presets[CUR_EDITING].effects[FX_REVERB].OnOff);
}

void fxHugeIcons(int x, int y) {
  // Drive icon    
  drawTextIcon("Dr", x+0,  y+18, 30, 32, presets[CUR_EDITING].effects[FX_DRIVE].OnOff);
  // Mod icon
  drawTextIcon("Md", x+32, y+18, 30, 32, presets[CUR_EDITING].effects[FX_MOD].OnOff);
  // Delay icon
  drawTextIcon("Dy", x+64, y+18, 30, 32, presets[CUR_EDITING].effects[FX_DELAY].OnOff);
  // Reverb icon
  drawTextIcon("Rv", x+96, y+18, 30, 32, presets[CUR_EDITING].effects[FX_REVERB].OnOff);
}

// Print out the requested preset data
void dump_preset(SparkPreset preset) {
  int i,j;

  DEBUG(preset.curr_preset); DEBUG(" ");
  DEBUG(preset.preset_num); DEBUG(" ");
  DEBUG(preset.Name); DEBUG(" ");
  DEBUG(preset.Description);

  for (j=0; j<7; j++) {
    DEBUG("    ");
    DEBUG(preset.effects[j].EffectName); DEBUG(" ");
    if (preset.effects[j].OnOff == true) DEBUG(" On "); else DEBUG (" Off ");
    for (i = 0; i < preset.effects[j].NumParameters; i++) {
      DEBUG(preset.effects[j].Parameters[i]); DEBUG(" ");
    }
    DEBUG();
  }
}

// cycle through knobs (like they are on the Spark Amp)
void changeKnobFx(int changeDirection=1) {
  curKnob = curKnob + changeDirection;
  if (curKnob>=knobs_number) curKnob=0;
  if (curKnob<0) curKnob=knobs_number-1;
  curFx = knobs_order[curKnob].fxSlot;
  curParam = knobs_order[curKnob].fxNumber;
  fxCaption = spark_knobs[curFx][curParam];
  level = presets[CUR_EDITING].effects[curFx].Parameters[curParam] * MAX_LEVEL;
  timeToGoBack = millis() + actual_timeout;
  DEBUG(curKnob);
}


// Pushbutton handling
void doPushButtons(void)
{
  static unsigned long buttonTimer[NUM_SWITCHES];         // stores the time that the button was pressed (relative to boot time)
  static unsigned long buttonPressDuration[NUM_SWITCHES]; // stores the duration (in milliseconds) that the button was pressed/held down for
  static unsigned long autoFireTimer = 0;
  static bool buttonActive[NUM_SWITCHES];                 // indicates if the button is active/pressed
  static bool longPressActive[NUM_SWITCHES];              // indicates if the button has been long-pressed
  static bool buttonClick[NUM_SWITCHES];                  // indicates if the button has been clicked
  static bool longPressFired = false;                     // indicates if the long-press event has fired
  bool AnylongPressActive = false;                        // OR of any longPressActive states
  bool AllPressActive = true;                             // AND of any longPressActive states
  uint8_t ClickFlags = 0;                                 // Write buttons states to one binary mask variable
  uint8_t LongPressFlags = 0;                             // Write buttons states to one binary mask variable
  static uint8_t zeroCounter = 0;
  static uint8_t oldActiveFlags = 0;
  static uint8_t maxFlags = 0;
  ActiveFlags = 0;
  // Debounce and long press code
  for (int i = 0; i < NUM_SWITCHES; i++) {
    // If the button pin reads ON, the button is pressed
    if (digitalRead(switchPins[i]) == logicON)
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
          longPressFired = false;
      }
    }
    // The button either hasn't been pressed, or has just been released
    else { // The button state is LOW
      // Reset switch register here so that switch is not repeated    
      buttonClick[i] = false; 
      // If the button was marked as active, it was recently pressed
      if (buttonActive[i] == true){
        
        // Reset the long press active state
        if (longPressActive[i] == true){
          longPressActive[i] = false;
          longPressFired = true;
        }
        
        // Long press wasn't active. We either need to debounce/reject the press or register a normal tap
        else
        {
          // if the button press duration exceeds our bounce threshold, then we register a tap
          if (buttonPressDuration[i] > debounceThreshold){
            buttonClick[i] = true;
            DEBUG("Tap " + String(1<<i));
            onTap(1<<i);
          }
        }
        
        // Reset the button active status
        buttonActive[i] = false;
      }
      
    }  // The button either hasn't been pressed, or has just been released
    LongPressFlags += (static_cast <uint8_t> (longPressActive[i])) << i;
    ActiveFlags += (static_cast <uint8_t> (buttonActive[i])) << i;
    ClickFlags += (static_cast <uint8_t> (buttonClick[i])) << i;
    
  }  // Debounce and long press code loop

  // OR all the long press flags so any of the four main footswitches can switch modes
  AnylongPressActive = (LongPressFlags > 0) ;
  AllPressActive = (LongPressFlags == ((1 << NUM_SWITCHES)-1)) ;
  if (oldActiveFlags == 0 && ActiveFlags == 0) {
    zeroCounter++; // Idle counter to drop maxFlags when there's actually no activity
    if (zeroCounter>10) {maxFlags = 0;}
  }
  if (oldActiveFlags != ActiveFlags) {
//    DEBUG(ActiveFlags);
    oldActiveFlags = ActiveFlags;
    maxFlags = max(maxFlags, ActiveFlags);    
  }
  if (LongPressFlags >0 && LongPressFlags==ActiveFlags && !longPressFired){
    DEBUG("Long press " + String(LongPressFlags));
    longPressFired = true;  // In case when the next function is async and time consuming, 
                            // let's flush it here not to call the function twice or more in a row
    onLongPress(LongPressFlags);  // function to execute on Long Press event 
  } else if (LongPressFlags >0 && LongPressFlags==ActiveFlags && autoFireEnabled) { //Autofire 
    if (autoFireTimer < millis()-autoFireDelay) { 
      DEBUG("AutoFire " + String(LongPressFlags));
      autoFireTimer = millis();
      onAutoClick(LongPressFlags);  // function to execute on Long Press event 
    }
  }
  if (ClickFlags > 0 && ActiveFlags ==0){
    DEBUG("Click " + String(maxFlags));
    onClick(maxFlags);      // This will give you multi-button clicks
  //  onClick(clickFlags);  // This will give only single button at a time to be clicked
  }
}

// buttonMask is binary mask that has 1 in Nth position, if Nth button is active, 
// say 0b00000100 (decimal 4) means that your 3rd button fired this event, multiple buttons allowed
void onClick(uint8_t buttonMask) {
  // In Preset mode, use the four buttons to select the four HW presets
  uint8_t buttonId;
  if (isTunerMode) {
    // bail out
    tunerOff();
  } else if (curMode == MODE_BYPASS) {
    // bail out
    bypassOff();
  } else if (curMode == MODE_PRESETS) {
  // Mode PRESETS
    switch (buttonMask) {
      case 1: // button 1
      case 2: // button 2
      case 4: // button 3
      case 8: // button 4      
      case 16:// ...
      case 32:// ...
      case 64:// ...
        buttonId = log(buttonMask)/log(2); 
        change_hardware_preset(buttonId);
        display_preset_num = buttonId;
        break;
      default:
        //no action yet
        break;        
    }
  } else if (curMode == MODE_EFFECTS) {
  // Mode EFFECTS
    for(int i = 0; i<NUM_SWITCHES; ++i) {
      if(bitRead(buttonMask,i)==1){
        SWITCHES[i].fxOnOff = !SWITCHES[i].fxOnOff;
        change_generic_onoff(SWITCHES[i].fxSlotNumber, SWITCHES[i].fxOnOff);
        setting_modified = true;
      }
    }
  } else if (curMode == MODE_LEVEL && (buttonMask==2 || buttonMask==8)) {
// Effect level adjustment with buttons 2 and 4
    timeToGoBack = millis() + actual_timeout; // Prolongue the Mode as we are not idle
    curFx = knobs_order[curKnob].fxSlot;
    curParam = knobs_order[curKnob].fxNumber;
    fxCaption = spark_knobs[curFx][curParam];
    level = presets[CUR_EDITING].effects[curFx].Parameters[curParam] * MAX_LEVEL;
    DEBUG(level);
    if (buttonMask==8) {
      level=level+1;
      if (level>MAX_LEVEL) {level = MAX_LEVEL;}
    } else {
      level=level-1;
      if (level<0) {level=0;}
    }
    float newVal = (float)level/(float)MAX_LEVEL + 0.005;
  //  DEBUG(newVal);
   // DEBUG(String(presets[CUR_EDITING].effects[curFx].EffectName) + " " + String(newVal) );
    change_generic_param(curFx, curParam, newVal);
    presets[CUR_EDITING].effects[curFx].Parameters[curParam] = newVal;
  } else if (curMode == MODE_LEVEL && buttonMask == 1) {
    changeKnobFx();
  }
}

// buttonMask is binary mask that has 1 in Nth position, if Nth button is active, 
// say 0b00000110 (decimal 6) means that your 2nd and 3rd button were pressed
void onLongPress(uint8_t buttonMask) {
  if (isTunerMode) {
    // bail out
    tunerOff();
  } else if (curMode == MODE_BYPASS) {
    // bail out
    bypassOff();
  } else {
    switch (buttonMask) {
      case 1: // button 1
        cycleModes(); // Change current mode in cycle
        break;
      case 3: // buttons 1 an 2
        toggleTuner();
        break;
      case 4: // button 3
        if (!tempUI) {
          autoFireEnabled = true;
          curFx = knobs_order[curKnob].fxSlot;
          curParam = knobs_order[curKnob].fxNumber;
          fxCaption = spark_knobs[curFx][curParam];
          level = presets[CUR_EDITING].effects[curFx].Parameters[curParam] * MAX_LEVEL;
          tempFrame(MODE_LEVEL, curMode, FRAME_TIMEOUT); // Master level adj
        } else {
          tempUI=false;
          autoFireEnabled = false;
          change_custom_preset(&presets[CUR_EDITING], display_preset_num);
          msgCaption = "CHANGES";
          msgText = "SAVED TO AMP";
          tempFrame(MODE_MESSAGE, returnMode, 1000);
        }
        break;
      case 12: // buttons 2 an 3
        toggleBypass();
        break;
      default:
        //no action yet
        break;
    }
  }
}

void onAutoClick(uint8_t buttonMask) {
  if (curMode==MODE_LEVEL && (buttonMask==2 || buttonMask==8)) {
    onClick(buttonMask);    // inc/dec fx level with buttons 2 and 4
  }
}

void onTap(uint8_t buttonMask) {
  //DEBUG("TAP " + String(buttonMask));
  //timeToGoBack = millis() + actual_timeout;  
}

// Cycle through first {NUM_MODES} modes
void cycleModes() {
  uint8_t iCurMode;
  if (isTunerMode) {
    tunerOff();
  } else if (curMode == MODE_BYPASS) {
    bypassOff();
  } else {
    returnToMainUI();
    iCurMode = static_cast <uint8_t> (curMode);
    iCurMode++;
    if (iCurMode >= NUM_MODES) {iCurMode = 0;}
    curMode = static_cast <eMode_t> (iCurMode);
    updateFxStatuses();
    DEBUG("Mode: " + String(curMode));
  }
}


// Refresh UI ============================================================================
void refreshUI(void) {
  // If some button is active, ploceed with a temp frame  
  if (ActiveFlags > 0) {
    timeToGoBack = millis() + actual_timeout;
  }

  // maybe it's time to return from a temp UI
  if ((millis() > timeToGoBack) && tempUI) {
    returnToMainUI();
  }
  
  // Flip GUI flash bool
  if (isTimeout) {
    flash_GUI = !flash_GUI;
  }

  if (isTunerMode) { // If Spark reports that we are in tuner mode
    if (curMode!=MODE_TUNER) { // We have to switch the pedal to tuner also
      returnMode = curMode;
      curMode = MODE_TUNER;
    }
  } else {
    if (curMode == MODE_TUNER) { //as this is async operation, we have to sync the pedal again
      curMode = returnMode;
    }
  }

  if (oldMode!=curMode) {
    switch (oldMode) {
      case MODE_PRESETS:
        ui.setFrameAnimation(SLIDE_DOWN);
        break;
      case MODE_EFFECTS:
        ui.setFrameAnimation(SLIDE_DOWN);
        break;
      case MODE_LEVEL:
        ui.setFrameAnimation(SLIDE_DOWN);
        break;
      default:
        break;
    }
    if (tempUI) {ui.setFrameAnimation(SLIDE_UP);}    
    ui.transitionToFrame(curMode);
    updateFxStatuses();
    oldMode = curMode;
    DEBUG("Switch to mode: " + String(curMode));
  }
/*  
  // if a change has been made or the timer timed out and fully synched...
  if ((isOLEDUpdate || isTimeout) && (spark_state == SPARK_SYNCED)){
    isOLEDUpdate = false;  
  }
*/  
  if (!connected_sp) {
    // Show reconnection message
    oled.clear();
    oled.setFont(MEDIUM_FONT);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    oled.drawString(X1, Y3, "Reconnecting");
    oled.setFont(MEDIUM_FONT);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    oled.drawString(X1, Y4, "Please wait");
    mainIcons();
    oled.display();
    delay(10);

#ifndef NOSLEEP
    if (millis() > time_to_sleep) {
      ESP_off();
    }
#endif    

  } else {
    time_to_sleep = millis() + (MAX_ATTEMPTS * MILLIS_PER_ATTEMPT);
  }
}

void drawStatusIcon(uint8_t* xbmVar, int xbmW, int xbmH, int x, int y, int w, int h, bool active) {
  // draw active or inactive icon placeholder
  oled.setColor(WHITE);
  if (active) {
    oled.fillRect(x, y, w, h);
  } else {
    oled.setColor(INVERSE);     // Rounded inactive icon borders or comment this line out for simple corners
    oled.drawRect(x, y, w, h);  // Comment this line out if you want inactive icons without borders
  }
  // draw letters and signs within
  oled.setColor(INVERSE);
  oled.drawXbm(x+(w-xbmW)/2, y+(h-xbmH)/2, xbmW, xbmH, xbmVar);
}


void drawTextIcon(const String &text, int x, int y, int w, int h, bool active) {
  int16_t yOffset = -10;
  oled.setFont(MEDIUM_FONT);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  // draw letters and signs within
  oled.setColor(WHITE);
  oled.drawString(x+w/2, y+h/2+yOffset, text);
  oled.drawString(1+x+w/2, y+h/2+yOffset, text);
  // draw active or inactive icon placeholder
  oled.setColor(INVERSE);
  if (active) {
    oled.fillRect(x, y, w, h);
  } else {
    oled.setColor(INVERSE);     // INVERSE = rounded inactive icon borders, WHITE = corners
    oled.drawRect(x, y, w, h);  // Comment this line out if you want inactive icons without borders
  }
}


void drawBatteryH(int x, int y, int w, int h, int chg_percent, bool charging) {
  //draw gauge
  oled.setColor(WHITE);
  oled.fillRect(x+2, y+2, constrain(map(chg_percent, 0, 100, 0, w-4), 0, (w*0.9)-3), h-4); // not to draw on the cap we use constrain
  oled.fillRect(x+2, y+2+(h/4)-1, map(chg_percent, 0, 100, 0, w-4), h/2-2); // narrow gauge can draw on the cap
  oled.setColor(INVERSE);
  if (charging) {
    chg_percent = 100; // overwrite because we don't really measure the process of charging
    oled.drawXbm(x+((w-chrg_width)/2), y, chrg_width, chrg_height, chrg_bits);
  }
  //draw contour
  oled.setColor(WHITE);
  oled.drawLine(x, y, x+(w*0.9), y);
  oled.drawLine(x+(w*0.9), y, x+(w*0.9), y+(h/4)-1);
  oled.drawLine(x+(w*0.9), y+(h/4)-1, x+w-1, y+(h/4)-1);
  oled.drawLine(x+w-1, y+(h/4)-1, x+w-1, y+(h-h/4));
  oled.drawLine(x+(w*0.9), y+(h-h/4), x+w-1, y+(h-h/4));
  oled.drawLine(x+(w*0.9), y+(h-h/4), x+(w*0.9), y+h-1);
  oled.drawLine(x, y+h-1, x+(w*0.9), y+h-1);
  oled.drawLine(x, y, x, y+h-1);
}

// a simple script to show a text string during a given time
void textAnimation(const String &s, ulong msDelay, int yShift=0, bool show=true) {  
  oled.clear();
  oled.drawString(oled.width()/2, oled.height()/2-6 + yShift, s);
  if (show) {
    oled.display();
    delay(msDelay);
  }
}

void readBattery(){
  vbat_result = analogRead(VBAT_AIN); // Read battery voltage
  // To speed up the display when a battery is connected from scratch
  // ignore/fudge any readings below the lower threshold
  if (vbat_result < BATTERY_LOW * ADC_COEFF) {
    vbat_result = BATTERY_LOW * ADC_COEFF;
  }
  // While collecting data
  if (vbat_ring_count < VBAT_NUM) {
    vbat_ring_sum += vbat_result;
    vbat_ring_count++;
    vbat_result = vbat_ring_sum / vbat_ring_count;
  }
  // Once enough is gathered, do a decimating average
  else {
    vbat_ring_sum = (VBAT_NUM-1) * vbat_ring_sum / VBAT_NUM + vbat_result;
    vbat_result = vbat_ring_sum / VBAT_NUM;
    #ifndef NOSLEEP
      #ifndef BATT_CHECK_0
        // Low-battery go to sleep to save the LiPo's life
        if ((vbat_result < BATTERY_LOW * ADC_COEFF) && (batteryCharging()<=0)) {
          oled.clear();
          oled.setFont(MEDIUM_FONT);
          oled.setColor(WHITE);
          textAnimation("LOW BATTERY", 5000);
          ESP_off();
        }
      #endif
    #endif
  }
  chrg_result = analogRead(CHRG_AIN); // Check state of /CHRG output  
  //DEBUG(vbat_result/ADC_COEFF);
}

// Calc the percentage
int batteryPercent(int vbat_value) {
  return constrain(map(vbat_value, BATTERY_LOW*ADC_COEFF, BATTERY_100*ADC_COEFF, 0, 100), 0, 100);  
}

// Charging or not?
int batteryCharging() {
  #ifdef BATT_CHECK_0
    return -1; //unsupported
  #endif
  
  // For level-based charge detection (not very reliable)
  #ifdef BATT_CHECK_1
    if (vbat_result >= BATTERY_CHRG*ADC_COEFF) {
      return 1;
    } else {
      return 0;
    }
  #endif
  
  // If advanced charge detection available, and charge detected
  #ifdef BATT_CHECK_2
    if (chrg_result < CHRG_LOW*ADC_COEFF) {
      return 1;
    } else {
      return 0;
    }
  #endif  
}

// Stand by mode with some fun
void ESP_off(){
  uint64_t bit_mask=0;
  String wake_buttons;
  int deep_sleep_pins;
  int k;
  deep_sleep_pins = Check_RTC();  // Find out if we have RTC pins assigned to buttons allowing deep sleep, otherwise we use light sleep
                                  //  CRT-off effect =) or something
  String s = "_________________";

  // Debug
  DEBUG("Deep_sleep_pins = ");
  DEBUG(deep_sleep_pins);
  DEBUG(" RTC_present = ");
  DEBUG(RTC_present);
    
  if (deep_sleep_pins > 0){
    oled.clear();
    oled.display();
    oled.setFont(MEDIUM_FONT);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
   
    //  Only GPIOs which have RTC functionality can be used for deep sleep: 0,2,4,12-15,25-27,32-39
  
    //  Future radio shutdown support here:
    //  esp_bluedroid_disable(); //gracefully shutdoun BT (and WiFi)
    //  esp_bt_controller_disable();
    //  esp_wifi_stop();

    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    #ifdef ACTIVE_HIGH
      for (i = 0; i < NUM_SWITCHES; i++) {
        if (sw_RTC[i]) {
          gpio_pullup_dis(static_cast <gpio_num_t> (switchPins[i]));
          gpio_pulldown_en(static_cast <gpio_num_t> (switchPins[i]));
          bit_mask += 1<<switchPins[i];
          wake_buttons += String(i+1) + ",";
          k = i+1;
        }
      }
      if (deep_sleep_pins == NUM_SWITCHES) {
        wake_buttons = "Any button wakes";
      } else {
        if (deep_sleep_pins == 1) {
          wake_buttons = "Button " + String(k) + " wakes";
        } else {
          wake_buttons = "Buttons " + wake_buttons.substring(0, wake_buttons.length()-1) + " wake";
        }
      }
    #else
      gpio_pulldown_dis(static_cast <gpio_num_t> (switchPins[RTC_1st]));
      gpio_pullup_en(static_cast <gpio_num_t> (switchPins[RTC_1st]));
      bit_mask += 1<<switchPins[RTC_1st];
      wake_buttons = "Button " + String(RTC_1st+1) + " wakes";
    #endif
    oled.setFont(MEDIUM_FONT);
    textAnimation(wake_buttons,3000);
    textAnimation("Deep sleep",1000);
    for (int i=0; i<8; i++) {
      s = s.substring(i);
      textAnimation(s,70,-8);
    }
    textAnimation(".",200,-5);
    textAnimation("*",100,3);
    textAnimation("x",100,-3);
    textAnimation("X",100,-1);
    textAnimation("x",100,-3);
    textAnimation(".",100,-5);

    DEBUG("Deep sleep");
    oled.displayOff();                // turn it off, otherwise oled remains active
    #ifdef ACTIVE_HIGH
      esp_sleep_enable_ext1_wakeup( bit_mask, ESP_EXT1_WAKEUP_ANY_HIGH); // wake up on RTC enabled GPIOs
    #else
      // if you use LOW as an active state, only one button can be used for wake up source
      esp_sleep_enable_ext1_wakeup( bit_mask, ESP_EXT1_WAKEUP_ALL_LOW );
    #endif
    esp_deep_sleep_start();
  } 
  else { // if we don't have buttons on RTC GPIOs 
    oled.setFont(MEDIUM_FONT);
    textAnimation("Button 1 wakes",3000);
    textAnimation("Sleep",1000);
    for (int i=0; i<8; i++) {
      s = s.substring(i);
      textAnimation(s,70,-8);
    }
    textAnimation(".",200,-5);
    textAnimation("*",100,3);
    textAnimation("x",100,-3);
    textAnimation("X",100,-1);
    textAnimation("x",100,-3);
    textAnimation(".",100,-5);    

    DEBUG("Light sleep");
    oled.displayOff();                // turn it off, otherwise oled remains active
    #ifdef ACTIVE_HIGH
      gpio_wakeup_enable(static_cast <gpio_num_t> (switchPins[0]), GPIO_INTR_HIGH_LEVEL );
    #else
      gpio_wakeup_enable(static_cast <gpio_num_t> (switchPins[0]), GPIO_INTR_LOW_LEVEL );
    #endif
    esp_sleep_enable_gpio_wakeup();
    esp_light_sleep_start();
    esp_restart();
  }
};

// Respawn
void ESP_on () {
  uint8_t GPIO;  
  esp_sleep_wakeup_cause_t wakeup_cause;
  wakeup_cause = esp_sleep_get_wakeup_cause();
  switch(wakeup_cause)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : DEBUG("Wakeup caused by ext0 signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : 
      DEBUG("Wakeup caused by ext1 signal using RTC_CNTL");
      GPIO = log(esp_sleep_get_ext1_wakeup_status() )/log(2); 
      DEBUG("Waken up by GPIO_" + String(GPIO));
      break;
    case ESP_SLEEP_WAKEUP_TIMER : DEBUG("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : DEBUG("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : DEBUG("Wakeup caused by ULP program"); break;
    default : DEBUG("Wakeup was not caused by deep sleep: " + String(wakeup_cause)); break;
  }
  oled.displayOn();
  oled.setFont(MEDIUM_FONT);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  /*
  textAnimation(".",200,-5);
  textAnimation("*",100,3);
  textAnimation("x",100,-3);
  textAnimation("X",100,-1);
  textAnimation("x",100,-3);
  textAnimation(".",100,-5);
  textAnimation("-",50,-3);
  textAnimation("___",50,-8);
  textAnimation("_____",50,-8);
  textAnimation("_______",50,-8);
  textAnimation("_________",40,-8);
  textAnimation("___________",30,-8);
  textAnimation("_____________",20,-8);
  textAnimation("_______________",10,-8);
  */
  textAnimation("_______________",180,-8);
  textAnimation("\\_______________",30,-8);
  textAnimation("/\\______________",30,-8);
  textAnimation("_/\\_____________",30,-8);
  textAnimation("__/\\____________",30,-8);
  textAnimation("___/\\___________",30,-8);
  textAnimation("____/\\__________",30,-8);
  textAnimation("_____/\\_________",30,-8);
  textAnimation("______/\\________",30,-8);
  textAnimation("_______/\\_______",30,-8);
  textAnimation("________/\\______",30,-8);
  textAnimation("_________/\\_____",30,-8);
  textAnimation("__________/\\____",30,-8);
  textAnimation("___________/\\___",30,-8);
  textAnimation("____________/\\__",30,-8);
  textAnimation("_____________/\\_",30,-8);
  textAnimation("______________/\\",30,-8);
  textAnimation("_______________/",30,-8);
  textAnimation("______________|",30,-8);
  textAnimation("               _____________",30,-8);
  textAnimation("                    _______|",30,-8);
  textAnimation("                           .",200,-5);
  textAnimation("                           *",100,3);
  textAnimation("                           x",100,-3);
  textAnimation("                           X",100,-1);
  textAnimation("                           x",100,-3);
  textAnimation("                           .",100,-5);
  textAnimation("                           -",50,-3);
}

// Checking the config if we have some pins with RTC connection which allows waking from a deep sleep. Otherwise we only can use light sleep
int Check_RTC() {
  RTC_present = 0;
  for(int i = 0; i < NUM_SWITCHES; ++i) {
    sw_RTC[i] = false;
    for(int j = 0; j < (sizeof(RTC_pins)/sizeof(RTC_pins[0])); ++j) {
      if(switchPins[i] == RTC_pins[j]) {
        sw_RTC[i] = true;
        RTC_present++;
        if (RTC_present == 1) {RTC_1st = switchPins[i];}
        DEBUG("RTC pin: " + String(switchPins[i]));
        break;
      }
    }    
  }
  DEBUG ("RTC pins present: " + String(RTC_present));
  return RTC_present;
}

// Toggle bypass mode, switching on/off all the effects and amp sim
void toggleBypass() {
  if (curMode == MODE_BYPASS) {
    bypassOff();
  } else {
    bypassOn();
  }
}

void bypassOn() {
  if (curMode != MODE_BYPASS) {
    for (int i=0; i<=6; i++){
      fxState[i] = presets[CUR_EDITING].effects[i].OnOff;
      change_generic_onoff(i, false);
    }
    returnMode = curMode;
    curMode = MODE_BYPASS;
  }
}

void bypassOff() {
  if (curMode == MODE_BYPASS) {
    for (int i=0; i<=6; i++){
      presets[CUR_EDITING].effects[i].OnOff = fxState[i];
      change_generic_onoff(i, fxState[i]);
    }
    curMode = returnMode;
  }
}

// Toggle tuner mode. This is Spark mode, not just pedal mode. Spark will send measurements continiously.
void toggleTuner() {
  if (curMode==MODE_TUNER) {
    tunerOff();
  } else {
    tunerOn();
  }
}

void tunerOn() {
  if (!isTunerMode) {
    spark_msg_out.tuner_on_off(true);
    returnMode = curMode;
    curMode = MODE_TUNER;
    DEBUG("Tuner mode ON, return to:" + String(returnMode));
  }
}

void tunerOff() {
  if (isTunerMode) {
    spark_msg_out.tuner_on_off(false);
    curMode = returnMode;
    DEBUG("Tuner mode OFF, return to:" + String(returnMode) + " " + String(isTunerMode));
  }
}

void doExpressionPedal() {
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
    DEBUG("Pedal data: ");
    DEBUG(express_result);
    DEBUG(" : ");
    DEBUG(effect_volume);
#endif
    // If effect on/off
    if (expression_target) {
        // Send effect ON state to Spark and App only if OFF
        if ((effect_volume > 0.5)&&(!effectstate)) {
          change_generic_onoff(get_effect_index(msg.str1),true);
          DEBUG("Turning effect ");
          DEBUG(msg.str1);
          DEBUG(" ON via pedal");
          effectstate = true;
        }
        // Send effect OFF state to Spark and App only if ON, also add hysteresis
        else if ((effect_volume < 0.3)&&(effectstate))
        {
          change_generic_onoff(get_effect_index(msg.str1),false);
          DEBUG("Turning effect ");
          DEBUG(msg.str1);
          DEBUG(" OFF via pedal");
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

// We need to know which effects are on and which are off to draw icons
void updateFxStatuses() {
  for (int i=0 ; i< NUM_SWITCHES; i++) {
    SWITCHES[i].fxOnOff = presets[CUR_EDITING].effects[SWITCHES[i].fxSlotNumber].OnOff;
  }
}

// it's useful sometimes not just have an error code, but also some random yet valid data to play with.
SparkPreset somePreset(const char* substTitle) {
  SparkPreset ret_preset = *my_presets[random(HARD_PRESETS-1)];
  strcpy(ret_preset.Description, ret_preset.Name);
  strcpy(ret_preset.Name, substTitle);
  return ret_preset;
}

// load preset from json file in the format used by PG cloud back-up
SparkPreset loadPresetFromFile(int presetSlot) {
  SparkPreset retPreset;
  File presetFile;
  // open dir bound to the slot number
  String dirName =  "/" + (String)(presetSlot) ;
  String fileName = "";
  if (!SPIFFS.exists(dirName)) {
    return somePreset("(No Such Slot)");
  } else {
    File dir = SPIFFS.open(dirName);
    while (!fileName.endsWith(".json")) {
      presetFile = dir.openNextFile();
      if (!presetFile) {
        // no preset found in current slot directory, let's substitute a random one
        DEBUG(">>>> '" + dirName + "' Empty Slot < Random");
        return somePreset("(Empty Slot)");
      }
      fileName = presetFile.name();
      DEBUG(">>>>>>>>>>>>>>>>>> '" + fileName + "'");
    }
    dir.close();
    parseJsonPreset(presetFile, retPreset);
  }
  presetFile.close();
  return retPreset;
}

void parseJsonPreset(File &presetFile, SparkPreset &retPreset) {
  DynamicJsonDocument doc(3072);
  DeserializationError error = deserializeJson(doc, presetFile);
  if (error) {
    retPreset = somePreset("(Invalid json file)");
  } else {
    if (doc["type"] == "jamup_speaker") { // PG app's json
      retPreset.BPM = doc["bpm"];
      JsonObject meta = doc["meta"];
      strcpy(retPreset.Name, meta["name"]);
      strcpy(retPreset.Description, meta["description"]);
      strcpy(retPreset.Version, meta["version"]);
      strcpy(retPreset.Icon, meta["icon"]);
      strcpy(retPreset.UUID, meta["id"]);
      JsonArray sigpath = doc["sigpath"];
      for (int i=0; i<=6; i++) { // effects
        int numParams = 0;
        double value;
        JsonObject fx = sigpath[i];
        for (JsonObject elem : fx["params"].as<JsonArray>()) {
          // <-----> PG format sometimes uses double, and sometimes bool as char[]
          if ( elem["value"].is<bool>() ) {
            if (elem["value"]) {
              value = 0.5;
            } else {
              value = 0;
            }
          } else { // let's hope they don't invent some other type
            value = elem["value"]; 
          }
          int index = elem["index"];
          retPreset.effects[i].Parameters[index] = value;
          numParams = max(numParams,index);
        }
        retPreset.effects[i].NumParameters = numParams+1;
        strcpy(retPreset.effects[i].EffectName , fx["dspId"]);
        retPreset.effects[i].OnOff = fx["active"];
        retPreset.preset_num = 0;
        retPreset.curr_preset = 0;
      }   
    }
  }
}

// save preset to json file in the format used by PG cloud back-up
bool savePresetToFile(SparkPreset savedPreset, const String &filePath) {
  bool noErr = true;
  if(strcmp(savedPreset.Name,"(Empty Slot)")==0){
    strcpy(savedPreset.Name,savedPreset.Description);
  }
  DynamicJsonDocument doc(3072);
  doc["type"] = "jamup_speaker";
  doc["bpm"] = savedPreset.BPM;
  JsonObject meta = doc.createNestedObject("meta");
  meta["id"] = savedPreset.UUID;
  meta["version"] = savedPreset.Version;
  meta["icon"] = savedPreset.Icon;
  meta["name"] = savedPreset.Name;
  meta["description"] = savedPreset.Description;
  JsonArray sigpath = doc.createNestedArray("sigpath");
  for (int i=0; i<7; i++){
    for (int j=0; j<savedPreset.effects[i].NumParameters; j++) {
      sigpath[i]["params"][j]["index"] = j;
      sigpath[i]["params"][j]["value"] = savedPreset.effects[i].Parameters[j];
    }
    sigpath[i]["type"] = "speaker_fx";
    sigpath[i]["dspId"] = savedPreset.effects[i].EffectName;
    sigpath[i]["active"] = savedPreset.effects[i].OnOff;
  }
  File fJson = SPIFFS.open(filePath,"w");
  noErr = serializeJson(doc, fJson);
  return noErr;
}

// This function returns {fxSlot, fxNumber} by fxName, thanks PG for such brute force routine
s_fx_coords fxNumByName(const char* fxName) {
  int i = 0;
  int j = 3; //3: amp is most often in use 
  for (const auto &fx: spark_amps) {
    if (strcmp(fx, fxName)==0){
      return {j,i};
    }
    i++;
  }

  i = 0;
  j = 4; //4: modulation 
  for (const auto &fx: spark_modulations) {
    if (strcmp(fx, fxName)==0){
      return {j,i};
    }
    i++;
  }

  i = 0;
  j = 5; // 5: delay
  for (const auto &fx: spark_delays) {
    if (strcmp(fx, fxName)==0){
      return {j,i};
    }
    i++;
  }
  
  i = 0;
  j = 6; // 6: reverb
  for (const auto &fx: spark_reverbs) {
    if (strcmp(fx, fxName)==0){
      return {j,i};
    }
    i++;
  }

  i = 0;
  j = 2; //2: drive
  for (const auto &fx: spark_drives) {
    if (strcmp(fx, fxName)==0){
      return {j,i};
    }
    i++;
  }

  i = 0;
  j = 1; // 1: compressor
  for (const auto &fx: spark_compressors) {
    if (strcmp(fx, fxName)==0){
      return {j,i};
    }
    i++;
  }

  i = 0;
  j = 0; //0: noise gate
  for (const auto &fx: spark_noisegates) {
    if (strcmp(fx, fxName)==0){
      return {j,i};
    }
    i++;
  }
  return {-1,-1}; // unknown situation, unknown effect
}

// temporarily switching to a different UI frame
void tempFrame(eMode_t tempFrame, eMode_t retFrame, const ulong msTimeout) {
  if (!tempUI) {
    curMode = tempFrame;
    returnMode = retFrame;
    tempUI = true;
  }
  actual_timeout = msTimeout;
  timeToGoBack = millis() + msTimeout;
}

// Returning from a temp UI frame
void returnToMainUI() {
  if (tempUI) {
    curMode = returnMode;
    timeToGoBack = millis();
    tempUI = false;
    #ifdef RETURN_TO_MASTER   
      curKnob = 4; // Set to Master knob {3,1} (SparkPresets.h)
    #endif
    DEBUG("Return to main UI");
  }
}
