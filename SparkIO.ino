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
 
//
// SparkIO class
//

SparkIO::SparkIO(bool passthru) {
  pass_through = passthru;
  rb_state = 0;
  rc_state = 0;
  oc_seq = 0x20;
  ob_ok_to_send = true;
  ob_last_sent_time = millis();
  
  bt_state = 0;
  bt_pos = 0;
  bt_len = -1;
}

SparkIO::~SparkIO() {
 
}


// 
// Main processing routine
//

void SparkIO::process() 
{
  // process inputs
  process_in_blocks();
  process_in_chunks();

  if (!ob_ok_to_send && (millis() - ob_last_sent_time > 500)) {
    DEBUG("Timeout on send");
    ob_ok_to_send = true;
  }

  // process outputs

 
  process_out_chunks();
  process_out_blocks();

}


//
// Routine to read the block from bluetooth and put into the in_chunk ring buffer
//

uint8_t chunk_header_from_spark[16]{0x01, 0xfe, 0x00, 0x00, 0x41, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void SparkIO::process_in_blocks() {
  uint8_t b;
  bool boo;

  while (bt_available()) {
    b = bt_read();
   
    // **** PASSTHROUGH OF SERIAL TO BLUETOOTH ****

    if (pass_through) {
      if (bt_state == 0 && b == 0x01) 
        bt_state = 1;
      else if (bt_state == 1) {
        if (b == 0xfe) {
          bt_state = 2;
          bt_buf[0] = 0x01;
          bt_buf[1] = 0xfe;
          bt_pos = 2;
        }
        else
          bt_state = 0;
      }
      else if (bt_state == 2) {
        if (bt_pos == 6) {
          bt_len = b;
        }
        bt_buf[bt_pos++] = b;
        if (bt_pos == bt_len) {
          ser_write(bt_buf, bt_pos);
          bt_pos = 0;
          bt_len = -1; 
          bt_state = 0; 
        }
      }

      if (bt_pos > MAX_BT_BUFFER) {
        Serial.println("SPARKIO IO_PROCESS_IN_BLOCKS OVERRUN");
        while (true);
      }
    }
    // **** END PASSTHROUGH ****
    
    // check the 7th byte which holds the block length
    if (rb_state == 6) {
      rb_len = b - 16;
      rb_state++;
    }
    // check every other byte in the block header for a match to the header standard
    else if (rb_state >= 0 && rb_state < 16) {
      if (b == chunk_header_from_spark[rb_state]) {
        rb_state++;
      }
      else {
        Serial.print (rb_state);
        Serial.print(" ");
        Serial.print(b);
        Serial.print(" ");
        Serial.print(rb_len);
        Serial.println();
        rb_state = -1;
        DEBUG("SparkIO bad block header");
      }
    } 
    // and once past the header just read the next bytes as defined by rb_len
    // store these to the chunk buffer
    else if (rb_state == 16) {
      in_chunk.add(b);
      rb_len--;
      if (rb_len == 0) {
        rb_state = 0;
        in_chunk.commit();
      }
    }
      
    // checking for rb_state 0 is done separately so that if a prior step finds a mismatch
    // and resets the state to 0 it can be processed here for that byte - saves missing the 
    // first byte of the header if that was misplaced
    
    if (rb_state == -1) 
      if (b == chunk_header_from_spark[0]) 
        rb_state = 1;
  }
}

//
// Routine to read chunks from the in_chunk ring buffer and copy to a in_message msgpack buffer
//

void SparkIO::process_in_chunks() {
  uint8_t b;
  bool boo;
  unsigned int len;
  uint8_t len_h, len_l;

  while (!in_chunk.is_empty()) {               // && in_message.is_empty()) {  -- no longer needed because in_message is now a proper ringbuffer
    boo = in_chunk.get(&b);
    if (!boo) DEBUG("Chunk is_empty was false but the buffer was empty!");

    switch (rc_state) {
      case 1:
        if (b == 0x01) 
          rc_state++; 
        else 
          rc_state = 0; 
        break;
      case 2:
        rc_seq = b; 
        rc_state++; 
        break;
      case 3:
        rc_checksum = b;
        rc_state++; 
        break;
      case 4:
        rc_cmd = b; 
        rc_state++; 
        break;
      case 5:
        rc_sub = b; 
        rc_state = 10;

        // flow control for blocking sends - put here in case we want to check rc_sub too
        if (rc_cmd == 0x04 && rc_sub == 0x01) {
          if (ob_ok_to_send == false) {
            ob_ok_to_send = true;
            DEBUG("Unblocked");
          }
        }
        
        // set up for the main data loop - rc_state 10
        rc_bitmask = 0x80;
        rc_calc_checksum = 0;
        rc_data_pos = 0;
        
        // check for multi-chunk
        if (rc_cmd == 3 && rc_sub == 1) 
          rc_multi_chunk = true;
        else {
          rc_multi_chunk = false;
          in_message_bad = false;
          in_message.add(rc_cmd);
          in_message.add(rc_sub);
          in_message.add(0);
          in_message.add(0);
        }
        break;
      case 10:                    // the main loop which ends on an 0xf7
        if (b == 0xf7) {
          if (rc_calc_checksum != rc_checksum) 
            in_message_bad = true;
          rc_state = 0;
          if (!rc_multi_chunk || (rc_this_chunk == rc_total_chunks-1)) { //last chunk in message
            if (in_message_bad) {
              DEBUG("Bad message, dropped");
              in_message.drop();
            }
            else {
              len = in_message.get_len();
              uint_to_bytes(len, &len_h, &len_l);

              in_message.set_at_index(2, len_h);
              in_message.set_at_index(3, len_l);
              in_message.commit();
            }  
          }
        }
        else if (rc_bitmask == 0x80) { // if the bitmask got to this value it is now a new bits 
          rc_calc_checksum ^= b;
          rc_bits = b;
          rc_bitmask = 1;
        }
        else {
          rc_data_pos++;
          rc_calc_checksum ^= b;          
          if (rc_bits & rc_bitmask) 
            b |= 0x80;
          rc_bitmask *= 2;
          
          if (rc_multi_chunk && rc_data_pos == 1) 
            rc_total_chunks = b;
          else if (rc_multi_chunk && rc_data_pos == 2) {
            rc_last_chunk = rc_this_chunk;
            rc_this_chunk = b;
            if (rc_this_chunk == 0) {
              in_message_bad = false;
              in_message.add(rc_cmd);
              in_message.add(rc_sub);
              in_message.add(0);
              in_message.add(0);
            }
            else if (rc_this_chunk != rc_last_chunk+1)
              in_message_bad = true;
          }
          else if (rc_multi_chunk && rc_data_pos == 3) 
            rc_chunk_len = b;
          else {  
            in_message.add(b);             
          }
          
        };
        break;
    }

    // checking for rc_state 0 is done separately so that if a prior step finds a mismatch
    // and resets the state to 0 it can be processed here for that byte - saves missing the 
    // first byte of the header if that was misplaced
    
    if (rc_state == 0) {
      if (b == 0xf0) 
        rc_state++;
    }
  }
}

//// Routines to interpret the data

void SparkIO::read_byte(uint8_t *b)
{
  uint8_t a;
  in_message.get(&a);
  *b = a;
}   
   
void SparkIO::read_string(char *str)
{
  uint8_t a, len;
  int i;

  read_byte(&a);
  if (a == 0xd9) {
    read_byte(&len);
  }
  else if (a >= 0xa0) {
    len = a - 0xa0;
  }
  else {
    read_byte(&a);
    if (a < 0xa0 || a >= 0xc0) DEBUG("Bad string");
    len = a - 0xa0;
  }

  if (len > 0) {
    // process whole string but cap it at STR_LEN-1
    for (i = 0; i < len; i++) {
      read_byte(&a);
      if (a<0x20 || a>0x7e) a=0x20; // make sure it is in ASCII range - to cope with get_serial 
      if (i < STR_LEN -1) str[i]=a;
    }
    str[i > STR_LEN-1 ? STR_LEN-1 : i]='\0';
  }
  else {
    str[0]='\0';
  }
}   

void SparkIO::read_prefixed_string(char *str)
{
  uint8_t a, len;
  int i;

  read_byte(&a); 
  read_byte(&a);

  if (a < 0xa0 || a >= 0xc0) DEBUG("Bad string");
  len = a-0xa0;

  if (len > 0) {
    for (i = 0; i < len; i++) {
      read_byte(&a);
      if (a<0x20 || a>0x7e) a=0x20; // make sure it is in ASCII range - to cope with get_serial 
      if (i < STR_LEN -1) str[i]=a;
    }
    str[i > STR_LEN-1 ? STR_LEN-1 : i]='\0';
  }
  else {
    str[0]='\0';
  }
}   

void SparkIO::read_float(float *f)
{
  union {
    float val;
    byte b[4];
  } conv;   
  uint8_t a;
  int i;

  read_byte(&a);  // should be 0xca
  if (a != 0xca) return;

  // Seems this creates the most significant byte in the last position, so for example
  // 120.0 = 0x42F00000 is stored as 0000F042  
   
  for (i=3; i>=0; i--) {
    read_byte(&a);
    conv.b[i] = a;
  } 
  *f = conv.val;
}

void SparkIO::read_onoff(bool *b)
{
  uint8_t a;
   
  read_byte(&a);
  if (a == 0xc3)
    *b = true;
  else // 0xc2
    *b = false;
}

// The functions to get the message

bool SparkIO::get_message(unsigned int *cmdsub, SparkMessage *msg, SparkPreset *preset)
{
  uint8_t cmd, sub, len_h, len_l;
  unsigned int len;
  unsigned int cs;
   
  uint8_t junk;
  int i, j;
  uint8_t num;

  if (in_message.is_empty()) return false;

  read_byte(&cmd);
  read_byte(&sub);
  read_byte(&len_h);
  read_byte(&len_l);
  
  bytes_to_uint(len_h, len_l, &len);
  bytes_to_uint(cmd, sub, &cs);

  *cmdsub = cs;
  switch (cs) {
    // change of effect model
    case 0x0306:
      read_string(msg->str1);
      read_string(msg->str2);
      break;
    // get current hardware preset number
    case 0x0310:
      read_byte(&msg->param1);
      read_byte(&msg->param2);
      break;
    // get name
    case 0x0311:
      read_string(msg->str1);
      break;
    // enable / disable an effect
    case 0x0315:
      read_string(msg->str1);
      read_onoff(&msg->onoff);
      break;
    // get serial number
    case 0x0323:
      read_string(msg->str1);
      break;
    // store into hardware preset
    case 0x0327:
      read_byte(&msg->param1);
      read_byte(&msg->param2);
      break;
    // firmware version number
    case 0x032f:
      // really this is a uint32 but it is just as easy to read into 4 uint8 - a bit of a cheat
      read_byte(&junk);           // this will be 0xce for a uint32
      read_byte(&msg->param1);      
      read_byte(&msg->param2); 
      read_byte(&msg->param3); 
      read_byte(&msg->param4); 
      break;
    // change of effect parameter
    case 0x0337:
      read_string(msg->str1);
      read_byte(&msg->param1);
      read_float(&msg->val);
      break;
    // change of preset number selected on the amp via the buttons
    case 0x0338:
      read_byte(&msg->param1);
      read_byte(&msg->param2);
      break;
    // response to a request for a full preset
    case 0x0301:
      read_byte(&preset->curr_preset);
      read_byte(&preset->preset_num);
      read_string(preset->UUID); 
      read_string(preset->Name);
      read_string(preset->Version);
      read_string(preset->Description);
      read_string(preset->Icon);
      read_float(&preset->BPM);

      for (j=0; j<7; j++) {
        read_string(preset->effects[j].EffectName);
        read_onoff(&preset->effects[j].OnOff);
        read_byte(&num);
        preset->effects[j].NumParameters = num - 0x90;
        for (i = 0; i < preset->effects[j].NumParameters; i++) {
          read_byte(&junk);
          read_byte(&junk);
          read_float(&preset->effects[j].Parameters[i]);
        }
      }
      read_byte(&preset->chksum);  
      break;
    // tap tempo!
    case 0x0363:
      read_float(&msg->val);  
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
        read_byte(&junk);
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


void SparkIO::start_message(int cmdsub)
{
  om_cmd = (cmdsub & 0xff00) >> 8;
  om_sub = cmdsub & 0xff;

  // THIS IS TEMPORARY JUST TO SHOW IT WORKS!!!!!!!!!!!!!!!!
  //sp.out_message.clear();

  out_message.add(om_cmd);
  out_message.add(om_sub);
  out_message.add(0);      // placeholder for length
  out_message.add(0);      // placeholder for length

  out_msg_chksum = 0;
}


void SparkIO::end_message()
{
  unsigned int len;
  uint8_t len_h, len_l;
  
  len = out_message.get_len();
  uint_to_bytes(len, &len_h, &len_l);
  
  out_message.set_at_index(2, len_h);   
  out_message.set_at_index(3, len_l);
  out_message.commit();
}

void SparkIO::write_byte_no_chksum(byte b)
{
  out_message.add(b);
}

void SparkIO::write_byte(byte b)
{
  out_message.add(b);
  out_msg_chksum += int(b);
}

void SparkIO::write_prefixed_string(const char *str)
{
  int len, i;

  len = strnlen(str, STR_LEN);
  write_byte(byte(len));
  write_byte(byte(len + 0xa0));
  for (i=0; i<len; i++)
    write_byte(byte(str[i]));
}

void SparkIO::write_string(const char *str)
{
  int len, i;

  len = strnlen(str, STR_LEN);
  write_byte(byte(len + 0xa0));
  for (i=0; i<len; i++)
    write_byte(byte(str[i]));
}      
  
void SparkIO::write_long_string(const char *str)
{
  int len, i;

  len = strnlen(str, STR_LEN);
  write_byte(byte(0xd9));
  write_byte(byte(len));
  for (i=0; i<len; i++)
    write_byte(byte(str[i]));
}

void SparkIO::write_float (float flt)
{
  union {
    float val;
    byte b[4];
  } conv;
  int i;
   
  conv.val = flt;
  // Seems this creates the most significant byte in the last position, so for example
  // 120.0 = 0x42F00000 is stored as 0000F042  
   
  write_byte(0xca);
  for (i=3; i>=0; i--) {
    write_byte(byte(conv.b[i]));
  }
}

void SparkIO::write_onoff (bool onoff)
{
  byte b;

  if (onoff)
  // true is 'on'
    b = 0xc3;
  else
    b = 0xc2;
  write_byte(b);
}

//
//
//

void SparkIO::change_effect_parameter (char *pedal, int param, float val)
{
   start_message (0x0104);
   write_prefixed_string (pedal);
   write_byte (byte(param));
   write_float(val);
   end_message();
}


void SparkIO::change_effect (char *pedal1, char *pedal2)
{
   start_message (0x0106);
   write_prefixed_string (pedal1);
   write_prefixed_string (pedal2);
   end_message();
}

void SparkIO::change_hardware_preset (uint8_t preset_num)
{
   // preset_num is 0 to 3

   start_message (0x0138);
   write_byte (0);
   write_byte (preset_num)  ;     
   end_message();  
}

void SparkIO::turn_effect_onoff (char *pedal, bool onoff)
{
   start_message (0x0115);
   write_prefixed_string (pedal);
   write_onoff (onoff);
   end_message();
}

void SparkIO::get_serial()
{
   start_message (0x0223);
   end_message();  
}

void SparkIO::get_name()
{
   start_message (0x0211);
   end_message();  
}

void SparkIO::get_hardware_preset_number()
{
   start_message (0x0210);
   end_message();  
}


void SparkIO::get_preset_details(unsigned int preset)
{
   int i;
   uint8_t h, l;

   uint_to_bytes(preset, &h, &l);
   
   start_message (0x0201);
   write_byte(h);
   write_byte(l);

   for (i=0; i<30; i++) {
     write_byte(0);
   }
   
   end_message(); 
}

void SparkIO::create_preset(SparkPreset *preset)
{
  int i, j, siz;

  start_message (0x0101);

  write_byte_no_chksum (0x00);
  write_byte_no_chksum (preset->preset_num);   
  write_long_string (preset->UUID);
  write_string (preset->Name);
  write_string (preset->Version);
  if (strnlen (preset->Description, STR_LEN) > 31)
    write_long_string (preset->Description);
  else
    write_string (preset->Description);
  write_string(preset->Icon);
  write_float (preset->BPM);

   
  write_byte (byte(0x90 + 7));       // always 7 pedals

  for (i=0; i<7; i++) {
      
    write_string (preset->effects[i].EffectName);
    write_onoff(preset->effects[i].OnOff);

    siz = preset->effects[i].NumParameters;
    write_byte ( 0x90 + siz); 
      
    for (j=0; j<siz; j++) {
      write_byte (j);
      write_byte (byte(0x91));
      write_float (preset->effects[i].Parameters[j]);
    }
  }
  write_byte_no_chksum (uint8_t(out_msg_chksum % 256));  
  Serial.print("CHECKSUM ");
  Serial.println(uint8_t(out_msg_chksum % 256));
  end_message();
}

//
//
//

void SparkIO::out_store(uint8_t b)
{
  uint8_t bits;
  
  if (oc_bit_mask == 0x80) {
    oc_bit_mask = 1;
    oc_bit_pos = out_chunk.get_pos();
    out_chunk.add(0);
  }
  
  if (b & 0x80) {
    out_chunk.set_bit_at_index(oc_bit_pos, oc_bit_mask);
    oc_checksum ^= oc_bit_mask;
  }
  out_chunk.add(b & 0x7f);
  oc_checksum ^= (b & 0x7f);

  oc_len++;

  /*
  if (oc_bit_mask == 0x40) {
    out_chunk.get_at_index(oc_bit_pos, &bits);
    oc_checksum ^= bits;    
  }
*/  
  oc_bit_mask *= 2;
}


void SparkIO::process_out_chunks() {
  int i, j, len;
  int checksum_pos;
  uint8_t b;
  uint8_t len_h, len_l;

  uint8_t num_chunks, this_chunk, this_len;
 
  while (!out_message.is_empty()) {
    out_message.get(&oc_cmd);
    out_message.get(&oc_sub);
    out_message.get(&len_h);
    out_message.get(&len_l);
    bytes_to_uint(len_h, len_l, &oc_len);
    len = oc_len -4;

    if (len > 0x80) { //this is a multi-chunk message
      num_chunks = int(len / 0x80) + 1;
      for (this_chunk=0; this_chunk < num_chunks; this_chunk++) {
       
        // create chunk header
        out_chunk.add(0xf0);
        out_chunk.add(0x01);
        out_chunk.add(oc_seq);
        
        oc_seq++;
        if (oc_seq > 0x7f) oc_seq = 0x20;
        
        checksum_pos = out_chunk.get_pos();
        out_chunk.add(0); // checksum
        
        out_chunk.add(oc_cmd);
        out_chunk.add(oc_sub);

        if (num_chunks == this_chunk+1) 
          this_len = len % 0x80; 
        else 
          this_len = 0x80;

        oc_bit_mask = 0x80;
        oc_checksum = 0;
        
        // create chunk sub-header          
        out_store(num_chunks);
        out_store(this_chunk);
        out_store(this_len);
        
        for (i = 0; i < this_len; i++) {
          out_message.get(&b);
          out_store(b);
        }
        out_chunk.set_at_index(checksum_pos, oc_checksum);        
        out_chunk.add(0xf7);
      }
    } 
    else { 
    // create chunk header
      out_chunk.add(0xf0);
      out_chunk.add(0x01);
      out_chunk.add(oc_seq);

      checksum_pos = out_chunk.get_pos();
      out_chunk.add(0); // checksum

      out_chunk.add(oc_cmd);
      out_chunk.add(oc_sub);

      oc_bit_mask = 0x80;
      oc_checksum = 0;
      for (i = 0; i < len; i++) {
        out_message.get(&b);
        out_store(b);
      }
     out_chunk.set_at_index(checksum_pos, oc_checksum);        
     out_chunk.add(0xf7);
    }
    out_chunk.commit();
  }
}

void SparkIO::process_out_blocks() {
  int i;
  int len;
  uint8_t b;  
  uint8_t cmd, sub;

  while (!out_chunk.is_empty() && ob_ok_to_send) {
    ob_pos = 16;
  
    out_block[0]= 0x01;
    out_block[1]= 0xfe;  
    out_block[2]= 0x00;    
    out_block[3]= 0x00;
    out_block[4]= 0x53;
    out_block[5]= 0xfe;
    out_block[6]= 0x00;
    for (i=7; i<16;i++) 
      out_block[i]= 0x00;
    
    b = 0;
    while (b != 0xf7) {
      out_chunk.get(&b);

      // look for cmd and sub in the stream and set blocking to true if 0x0101 found - multi chunk
      // not sure if that should be here because it means the block send needs to understand the chunks content
      // perhaps it should be between converting msgpack to chunks and put flow control in there
      if (ob_pos == 20) 
        cmd = b;
      if (ob_pos == 21) {
        sub = b;
        if (cmd == 0x01 && sub == 0x01) 
          ob_ok_to_send = false;
      }
      out_block[ob_pos++] = b;
    }
    out_block[6] = ob_pos;

    bt_write(out_block, ob_pos);
    ob_last_sent_time = millis();

    
    if (!ob_ok_to_send) {
      DEBUG("Blocked");
    }
  }
}
