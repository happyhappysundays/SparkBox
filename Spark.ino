///// ROUTINES TO SYNC TO AMP SETTINGS

unsigned long sync_timer;
int selected_preset;
bool ui_update_in_progress;
int preset_requested;
bool preset_received;

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

bool  spark_state_tracker_start() {
  spark_state = SPARK_DISCONNECTED;
  // try to find and connect to Spark - returns false if failed to find Spark
  if (!connect_to_all()) return false;    
                
  spark_start(false);                  // set up the classes to communicate with Spark and app
  spark_state = SPARK_CONNECTED;     // it has to be to have reached here

  spark_ping_timer = millis();
  selected_preset = 0;
  ui_update_in_progress = false;

  return true;
}

// get changes from app or Spark and update internal state to reflect this
// this function has the side-effect of loading cmdsub, msg and preset which can be used later

bool  update_spark_state() {
  int pres, ind;

  // sort out connection and sync progress
  if (conn_status[SPK] == false && spark_state != SPARK_DISCONNECTED) {
    spark_state = SPARK_DISCONNECTED;
    spark_ping_timer = millis();
    DEBUG("Spark disconnected, try to reconnect...");
  }


  if (spark_state == SPARK_DISCONNECTED) {
    if (millis() - spark_ping_timer > 500) {
      spark_ping_timer = millis();
      connect_spark();  // reconnects if any disconnects happen    
    }
  }

  if (conn_status[SPK] == true && spark_state == SPARK_DISCONNECTED) {
    spark_state = SPARK_CONNECTED;
    DEBUG("Spark connected");
  }

  if (spark_state == SPARK_CONNECTED) {
    if (millis() - spark_ping_timer > 500) {
      // every 0.5s ping the Spark amp to see if it will respond
      spark_ping_timer = millis();
      spark_msg_out.get_serial();
      DEBUG("Pinging Spark");  
    }  
  }

  if (spark_state == SPARK_SYNCING) {  
    if (preset_received == true || millis() - sync_timer > 2000) { // we got the preset we were after or time out, and need to ask for the next one
      if (preset_received == false) 
        DEBUG("**** Preset not received, trying again");
      sync_timer = millis();
      spark_msg_out.get_preset_details((preset_requested == 5) ? 0x0100 : preset_requested);
//      if (preset_requested <= 3)
//        spark_msg_out.change_hardware_preset(0, preset_requested);
      preset_received = false;
      DEB("Requested a preset: ");  
      DEBUG(preset_requested);
    }
  }

  if (spark_state == SPARK_COMMUNICATING) {
    spark_msg_out.get_firmware();
    spark_msg_out.get_checksum_info();
    preset_requested = 0;
    preset_received = true;
    spark_state = SPARK_CHECKSUM;
  }

  
  spark_process();
  app_process();
  
  // K&R: Expressions connected by && or || are evaluated left to right, 
  // and it is guaranteed that evaluation will stop as soon as the truth or falsehood is known.
  
  if (spark_msg_in.get_message(&cmdsub, &msg, &preset) || app_msg_in.get_message(&cmdsub, &msg, & preset)) {
    DEB("Message: ");
    DEBUG(cmdsub, HEX);

    // all the processing for sync
    switch (cmdsub) {
      // if serial number response then we know the connection is good
      case 0x0323:
        if (spark_state == SPARK_CONNECTED) {
          spark_state = SPARK_COMMUNICATING;
          DEBUG("Received serial number, got connection");
        }
        break;
      case 0x032a:
        if (spark_state == SPARK_CHECKSUM) {
          spark_state = SPARK_SYNCING;
          sync_timer = millis();
          DEBUG("Got checksum");
        }
        break;       
      // full preset details
      case 0x0101:
      case 0x0301:  
        pres = (preset.preset_num == 0x7f) ? 4 : preset.preset_num;
        if (preset.curr_preset == 0x01)
          pres = 5;

        presets[pres] = preset;

        DEB("Got preset ");
        DEBUG(pres);
        
        if (spark_state == SPARK_SYNCING) {
          if (!preset_received) {
            if (preset_requested == pres) {
              preset_received = true;
              preset_requested++;
              if (preset_requested == 4) // can't ask for 4!
                preset_requested++;
            }
          }
          if (pres == 5) {
            spark_state = SPARK_SYNCED;
            sp_bin.pass_through = true;
            app_bin.pass_through = true;             
            DEBUG("Fully synced now");
          }
        }

        // Only update the displayed preset number for HW presets
        if (pres < 4){
          display_preset_num = pres; 
        }
        #ifdef DUMP_ON
          DEB("Send / receive new preset: ");
          DEBUG(p, HEX);      
          dump_preset(preset);
        #endif
        break;
      // change of amp model
      case 0x0306:
        strcpy(presets[5].effects[3].EffectName, msg.str2);
        break;
      // change of effect
      case 0x0106:
        ind = get_effect_index(msg.str1);
        if (ind >= 0) strcpy(presets[5].effects[ind].EffectName, msg.str2);
        setting_modified = true;
        break;
      // effect on/off  
      case 0x0115:
      case 0x0315:
        ind = get_effect_index(msg.str1);
        if (ind >= 0) presets[5].effects[ind].OnOff = msg.onoff;
        setting_modified = true;  
        break;
      // change parameter value  
      case 0x0104:
      case 0x0337:
        //expression_target = false;
        ind = get_effect_index(msg.str1);
        if (ind >= 0) presets[5].effects[ind].Parameters[msg.param1] = msg.val;
        strcpy(param_str, msg.str1);
        param = msg.param1;
        setting_modified = true;
        break;  

      // Send licence key   
      case 0x0170:
        break; 
          
      // change to preset  
      case 0x0138:
      case 0x0338:
        selected_preset = (msg.param2 == 0x7f) ? 4 : msg.param2;
        presets[5] = presets[selected_preset];
        setting_modified = false;
        // Only update the displayed preset number for HW presets
        if (selected_preset < 4){
          display_preset_num = selected_preset; 
        }
        break;
      // store to preset  
      case 0x0327:
        selected_preset = (msg.param2 == 0x7f) ? 4 : msg.param2;
        presets[selected_preset] = presets[5];
        setting_modified = false;
        // Only update the displayed preset number for HW presets
        if (selected_preset < 4){
          display_preset_num = selected_preset; 
        }  
        break;
      // current selected preset
      case 0x0310:
        selected_preset = (msg.param2 == 0x7f) ? 4 : msg.param2;
        if (msg.param1 == 0x01) 
          selected_preset = 5;
        presets[5] = presets[selected_preset];
        // Only update the displayed preset number for HW presets
        if (selected_preset < 4){
          display_preset_num = selected_preset; 
        }
        break;
       // Refresh preset info based on app-requested change

      case 0x0364:
        isTunerMode = true;
        break;    
         
      case 0x0365:
      case 0x0465:
        isTunerMode = false;
        break;       
      
      case 0x0438:
        setting_modified = false;
        break;
      default:
        break;
    }

    // all the processing for UI update
    switch (cmdsub) {
      case 0x0201:  
         if (ui_update_in_progress) {
           DEBUG("Updating UI");

           strcpy(presets[5].Name, "SyncPreset");
           strcpy(presets[5].UUID, "F00DF00D-FEED-0123-4567-987654321000");  
           presets[5].curr_preset = 0x00;
           presets[5].preset_num = 0x03;
           app_msg_out.create_preset(&presets[5]);
           app_process();
           delay(100);
           
           app_msg_out.change_hardware_preset(0x00, 0x00);
           app_process();
           app_msg_out.change_hardware_preset(0x00, 0x03);     
           app_process();

           sp_bin.pass_through = true;
           app_bin.pass_through = true;   
           ui_update_in_progress = false;
         }
       break;
    }
          
    return true;
  }
  else
    return false;
}

void update_ui() {
  sp_bin.pass_through = false;
  app_bin.pass_through = false;
    
  app_msg_out.save_hardware_preset(0x00, 0x03);
  app_process();
  ui_update_in_progress = true;
}

///// ROUTINES TO CHANGE AMP SETTINGS

void change_generic_model(char *new_eff, int slot) {
  if (strcmp(presets[5].effects[slot].EffectName, new_eff) != 0) {
    spark_msg_out.change_effect(presets[5].effects[slot].EffectName, new_eff);
    strcpy(presets[5].effects[slot].EffectName, new_eff);
    spark_process();
    delay(100);
  }
}

void change_comp_model(char *new_eff) {
  change_generic_model(new_eff, 1);
}

void change_drive_model(char *new_eff) {
  change_generic_model(new_eff, 2);
}

void change_amp_model(char *new_eff) {
  if (strcmp(presets[5].effects[3].EffectName, new_eff) != 0) {
    spark_msg_out.change_effect(presets[5].effects[3].EffectName, new_eff);
    app_msg_out.change_effect(presets[5].effects[3].EffectName, new_eff);
    strcpy(presets[5].effects[3].EffectName, new_eff);
    spark_process();
    app_process();
    delay(100);
  }
}

void change_mod_model(char *new_eff) {
  change_generic_model(new_eff, 4);
}

void change_delay_model(char *new_eff) {
  change_generic_model(new_eff, 5);
}

void change_generic_onoff(int slot,bool onoff) {
  spark_msg_out.turn_effect_onoff(presets[5].effects[slot].EffectName, onoff);
  app_msg_out.turn_effect_onoff(presets[5].effects[slot].EffectName, onoff);
  presets[5].effects[slot].OnOff = onoff;
  spark_process();
  app_process();  
}

void change_noisegate_onoff(bool onoff) {
  change_generic_onoff(0, onoff);  
}

void change_comp_onoff(bool onoff) {
  change_generic_onoff(1, onoff);  
}

void change_drive_onoff(bool onoff) {
  change_generic_onoff(2, onoff);  
}

void change_amp_onoff(bool onoff) {
  change_generic_onoff(3, onoff);  
}

void change_mod_onoff(bool onoff) {
  change_generic_onoff(4, onoff);  
}

void change_delay_onoff(bool onoff) {
  change_generic_onoff(5, onoff);  
}

void change_reverb_onoff(bool onoff) {
  change_generic_onoff(6, onoff);  
}

void change_generic_toggle(int slot) {
  bool new_onoff;

  new_onoff = !presets[5].effects[slot].OnOff;
  
  spark_msg_out.turn_effect_onoff(presets[5].effects[slot].EffectName, new_onoff);
  app_msg_out.turn_effect_onoff(presets[5].effects[slot].EffectName, new_onoff);
  presets[5].effects[slot].OnOff = new_onoff;
  spark_process();
  app_process();  
}

void change_noisegate_toggle() {
  change_generic_toggle(0);  
}

void change_comp_toggle() {
  change_generic_toggle(1);  
}

void change_drive_toggle() {
  change_generic_toggle(2);  
}

void change_amp_toggle() {
  change_generic_toggle(3);  
}

void change_mod_toggle() {
  change_generic_toggle(4);  
}

void change_delay_toggle() {
  change_generic_toggle(5);  
}

void change_reverb_toggle() {
  change_generic_toggle(6);  
}

void change_generic_param(int slot, int param, float val) {
/* float diff;

  // some code to reduce the number of changes
  diff = presets[5].effects[slot].Parameters[param] - val;
  if (diff < 0) diff = -diff;
  if (diff > 0.04) {
*/
  {
    spark_msg_out.change_effect_parameter(presets[5].effects[slot].EffectName, param, val);
    app_msg_out.change_effect_parameter(presets[5].effects[slot].EffectName, param, val);
    presets[5].effects[slot].Parameters[param] = val;
    spark_process();  
    app_process();
  }
}

void change_noisegate_param(int param, float val) {
  change_generic_param(0, param, val);
}

void change_comp_param(int param, float val) {
  change_generic_param(1, param, val);
}

void change_drive_param(int param, float val) {
  change_generic_param(2, param, val);
}

void change_amp_param(int param, float val) {
  change_generic_param(3, param, val);
}

void change_mod_param(int param, float val) {
  change_generic_param(4, param, val);
}

void change_delay_param(int param, float val) {
  change_generic_param(5, param, val);
}

void change_reverb_param(int param, float val){
  change_generic_param(6, param, val);
}

void change_hardware_preset(int pres_num) {
  if (pres_num >= 0 && pres_num <= 3) {  
    presets[5] = presets[pres_num];
    
    spark_msg_out.change_hardware_preset(0, pres_num);
    app_msg_out.change_hardware_preset(0, pres_num);  
    spark_process();  
    app_process();
  }
}

void change_custom_preset(SparkPreset *preset, int pres_num) {
  if (pres_num >= 0 && pres_num <= 4) {
    preset->preset_num = (pres_num < 4) ? pres_num : 0x7f;
    presets[5] = *preset;
    presets[pres_num] = *preset;
    
    spark_msg_out.create_preset(preset);
    spark_msg_out.change_hardware_preset(0, preset->preset_num);
  }
}

void tuner_on_off(bool on_off) {
  spark_msg_out.tuner_on_off(on_off);
  spark_process();
}
