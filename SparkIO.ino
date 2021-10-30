#include "Spark.h"
#include "SparkIO.h"

/*  SparkIO
 *  
 *  SparkIO handles communication to and from the Positive Grid Spark amp over bluetooth for ESP32 boards
 *  
 *  From the programmers perspective, you create and read two formats - a Spark Message or a Spark Preset.
 *  The Preset has all the data for a full preset (amps, effects, values) and can be sent or received from the amp.
 *  The Message handles all other changes - change amp, change effect, change value of an effect parameter, change hardware preset and so on
 *  
 *  The class is initialized by creating an instance such as:
 *  
 *  SparkClass sp;
 *  
 *  Conection is handled with the two commands:
 *  
 *    sp.start_bt();
 *    sp.connect_to_spark();
 *    
 *  The first starts up bluetooth, the second connects to the amp
 *  
 *  
 *  Messages and presets to and from the amp are then queued and processed.
 *  The essential thing is the have the process() function somewhere in loop() - this handles all the processing of the input and output queues
 *  
 *  loop() {
 *    ...
 *    sp.process()
 *    ...
 *    do something
 *    ...
 *  }
 * 
 * Sending functions:
 *     void create_preset(SparkPreset *preset);    
 *     void get_serial();    
 *     void turn_effect_onoff(char *pedal, bool onoff);    
 *     void change_hardware_preset(uint8_t preset_num);    
 *     void change_effect(char *pedal1, char *pedal2);    
 *     void change_effect_parameter(char *pedal, int param, float val);
 *     
 *     These all create a message or preset to be sent to the amp when they reach the front of the 'send' queue
 *  
 * Receiving functions:
 *     bool get_message(unsigned int *cmdsub, SparkMessage *msg, SparkPreset *preset);
 * 
 *     This receives the front of the 'received' queue - if there is nothing it returns false
 *     
 *     Based on whatever was in the queue, it will populate fields of the msg parameter or the preset parameter.
 *     Eveything apart from a full preset sent from the amp will be a message.
 *     
 *     You can determine which by inspecting cmdsub - this will be 0x0301 for a preset.
 *     
 *     Other values are:
 *     
 *     cmdsub       str1                   str2              val           param1             param2                onoff
 *     0323         amp serial #
 *     0337         effect name                              effect val    effect number
 *     0306         old effect             new effect
 *     0338                                                                0                  new hw preset (0-3)
 * 
 * 
 * 
 */



void uint_to_bytes(unsigned int i, uint8_t *h, uint8_t *l) {
  *h = (i & 0xff00) / 256;
  *l = i & 0xff;
}

void bytes_to_uint(uint8_t h, uint8_t l,unsigned int *i) {
  *i = (h & 0xff) * 256 + (l & 0xff);
}

 
//
// SparkIO class
//

SparkIO::SparkIO(bool passthru) {
  sp_pass_through = passthru;
  sp_rb_state = 0;
  sp_rc_state = 0;
  sp_oc_seq = 0x20;
  sp_ob_ok_to_send = true;
  sp_ob_last_sent_time = millis();
  
  sp_bt_state = 0;
  sp_bt_pos = 0;
  sp_bt_len = -1;


  app_pass_through = passthru;
  app_rb_state = 0;
  app_rc_state = 0;
  app_oc_seq = 0x40;
  app_ob_ok_to_send = true;
  app_ob_last_sent_time = millis();

  app_ser_pos = 0;
  app_ser_state = 0;
  app_ser_len = -1;
}

// 
// Main processing routine
//

void SparkIO::sp_process() 
{
  // process inputs
  sp_process_in_blocks();
  sp_process_in_chunks();

  if (!sp_ob_ok_to_send && (millis() - sp_ob_last_sent_time > 500)) {
    DEBUG("Timeout on send");
    sp_ob_ok_to_send = true;
  }

  // process outputs
  sp_process_out_chunks();
  sp_process_out_blocks();
}


//
// Routine to read the block from bluetooth and put into the in_chunk ring buffer
//

uint8_t chunk_header_from_spark[16]{0x01, 0xfe, 0x00, 0x00, 0x41, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void SparkIO::sp_process_in_blocks() {
  uint8_t b;
  bool boo;

  while (sp_available()) {
    b = sp_read();
   
    // **** PASSTHROUGH OF SERIAL TO BLUETOOTH ****

    if (sp_pass_through) {
      if (sp_bt_state == 0 && b == 0x01) 
        sp_bt_state = 1;
      else if (sp_bt_state == 1) {
        if (b == 0xfe) {
          sp_bt_state = 2;
          sp_bt_buf[0] = 0x01;
          sp_bt_buf[1] = 0xfe;
          sp_bt_pos = 2;
        }
        else
          sp_bt_state = 0;
      }
      else if (sp_bt_state == 2) {
        if (sp_bt_pos == 6) {
          sp_bt_len = b;
        }
        sp_bt_buf[sp_bt_pos++] = b;
        if (sp_bt_pos == sp_bt_len) {
          app_write(sp_bt_buf, sp_bt_pos);
          sp_bt_pos = 0;
          sp_bt_len = -1; 
          sp_bt_state = 0; 
        }
      }

      if (sp_bt_pos > MAX_BT_BUFFER) {
        DEBUG("SPARKIO IO_PROCESS_IN_BLOCKS OVERRUN");
        while (true);
      }
    }
    // **** END PASSTHROUGH ****
    
    // check the 7th byte which holds the block length
    if (sp_rb_state == 6) {
      sp_rb_len = b - 16;
      sp_rb_state++;
    }
    // check every other byte in the block header for a match to the header standard
    else if (sp_rb_state >= 0 && sp_rb_state < 16) {
      if (b == chunk_header_from_spark[sp_rb_state]) {
        sp_rb_state++;
      }
      else {
        Serial.print (sp_rb_state);
        Serial.print(" ");
        Serial.print(b);
        Serial.print(" ");
        Serial.print(sp_rb_len);
        Serial.println();
        sp_rb_state = -1;
        DEBUG("SparkIO bad block header");
      }
    } 
    // and once past the header just read the next bytes as defined by rb_len
    // store these to the chunk buffer
    else if (sp_rb_state == 16) {
      sp_in_chunk.add(b);
      sp_rb_len--;
      if (sp_rb_len == 0) {
        sp_rb_state = 0;
        sp_in_chunk.commit();
      }
    }
      
    // checking for rb_state 0 is done separately so that if a prior step finds a mismatch
    // and resets the state to 0 it can be processed here for that byte - saves missing the 
    // first byte of the header if that was misplaced
    
    if (sp_rb_state == -1) 
      if (b == chunk_header_from_spark[0]) 
        sp_rb_state = 1;
  }
}

//
// Routine to read chunks from the in_chunk ring buffer and copy to a in_message msgpack buffer
//

void SparkIO::sp_process_in_chunks() {
  uint8_t b;
  bool boo;
  unsigned int len;
  uint8_t len_h, len_l;

  while (!sp_in_chunk.is_empty()) {               // && in_message.is_empty()) {  -- no longer needed because in_message is now a proper ringbuffer
    boo = sp_in_chunk.get(&b);
    if (!boo) DEBUG("Chunk is_empty was false but the buffer was empty!");

    switch (sp_rc_state) {
      case 1:
        if (b == 0x01) 
          sp_rc_state++; 
        else 
          sp_rc_state = 0; 
        break;
      case 2:
        sp_rc_seq = b; 
        sp_rc_state++; 
        break;
      case 3:
        sp_rc_checksum = b;
        sp_rc_state++; 
        break;
      case 4:
        sp_rc_cmd = b; 
        sp_rc_state++; 
        break;
      case 5:
        sp_rc_sub = b; 
        sp_rc_state = 10;

        // flow control for blocking sends - put here in case we want to check rc_sub too
        if (sp_rc_cmd == 0x04 && sp_rc_sub == 0x01) {
          if (sp_ob_ok_to_send == false) {
            sp_ob_ok_to_send = true;
            DEBUG("Unblocked");
          }
        }
        
        // set up for the main data loop - rc_state 10
        sp_rc_bitmask = 0x80;
        sp_rc_calc_checksum = 0;
        sp_rc_data_pos = 0;
        
        // check for multi-chunk
        if (sp_rc_cmd == 3 && sp_rc_sub == 1) 
          sp_rc_multi_chunk = true;
        else {
          sp_rc_multi_chunk = false;
          sp_in_message_bad = false;
          sp_in_message.add(sp_rc_cmd);
          sp_in_message.add(sp_rc_sub);
          sp_in_message.add(0);
          sp_in_message.add(0);
        }
        break;
      case 10:                    // the main loop which ends on an 0xf7
        if (b == 0xf7) {
          if (sp_rc_calc_checksum != sp_rc_checksum) 
            sp_in_message_bad = true;
          sp_rc_state = 0;
          if (!sp_rc_multi_chunk || (sp_rc_this_chunk == sp_rc_total_chunks-1)) { //last chunk in message
            if (sp_in_message_bad) {
              DEBUG("Bad message, dropped");
              sp_in_message.drop();
            }
            else {
              len = sp_in_message.get_len();
              uint_to_bytes(len, &len_h, &len_l);

              sp_in_message.set_at_index(2, len_h);
              sp_in_message.set_at_index(3, len_l);
              sp_in_message.commit();
              
// TRACE
//              Serial.print("From Spark     ");
//              sp_in_message.dump3();
            }  
          }
        }
        else if (sp_rc_bitmask == 0x80) { // if the bitmask got to this value it is now a new bits 
          sp_rc_calc_checksum ^= b;
          sp_rc_bits = b;
          sp_rc_bitmask = 1;
        }
        else {
          sp_rc_data_pos++;
          sp_rc_calc_checksum ^= b;          
          if (sp_rc_bits & sp_rc_bitmask) 
            b |= 0x80;
          sp_rc_bitmask *= 2;
          
          if (sp_rc_multi_chunk && sp_rc_data_pos == 1) 
            sp_rc_total_chunks = b;
          else if (sp_rc_multi_chunk && sp_rc_data_pos == 2) {
            sp_rc_last_chunk = sp_rc_this_chunk;
            sp_rc_this_chunk = b;
            if (sp_rc_this_chunk == 0) {
              sp_in_message_bad = false;
              sp_in_message.add(sp_rc_cmd);
              sp_in_message.add(sp_rc_sub);
              sp_in_message.add(0);
              sp_in_message.add(0);
            }
            else if (sp_rc_this_chunk != sp_rc_last_chunk+1)
              sp_in_message_bad = true;
          }
          else if (sp_rc_multi_chunk && sp_rc_data_pos == 3) 
            sp_rc_chunk_len = b;
          else {  
            sp_in_message.add(b);             
          }
          
        };
        break;
    }

    // checking for rc_state 0 is done separately so that if a prior step finds a mismatch
    // and resets the state to 0 it can be processed here for that byte - saves missing the 
    // first byte of the header if that was misplaced
    
    if (sp_rc_state == 0) {
      if (b == 0xf0) 
        sp_rc_state++;
    }
  }
}

//// Routines to interpret the data

void SparkIO::sp_read_byte(uint8_t *b)
{
  uint8_t a;
  sp_in_message.get(&a);
  *b = a;
}   
   
void SparkIO::sp_read_string(char *str)
{
  uint8_t a, len;
  int i;

  sp_read_byte(&a);
  if (a == 0xd9) {
    sp_read_byte(&len);
  }
  else if (a >= 0xa0) {
    len = a - 0xa0;
  }
  else {
    sp_read_byte(&a);
    if (a < 0xa0 || a >= 0xc0) DEBUG("Bad string");
    len = a - 0xa0;
  }

  if (len > 0) {
    // process whole string but cap it at STR_LEN-1
    for (i = 0; i < len; i++) {
      sp_read_byte(&a);
      if (a<0x20 || a>0x7e) a=0x20; // make sure it is in ASCII range - to cope with get_serial 
      if (i < STR_LEN -1) str[i]=a;
    }
    str[i > STR_LEN-1 ? STR_LEN-1 : i]='\0';
  }
  else {
    str[0]='\0';
  }
}   

void SparkIO::sp_read_prefixed_string(char *str)
{
  uint8_t a, len;
  int i;

  sp_read_byte(&a); 
  sp_read_byte(&a);

  if (a < 0xa0 || a >= 0xc0) DEBUG("Bad string");
  len = a-0xa0;

  if (len > 0) {
    for (i = 0; i < len; i++) {
      sp_read_byte(&a);
      if (a<0x20 || a>0x7e) a=0x20; // make sure it is in ASCII range - to cope with get_serial 
      if (i < STR_LEN -1) str[i]=a;
    }
    str[i > STR_LEN-1 ? STR_LEN-1 : i]='\0';
  }
  else {
    str[0]='\0';
  }
}   

void SparkIO::sp_read_float(float *f)
{
  union {
    float val;
    byte b[4];
  } conv;   
  uint8_t a;
  int i;

  sp_read_byte(&a);  // should be 0xca
  if (a != 0xca) return;

  // Seems this creates the most significant byte in the last position, so for example
  // 120.0 = 0x42F00000 is stored as 0000F042  
   
  for (i=3; i>=0; i--) {
    sp_read_byte(&a);
    conv.b[i] = a;
  } 
  *f = conv.val;
}

void SparkIO::sp_read_onoff(bool *b)
{
  uint8_t a;
   
  sp_read_byte(&a);
  if (a == 0xc3)
    *b = true;
  else // 0xc2
    *b = false;
}

// The functions to get the message

bool SparkIO::sp_get_message(unsigned int *cmdsub, SparkMessage *msg, SparkPreset *preset)
{
  uint8_t cmd, sub, len_h, len_l;
  unsigned int len;
  unsigned int cs;
   
  uint8_t junk;
  int i, j;
  uint8_t num;

  if (sp_in_message.is_empty()) return false;

  sp_read_byte(&cmd);
  sp_read_byte(&sub);
  sp_read_byte(&len_h);
  sp_read_byte(&len_l);
  
  bytes_to_uint(len_h, len_l, &len);
  bytes_to_uint(cmd, sub, &cs);

  *cmdsub = cs;
  switch (cs) {
    // change of effect model
    case 0x0306:
      sp_read_string(msg->str1);
      sp_read_string(msg->str2);
      break;
    // get current hardware preset number
    case 0x0310:
      sp_read_byte(&msg->param1);
      sp_read_byte(&msg->param2);
      break;
    // get name
    case 0x0311:
      sp_read_string(msg->str1);
      break;
    // enable / disable an effect
    case 0x0315:
      sp_read_string(msg->str1);
      sp_read_onoff(&msg->onoff);
      break;
    // get serial number
    case 0x0323:
      sp_read_string(msg->str1);
      break;
    // store into hardware preset
    case 0x0327:
      sp_read_byte(&msg->param1);
      sp_read_byte(&msg->param2);
      break;
    // firmware version number
    case 0x032f:
      // really this is a uint32 but it is just as easy to read into 4 uint8 - a bit of a cheat
      sp_read_byte(&junk);           // this will be 0xce for a uint32
      sp_read_byte(&msg->param1);      
      sp_read_byte(&msg->param2); 
      sp_read_byte(&msg->param3); 
      sp_read_byte(&msg->param4); 
      break;
    // change of effect parameter
    case 0x0337:
      sp_read_string(msg->str1);
      sp_read_byte(&msg->param1);
      sp_read_float(&msg->val);
      break;
    // change of preset number selected on the amp via the buttons
    case 0x0338:
      sp_read_byte(&msg->param1);
      sp_read_byte(&msg->param2);
      break;
    // response to a request for a full preset
    case 0x0301:
      sp_read_byte(&preset->curr_preset);
      sp_read_byte(&preset->preset_num);
      sp_read_string(preset->UUID); 
      sp_read_string(preset->Name);
      sp_read_string(preset->Version);
      sp_read_string(preset->Description);
      sp_read_string(preset->Icon);
      sp_read_float(&preset->BPM);

      for (j=0; j<7; j++) {
        sp_read_string(preset->effects[j].EffectName);
        sp_read_onoff(&preset->effects[j].OnOff);
        sp_read_byte(&num);
        preset->effects[j].NumParameters = num - 0x90;
        for (i = 0; i < preset->effects[j].NumParameters; i++) {
          sp_read_byte(&junk);
          sp_read_byte(&junk);
          sp_read_float(&preset->effects[j].Parameters[i]);
        }
      }
      sp_read_byte(&preset->chksum);  
      break;
    // tap tempo!
    case 0x0363:
      sp_read_float(&msg->val);  
      break;
    // acks - no payload to read - no ack sent for an 0x104
    case 0x0401:
    case 0x0406:
    case 0x0415:
    case 0x0438:
      Serial.print("Got an ack ");
      Serial.println(cs, HEX);
      break;
    default:
      Serial.print("Unprocessed message SparkIO ");
      Serial.print (cs, HEX);
      Serial.print(":");
      for (i = 0; i < len - 4; i++) {
        sp_read_byte(&junk);
        Serial.print(junk, HEX);
        Serial.print(" ");
      }
      Serial.println();
  }

  return true;
}

    
//
// Output routines
//


void SparkIO::sp_start_message(int cmdsub)
{
  sp_om_cmd = (cmdsub & 0xff00) >> 8;
  sp_om_sub = cmdsub & 0xff;

  // THIS IS TEMPORARY JUST TO SHOW IT WORKS!!!!!!!!!!!!!!!!
  //sp.out_message.clear();

  sp_out_message.add(sp_om_cmd);
  sp_out_message.add(sp_om_sub);
  sp_out_message.add(0);      // placeholder for length
  sp_out_message.add(0);      // placeholder for length

  sp_out_msg_chksum = 0;
}


void SparkIO::sp_end_message()
{
  unsigned int len;
  uint8_t len_h, len_l;
  
  len = sp_out_message.get_len();
  uint_to_bytes(len, &len_h, &len_l);
  
  sp_out_message.set_at_index(2, len_h);   
  sp_out_message.set_at_index(3, len_l);
  sp_out_message.commit();
}

void SparkIO::sp_write_byte_no_chksum(byte b)
{
  sp_out_message.add(b);
}

void SparkIO::sp_write_byte(byte b)
{
  sp_out_message.add(b);
  sp_out_msg_chksum += int(b);
}

void SparkIO::sp_write_prefixed_string(const char *str)
{
  int len, i;

  len = strnlen(str, STR_LEN);
  sp_write_byte(byte(len));
  sp_write_byte(byte(len + 0xa0));
  for (i=0; i<len; i++)
    sp_write_byte(byte(str[i]));
}

void SparkIO::sp_write_string(const char *str)
{
  int len, i;

  len = strnlen(str, STR_LEN);
  sp_write_byte(byte(len + 0xa0));
  for (i=0; i<len; i++)
    sp_write_byte(byte(str[i]));
}      
  
void SparkIO::sp_write_long_string(const char *str)
{
  int len, i;

  len = strnlen(str, STR_LEN);
  sp_write_byte(byte(0xd9));
  sp_write_byte(byte(len));
  for (i=0; i<len; i++)
    sp_write_byte(byte(str[i]));
}

void SparkIO::sp_write_float (float flt)
{
  union {
    float val;
    byte b[4];
  } conv;
  int i;
   
  conv.val = flt;
  // Seems this creates the most significant byte in the last position, so for example
  // 120.0 = 0x42F00000 is stored as 0000F042  
   
  sp_write_byte(0xca);
  for (i=3; i>=0; i--) {
    sp_write_byte(byte(conv.b[i]));
  }
}

void SparkIO::sp_write_onoff (bool onoff)
{
  byte b;

  if (onoff)
  // true is 'on'
    b = 0xc3;
  else
    b = 0xc2;
  sp_write_byte(b);
}

//
//
//

void SparkIO::sp_change_effect_parameter (char *pedal, int param, float val)
{
   sp_start_message (0x0104);
   sp_write_prefixed_string (pedal);
   sp_write_byte (byte(param));
   sp_write_float(val);
   sp_end_message();
}


void SparkIO::sp_change_effect (char *pedal1, char *pedal2)
{
   sp_start_message (0x0106);
   sp_write_prefixed_string (pedal1);
   sp_write_prefixed_string (pedal2);
   sp_end_message();
}

void SparkIO::sp_change_hardware_preset (uint8_t preset_num)
{
   // preset_num is 0 to 3

   sp_start_message (0x0138);
   sp_write_byte (0);
   sp_write_byte (preset_num)  ;     
   sp_end_message();  
}

void SparkIO::sp_turn_effect_onoff (char *pedal, bool onoff)
{
   sp_start_message (0x0115);
   sp_write_prefixed_string (pedal);
   sp_write_onoff (onoff);
   sp_end_message();
}

void SparkIO::sp_get_serial()
{
   sp_start_message (0x0223);
   sp_end_message();  
}

void SparkIO::sp_get_name()
{
   sp_start_message (0x0211);
   sp_end_message();  
}

void SparkIO::sp_get_hardware_preset_number()
{
   sp_start_message (0x0210);
   sp_end_message();  
}


void SparkIO::sp_get_preset_details(unsigned int preset)
{
   int i;
   uint8_t h, l;

   uint_to_bytes(preset, &h, &l);
   
   sp_start_message (0x0201);
   sp_write_byte(h);
   sp_write_byte(l);

   for (i=0; i<30; i++) {
     sp_write_byte(0);
   }
   
   sp_end_message(); 
}

void SparkIO::sp_create_preset(SparkPreset *preset)
{
  int i, j, siz;

  sp_start_message (0x0101);

  sp_write_byte_no_chksum (0x00);
  sp_write_byte_no_chksum (preset->preset_num);   
  sp_write_long_string (preset->UUID);
  sp_write_string (preset->Name);
  sp_write_string (preset->Version);
  if (strnlen (preset->Description, STR_LEN) > 31)
    sp_write_long_string (preset->Description);
  else
    sp_write_string (preset->Description);
  sp_write_string(preset->Icon);
  sp_write_float (preset->BPM);

   
  sp_write_byte (byte(0x90 + 7));       // always 7 pedals

  for (i=0; i<7; i++) {
      
    sp_write_string (preset->effects[i].EffectName);
    sp_write_onoff(preset->effects[i].OnOff);

    siz = preset->effects[i].NumParameters;
    sp_write_byte ( 0x90 + siz); 
      
    for (j=0; j<siz; j++) {
      sp_write_byte (j);
      sp_write_byte (byte(0x91));
      sp_write_float (preset->effects[i].Parameters[j]);
    }
  }
  sp_write_byte_no_chksum (uint8_t(sp_out_msg_chksum % 256));  
  sp_end_message();
}

//
//
//

void SparkIO::sp_out_store(uint8_t b)
{
  uint8_t bits;
  
  if (sp_oc_bit_mask == 0x80) {
    sp_oc_bit_mask = 1;
    sp_oc_bit_pos = sp_out_chunk.get_pos();
    sp_out_chunk.add(0);
  }
  
  if (b & 0x80) {
    sp_out_chunk.set_bit_at_index(sp_oc_bit_pos, sp_oc_bit_mask);
    sp_oc_checksum ^= sp_oc_bit_mask;
  }
  sp_out_chunk.add(b & 0x7f);
  sp_oc_checksum ^= (b & 0x7f);

  sp_oc_len++;

  /*
  if (oc_bit_mask == 0x40) {
    out_chunk.get_at_index(oc_bit_pos, &bits);
    oc_checksum ^= bits;    
  }
*/  
  sp_oc_bit_mask *= 2;
}


void SparkIO::sp_process_out_chunks() {
  int i, j, len;
  int checksum_pos;
  uint8_t b;
  uint8_t len_h, len_l;

  uint8_t num_chunks, this_chunk, this_len;
 
  while (!sp_out_message.is_empty()) {
    sp_out_message.get(&sp_oc_cmd);
    sp_out_message.get(&sp_oc_sub);
    sp_out_message.get(&len_h);
    sp_out_message.get(&len_l);
    bytes_to_uint(len_h, len_l, &sp_oc_len);
    len = sp_oc_len -4;

    if (len > 0x80) { //this is a multi-chunk message
      num_chunks = int(len / 0x80) + 1;
      for (this_chunk=0; this_chunk < num_chunks; this_chunk++) {
       
        // create chunk header
        sp_out_chunk.add(0xf0);
        sp_out_chunk.add(0x01);
        sp_out_chunk.add(sp_oc_seq);
        
        sp_oc_seq++;
        if (sp_oc_seq > 0x7f) sp_oc_seq = 0x20;
        
        checksum_pos = sp_out_chunk.get_pos();
        sp_out_chunk.add(0); // checksum
        
        sp_out_chunk.add(sp_oc_cmd);
        sp_out_chunk.add(sp_oc_sub);

        if (num_chunks == this_chunk+1) 
          this_len = len % 0x80; 
        else 
          this_len = 0x80;

        sp_oc_bit_mask = 0x80;
        sp_oc_checksum = 0;
        
        // create chunk sub-header          
        sp_out_store(num_chunks);
        sp_out_store(this_chunk);
        sp_out_store(this_len);
        
        for (i = 0; i < this_len; i++) {
          sp_out_message.get(&b);
          sp_out_store(b);
        }
        sp_out_chunk.set_at_index(checksum_pos, sp_oc_checksum);        
        sp_out_chunk.add(0xf7);
      }
    } 
    else { 
    // create chunk header
      sp_out_chunk.add(0xf0);
      sp_out_chunk.add(0x01);
      sp_out_chunk.add(sp_oc_seq);

      checksum_pos = sp_out_chunk.get_pos();
      sp_out_chunk.add(0); // checksum

      sp_out_chunk.add(sp_oc_cmd);
      sp_out_chunk.add(sp_oc_sub);

      sp_oc_bit_mask = 0x80;
      sp_oc_checksum = 0;
      for (i = 0; i < len; i++) {
        sp_out_message.get(&b);
        sp_out_store(b);
      }
     sp_out_chunk.set_at_index(checksum_pos, sp_oc_checksum);        
     sp_out_chunk.add(0xf7);
    }
    sp_out_chunk.commit();
  }
}

void SparkIO::sp_process_out_blocks() {
  int i;
  int len;
  uint8_t b;  
  uint8_t cmd, sub;

  while (!sp_out_chunk.is_empty() && sp_ob_ok_to_send) {
    sp_ob_pos = 16;
  
    sp_out_block[0]= 0x01;
    sp_out_block[1]= 0xfe;  
    sp_out_block[2]= 0x00;    
    sp_out_block[3]= 0x00;
    sp_out_block[4]= 0x53;
    sp_out_block[5]= 0xfe;
    sp_out_block[6]= 0x00;
    for (i=7; i<16;i++) 
      sp_out_block[i]= 0x00;
    
    b = 0;
    while (b != 0xf7) {
      sp_out_chunk.get(&b);

      // look for cmd and sub in the stream and set blocking to true if 0x0101 found - multi chunk
      // not sure if that should be here because it means the block send needs to understand the chunks content
      // perhaps it should be between converting msgpack to chunks and put flow control in there
      if (sp_ob_pos == 20) 
        cmd = b;
      if (sp_ob_pos == 21) {
        sub = b;
        if (cmd == 0x01 && sub == 0x01) 
          sp_ob_ok_to_send = false;
      }
      sp_out_block[sp_ob_pos++] = b;
    }
    sp_out_block[6] = sp_ob_pos;

    sp_write(sp_out_block, sp_ob_pos);
    sp_ob_last_sent_time = millis();

    
    if (!sp_ob_ok_to_send) {
      DEBUG("Blocked");
    }
  }
}





// 
// Main processing routine
//

void SparkIO::app_process() 
{
  // process inputs
  app_process_in_blocks();
  app_process_in_chunks();

/* 
  if (!in_message.is_empty()) {
    Serial.print("FROM SPARK ");
    in_message.dump2();
  }
  
  
  if (!out_message.is_empty()) {
    Serial.print("TO SPARK ");
    out_message.dump2();
  }

  
  if (!ob_ok_to_send && (millis() - ob_last_sent_time > 500)) {
    DEBUG("Timeout on send");
    ob_ok_to_send = true;
  }
*/

  // process outputs
  
  app_process_out_chunks();
  app_process_out_blocks();
}


//
// Routine to read the block from bluetooth and put into the in_chunk ring buffer
//

uint8_t chunk_header_to_spark[16]{0x01, 0xfe, 0x00, 0x00, 0x53, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void SparkIO::app_process_in_blocks() {
  uint8_t b;
  bool boo;

  while (app_available()) {
    b = app_read();

    // **** PASSTHROUGH OF SERIAL TO BLUETOOTH ****

    if (app_pass_through) {
      if (app_ser_state == 0 && b == 0x01) {
        app_ser_state = 1;
      }
      else if (app_ser_state == 1) {
        if (b == 0xfe) {
          app_ser_state = 2;
          app_ser_buf[0] = 0x01;
          app_ser_buf[1] = 0xfe;
          app_ser_pos = 2;
        }
        else 
          app_ser_state = 0;
      }
      else if (app_ser_state == 2) {
        if (app_ser_pos == 6) {
          app_ser_len = b;
        }
        app_ser_buf[app_ser_pos++] = b;
        if (app_ser_pos == app_ser_len) {
          sp_write(app_ser_buf, app_ser_pos);   
          app_ser_pos = 0;
          app_ser_len = -1; 
          app_ser_state = 0; 
        }
      }
      if (app_ser_pos > MAX_SER_BUFFER) {
        DEBUG("APPIO IO_PROCESS_IN_BLOCKS OVERRUN");
        while (true);
      }
    }
    
    // **** END PASSTHROUGH ****

    // check the 7th byte which holds the block length
    if (app_rb_state == 6) {
      app_rb_len = b - 16;
      app_rb_state++;
    }
    // check every other byte in the block header for a match to the header standard
    else if (app_rb_state > 0 && app_rb_state < 16) {
      if (b == chunk_header_to_spark[app_rb_state]) {
        app_rb_state++;
      }
      else {
        app_rb_state = 0;
        DEBUG("SparkAppIO bad block header");
      }
    } 
    // and once past the header just read the next bytes as defined by rb_len
    // store these to the chunk buffer
    else if (app_rb_state == 16) {
      app_in_chunk.add(b);
      app_rb_len--;
      if (app_rb_len == 0) {
        app_rb_state = 0;
        app_in_chunk.commit();
      }
    }
      
    // checking for rb_state 0 is done separately so that if a prior step finds a mismatch
    // and resets the state to 0 it can be processed here for that byte - saves missing the 
    // first byte of the header if that was misplaced
    
    if (app_rb_state == 0) 
      if (b == chunk_header_to_spark[0]) 
        app_rb_state++;
  }
}

//
// Routine to read chunks from the in_chunk ring buffer and copy to a in_message msgpack buffer
//

void SparkIO::app_process_in_chunks() {
  uint8_t b;
  bool boo;
  unsigned int len;
  uint8_t len_h, len_l;

  while (!app_in_chunk.is_empty()) {               // && in_message.is_empty()) {  -- no longer needed because in_message is now a proper ringbuffer
    boo = app_in_chunk.get(&b);
    if (!boo) DEBUG("Chunk is_empty was false but the buffer was empty!");

    switch (app_rc_state) {
      case 1:
        if (b == 0x01) 
          app_rc_state++; 
        else 
          app_rc_state = 0; 
        break;
      case 2:
        app_rc_seq = b; 
        app_rc_state++; 
        break;
      case 3:
        app_rc_checksum = b;
        app_rc_state++; 
        break;
      case 4:
        app_rc_cmd = b; 
        app_rc_state++; 
        break;
      case 5:
        app_rc_sub = b; 
        app_rc_state = 10;
/*
        // flow control for blocking sends - put here in case we want to check rc_sub too
        if (rc_cmd == 0x04 && rc_sub == 0x01) {
          ob_ok_to_send = true;
          DEBUG("Unblocked");
        }
*/        
        // set up for the main data loop - rc_state 10
        app_rc_bitmask = 0x80;
        app_rc_calc_checksum = 0;
        app_rc_data_pos = 0;
        
        // check for multi-chunk
        if (app_rc_cmd == 1 && app_rc_sub == 1) 
          app_rc_multi_chunk = true;
        else {
          app_rc_multi_chunk = false;
          app_in_message_bad = false;
          app_in_message.add(app_rc_cmd);
          app_in_message.add(app_rc_sub);
          app_in_message.add(0);
          app_in_message.add(0);
        }
        break;
      case 10:                    // the main loop which ends on an 0xf7
        if (b == 0xf7) {
          if (app_rc_calc_checksum != app_rc_checksum) 
            app_in_message_bad = true;
          app_rc_state = 0;
          if (!app_rc_multi_chunk || (app_rc_this_chunk == app_rc_total_chunks-1)) { //last chunk in message
            if (app_in_message_bad) {
              DEBUG("Bad message, dropped");
              app_in_message.drop();
            }
            else {
              len = app_in_message.get_len();
              uint_to_bytes(len, &len_h, &len_l);

              app_in_message.set_at_index(2, len_h);
              app_in_message.set_at_index(3, len_l);
              app_in_message.commit();

// TRACE
//              Serial.print("From App       ");
//              app_in_message.dump3();
            }  
          }
        }
        else if (app_rc_bitmask == 0x80) { // if the bitmask got to this value it is now a new bits 
          app_rc_calc_checksum ^= b;
          app_rc_bits = b;
          app_rc_bitmask = 1;
        }
        else {
          app_rc_data_pos++;
          app_rc_calc_checksum ^= b;          
          if (app_rc_bits & app_rc_bitmask) 
            b |= 0x80;
          app_rc_bitmask *= 2;
          
          if (app_rc_multi_chunk && app_rc_data_pos == 1) 
            app_rc_total_chunks = b;
          else if (app_rc_multi_chunk && app_rc_data_pos == 2) {
            app_rc_last_chunk = app_rc_this_chunk;
            app_rc_this_chunk = b;
            if (app_rc_this_chunk == 0) {
              app_in_message_bad = false;
              app_in_message.add(app_rc_cmd);
              app_in_message.add(app_rc_sub);
              app_in_message.add(0);
              app_in_message.add(0);
            }
            else if (app_rc_this_chunk != app_rc_last_chunk+1)
              app_in_message_bad = true;
          }
          else if (app_rc_multi_chunk && app_rc_data_pos == 3) 
            app_rc_chunk_len = b;
          else {  
            app_in_message.add(b);             
          }
          
        };
        break;
    }

    // checking for rc_state 0 is done separately so that if a prior step finds a mismatch
    // and resets the state to 0 it can be processed here for that byte - saves missing the 
    // first byte of the header if that was misplaced
    
    if (app_rc_state == 0) {
      if (b == 0xf0) 
        app_rc_state++;
    }
  }
}

//// Routines to interpret the data

void SparkIO::app_read_byte(uint8_t *b)
{
  uint8_t a;
  app_in_message.get(&a);
  *b = a;
}   
   
void SparkIO::app_read_string(char *str)
{
  uint8_t a, len;
  int i;

  app_read_byte(&a);
  if (a == 0xd9) {
    app_read_byte(&len);
  }
  else if (a > 0xa0) {
    len = a - 0xa0;
  }
  else {
    app_read_byte(&a);
    if (a < 0xa1 || a >= 0xc0) DEBUG("Bad string");
    len = a - 0xa0;
  }

  if (len > 0) {
    // process whole string but cap it at STR_LEN-1
    for (i = 0; i < len; i++) {
      app_read_byte(&a);
      if (a<0x20 || a>0x7e) a=0x20; // make sure it is in ASCII range - to cope with get_serial 
      if (i < STR_LEN -1) str[i]=a;
    }
    str[i > STR_LEN-1 ? STR_LEN-1 : i]='\0';
  }
  else {
    str[0]='\0';
  }
}   

void SparkIO::app_read_prefixed_string(char *str)
{
  uint8_t a, len;
  int i;

  app_read_byte(&a); 
  app_read_byte(&a);

  if (a < 0xa1 || a >= 0xc0) DEBUG("Bad string");
  len = a-0xa0;

  if (len > 0) {
    for (i = 0; i < len; i++) {
      app_read_byte(&a);
      if (a<0x20 || a>0x7e) a=0x20; // make sure it is in ASCII range - to cope with get_serial 
      if (i < STR_LEN -1) str[i]=a;
    }
    str[i > STR_LEN-1 ? STR_LEN-1 : i]='\0';
  }
  else {
    str[0]='\0';
  }
}   

void SparkIO::app_read_float(float *f)
{
  union {
    float val;
    byte b[4];
  } conv;   
  uint8_t a;
  int i;

  app_read_byte(&a);  // should be 0xca
  if (a != 0xca) return;

  // Seems this creates the most significant byte in the last position, so for example
  // 120.0 = 0x42F00000 is stored as 0000F042  
   
  for (i=3; i>=0; i--) {
    app_read_byte(&a);
    conv.b[i] = a;
  } 
  *f = conv.val;
}

void SparkIO::app_read_onoff(bool *b)
{
  uint8_t a;
   
  app_read_byte(&a);
  if (a == 0xc3)
    *b = true;
  else // 0xc2
    *b = false;
}

// The functions to get the message

bool SparkIO::app_get_message(unsigned int *cmdsub, SparkMessage *msg, SparkPreset *preset)
{
  uint8_t cmd, sub, len_h, len_l;
  unsigned int len;
  unsigned int cs;
   
  uint8_t junk;
  int i, j;
  uint8_t num;

  if (app_in_message.is_empty()) return false;

  app_read_byte(&cmd);
  app_read_byte(&sub);
  app_read_byte(&len_h);
  app_read_byte(&len_l);
  
  bytes_to_uint(len_h, len_l, &len);
  bytes_to_uint(cmd, sub, &cs);

  *cmdsub = cs;
  switch (cs) {
    // 0x02 series - requests
    // get preset information
    case 0x0201:
      app_read_byte(&msg->param1);
      app_read_byte(&msg->param2);
      for (i=0; i < 30; i++) app_read_byte(&junk); // 30 bytes of 0x00
      break;            
    // get current hardware preset number - this is a request with no payload
    case 0x0210:
      break;
    // get amp name - no payload
    case 0x0211:
      break;
    // get name - this is a request with no payload
    case 0x0221:
      break;
    // get serial number - this is a request with no payload
    case 0x0223:
      break;
    // the UNKNOWN command - 0x0224 00 01 02 03
    case 0x0224:
      // the data is a fixed array of four bytes (0x94 00 01 02 03)
      app_read_byte(&junk);
      app_read_byte(&msg->param1);
      app_read_byte(&msg->param2);
      app_read_byte(&msg->param3);
      app_read_byte(&msg->param4);
      break;
    // get firmware version - this is a request with no payload
    case 0x022f:
      break;
    // 0x01 series - instructions
    // change effect parameter
    case 0x0104:
      app_read_string(msg->str1);
      app_read_byte(&msg->param1);
      app_read_float(&msg->val);
      break;
    // change effect model
    case 0x0106:
      app_read_string(msg->str1);
      app_read_string(msg->str2);
      break;
    // enable / disable effecct
    case 0x0115:
      app_read_string(msg->str1);
      app_read_onoff(&msg->onoff);
      break;    
    // change preset to 0-3, 0x7f
    case 0x0138:
      app_read_byte(&msg->param1);
      app_read_byte(&msg->param2);
      break;
    // send whole new preset to 0-3, 0x7f  
    case 0x0101:
      app_read_byte(&junk);
      app_read_byte(&preset->preset_num);
      app_read_string(preset->UUID); 
      app_read_string(preset->Name);
      app_read_string(preset->Version);
      app_read_string(preset->Description);
      app_read_string(preset->Icon);
      app_read_float(&preset->BPM);

      for (j=0; j<7; j++) {
        app_read_string(preset->effects[j].EffectName);
        app_read_onoff(&preset->effects[j].OnOff);
        app_read_byte(&num);
        preset->effects[j].NumParameters = num - 0x90;
        for (i = 0; i < preset->effects[j].NumParameters; i++) {
          app_read_byte(&junk);
          app_read_byte(&junk);
          app_read_float(&preset->effects[j].Parameters[i]);
        }
      }
      app_read_byte(&preset->chksum);  
      break;
    default:
      Serial.print("Unprocessed message SparkAppIO ");
      Serial.print (cs, HEX);
      Serial.print(":");
      for (i = 0; i < len - 4; i++) {
        app_read_byte(&junk);
        Serial.print(junk, HEX);
        Serial.print(" ");
      }
      Serial.println();
  }

  return true;
}

    
//
// Output routines
//


void SparkIO::app_start_message(int cmdsub)
{
  app_om_cmd = (cmdsub & 0xff00) >> 8;
  app_om_sub = cmdsub & 0xff;

  app_out_message.add(app_om_cmd);
  app_out_message.add(app_om_sub);
  app_out_message.add(0);      // placeholder for length
  app_out_message.add(0);      // placeholder for length
}


void SparkIO::app_end_message()
{
  unsigned int len;
  uint8_t len_h, len_l;
  
  len = app_out_message.get_len();
  uint_to_bytes(len, &len_h, &len_l);
  
  app_out_message.set_at_index(2, len_h);   
  app_out_message.set_at_index(3, len_l);
  app_out_message.commit();
}


void SparkIO::app_write_byte(byte b)
{
  app_out_message.add(b);
}

void SparkIO::app_write_prefixed_string(const char *str)
{
  int len, i;

  len = strnlen(str, STR_LEN);
  app_write_byte(byte(len));
  app_write_byte(byte(len + 0xa0));
  for (i=0; i<len; i++)
    app_write_byte(byte(str[i]));
}

void SparkIO::app_write_string(const char *str)
{
  int len, i;

  len = strnlen(str, STR_LEN);
  app_write_byte(byte(len + 0xa0));
  for (i=0; i<len; i++)
    app_write_byte(byte(str[i]));
}      
  
void SparkIO::app_write_long_string(const char *str)
{
  int len, i;

  len = strnlen(str, STR_LEN);
  app_write_byte(byte(0xd9));
  app_write_byte(byte(len));
  for (i=0; i<len; i++)
    app_write_byte(byte(str[i]));
}

void SparkIO::app_write_float (float flt)
{
  union {
    float val;
    byte b[4];
  } conv;
  int i;
   
  conv.val = flt;
  // Seems this creates the most significant byte in the last position, so for example
  // 120.0 = 0x42F00000 is stored as 0000F042  
   
  app_write_byte(0xca);
  for (i=3; i>=0; i--) {
    app_write_byte(byte(conv.b[i]));
  }
}

void SparkIO::app_write_onoff (bool onoff)
{
  byte b;

  if (onoff)
  // true is 'on'
    b = 0xc3;
  else
    b = 0xc2;
  app_write_byte(b);
}

//
//
//

void SparkIO::app_change_effect_parameter (char *pedal, int param, float val)
{
   app_start_message (0x0337);
   app_write_prefixed_string (pedal);
   app_write_byte (byte(param));
   app_write_float(val);
   app_end_message();
}


void SparkIO::app_change_effect (char *pedal1, char *pedal2)
{
   app_start_message (0x0306);
   app_write_prefixed_string (pedal1);
   app_write_prefixed_string (pedal2);
   app_end_message();
}

void SparkIO::app_change_hardware_preset (uint8_t preset_num)
{
   // preset_num is 0 to 3

   app_start_message (0x0338);
   app_write_byte (0);
   app_write_byte (preset_num);     
   app_end_message();  
}

void SparkIO::app_turn_effect_onoff (char *pedal, bool onoff)
{
   app_start_message (0x0315);
   app_write_prefixed_string (pedal);
   app_write_onoff (onoff);
   app_end_message();
}

void SparkIO::app_save_hardware_preset(uint8_t preset_num)
{
   app_start_message (0x0327);
   app_write_byte (0);
   app_write_byte (preset_num);  
   app_end_message();
}

void SparkIO::app_create_preset(SparkPreset *preset)
{
  int i, j, siz;

  app_start_message (0x0301);

  app_write_byte (0x00);
  app_write_byte (preset->preset_num);   
  app_write_long_string (preset->UUID);
  app_write_string (preset->Name);
  app_write_string (preset->Version);
  if (strnlen (preset->Description, STR_LEN) > 31)
    app_write_long_string (preset->Description);
  else
    app_write_string (preset->Description);
  app_write_string(preset->Icon);
  app_write_float (preset->BPM);

   
  app_write_byte (byte(0x90 + 7));       // always 7 pedals

  for (i=0; i<7; i++) {
      
    app_write_string (preset->effects[i].EffectName);
    app_write_onoff(preset->effects[i].OnOff);

    siz = preset->effects[i].NumParameters;
    app_write_byte ( 0x90 + siz); 
      
    for (j=0; j<siz; j++) {
      app_write_byte (j);
      app_write_byte (byte(0x91));
      app_write_float (preset->effects[i].Parameters[j]);
    }
  }
  app_write_byte (preset->chksum);  
  app_end_message();
}

//
//
//

void SparkIO::app_out_store(uint8_t b)
{
  uint8_t bits;
  
  if (app_oc_bit_mask == 0x80) {
    app_oc_bit_mask = 1;
    app_oc_bit_pos = app_out_chunk.get_pos();
    app_out_chunk.add(0);
  }
  
  if (b & 0x80) {
    app_out_chunk.set_bit_at_index(app_oc_bit_pos, app_oc_bit_mask);
    app_oc_checksum ^= app_oc_bit_mask;
  }
  app_out_chunk.add(b & 0x7f);
  app_oc_checksum ^= (b & 0x7f);

  app_oc_len++;

  /*
  if (oc_bit_mask == 0x40) {
    out_chunk.get_at_index(oc_bit_pos, &bits);
    oc_checksum ^= bits;    
  }
*/  
  app_oc_bit_mask *= 2;
}


void SparkIO::app_process_out_chunks() {
  int i, j, len;
  int checksum_pos;
  uint8_t b;
  uint8_t len_h, len_l;

  uint8_t num_chunks, this_chunk, this_len;
 
  while (!app_out_message.is_empty()) {
    app_out_message.get(&app_oc_cmd);
    app_out_message.get(&app_oc_sub);
    app_out_message.get(&len_h);
    app_out_message.get(&len_l);
    bytes_to_uint(len_h, len_l, &app_oc_len);
    len = app_oc_len -4;

    if (len > 0x19) { //this is a multi-chunk message for amp to app (max 0x19 data)
      num_chunks = int(len / 0x19) + 1;
      for (this_chunk=0; this_chunk < num_chunks; this_chunk++) {
       
        // create chunk header
        app_out_chunk.add(0xf0);
        app_out_chunk.add(0x01);
        if (app_oc_sub == 0x01)        // asked for a preset
          app_out_chunk.add(app_rc_seq);   // last sequence received
        else {
          app_out_chunk.add(app_oc_seq);
          app_oc_seq++;
          if (app_oc_seq > 0x7f) app_oc_seq = 0x40;
        }
        
        checksum_pos = app_out_chunk.get_pos();
        app_out_chunk.add(0); // checksum
        
        app_out_chunk.add(app_oc_cmd);
        app_out_chunk.add(app_oc_sub);

        if (num_chunks == this_chunk+1) 
          this_len = len % 0x19;            
        else 
          this_len = 0x19;                  

        app_oc_bit_mask = 0x80;
        app_oc_checksum = 0;
        
        // create chunk sub-header          
        app_out_store(num_chunks);
        app_out_store(this_chunk);
        app_out_store(this_len);
        
        for (i = 0; i < this_len; i++) {
          app_out_message.get(&b);
          app_out_store(b);
        }
        app_out_chunk.set_at_index(checksum_pos, app_oc_checksum);        
        app_out_chunk.add(0xf7);
      }
    } 
    else { 
    // create chunk header
      app_out_chunk.add(0xf0);
      app_out_chunk.add(0x01);
      app_out_chunk.add(app_oc_seq);

      checksum_pos = app_out_chunk.get_pos();
      app_out_chunk.add(0); // checksum

      app_out_chunk.add(app_oc_cmd);
      app_out_chunk.add(app_oc_sub);

      app_oc_bit_mask = 0x80;
      app_oc_checksum = 0;
      for (i = 0; i < len; i++) {
        app_out_message.get(&b);
        app_out_store(b);
      }
      app_out_chunk.set_at_index(checksum_pos, app_oc_checksum);        
      app_out_chunk.add(0xf7);
    }
    app_out_chunk.commit();
  }
}

void SparkIO::app_process_out_blocks() {
  int i;
  int len;
  uint8_t b;  
  uint8_t cmd, sub;

  while (!app_out_chunk.is_empty()) {
    app_ob_pos = 16;
  
    app_out_block[0]= 0x01;
    app_out_block[1]= 0xfe;  
    app_out_block[2]= 0x00;    
    app_out_block[3]= 0x00;
    app_out_block[4]= 0x41;
    app_out_block[5]= 0xff;
    app_out_block[6]= 0x00;
    for (i=7; i<16;i++) 
      app_out_block[i]= 0x00;
    
    b = 0;
    while (app_ob_pos < 0x6a && !app_out_chunk.is_empty()) {
      app_out_chunk.get(&b);
      app_out_block[app_ob_pos++] = b;
    }
    app_out_block[6] = app_ob_pos;
/*
    for (i=0; i<ob_pos;i++) {
      if (out_block[i]<16) Serial.print("0");
      Serial.print(out_block[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
*/    
    app_write(app_out_block, app_ob_pos);
    delay(50);
  }
}
