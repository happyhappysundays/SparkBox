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

// TO FIX
// *ok_to_send shared across classes, and also checked whilst sending to the app
// app_rc_seq / sp_rc_seq - anaylse and work out how it should work!!!

// UTILITY FUNCTIONS

void uint_to_bytes(unsigned int i, uint8_t *h, uint8_t *l) {
  *h = (i & 0xff00) / 256;
  *l = i & 0xff;
}

void bytes_to_uint(uint8_t h, uint8_t l,unsigned int *i) {
  *i = (h & 0xff) * 256 + (l & 0xff);
}

 
// MAIN SparkIO CLASS

uint8_t chunk_header_from_spark[16]{0x01, 0xfe, 0x00, 0x00, 0x41, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t chunk_header_to_spark[16]  {0x01, 0xfe, 0x00, 0x00, 0x53, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void spark_start(bool passthru) {
  sp_bin.set(passthru, &sp_in_chunk, chunk_header_from_spark);
  app_bin.set(passthru, &app_in_chunk, chunk_header_to_spark);

  sp_cin.set(&sp_in_chunk, &sp_in_message, &sp_ok_to_send, &sp_rec_seq);
  app_cin.set(&app_in_chunk, &app_in_message, &app_ok_to_send, &app_rec_seq);

  spark_msg_in.set(&sp_in_message);
  app_msg_in.set(&app_in_message);

  spark_msg_out.set(&sp_out_message);
  app_msg_out.set(&app_out_message);

  sp_cout.set(&sp_out_chunk, &sp_out_message, &sp_rec_seq);
  app_cout.set(&app_out_chunk, &app_out_message, &app_rec_seq);

  sp_bout.set(&sp_out_chunk, chunk_header_to_spark, &sp_ok_to_send);
  app_bout.set(&app_out_chunk, chunk_header_from_spark, &app_ok_to_send);
}

// 
// Main processing routine
//

void spark_process() 
{
  // process inputs
  sp_bin.process();
  sp_cin.process();

  if (!sp_ok_to_send && (millis() - sp_bout.last_sent_time > 500)) {
    DEBUG("Timeout on send");
    sp_ok_to_send = true;
  }

  // process outputs
  sp_cout.process();
  sp_bout.process();

}


void app_process() 
{
  // process inputs
  app_bin.process();
  app_cin.process();

  // process outputs
  app_cout.process();
  app_bout.process();
}

// BLOCK INPUT ROUTINES 
// Routine to read the block and put into the in_chunk ring buffer

void BlockIn::process() {
  uint8_t b;
  bool boo;

  while (data_available()) {
    b = data_read();
   
    // **** PASSTHROUGH APP AND AMP ****

    if (pass_through) {
      if (io_state == 0 && b == 0x01) 
        io_state = 1;
      else if (io_state == 1) {
        if (b == 0xfe) {
          io_state = 2;
          io_buf[0] = 0x01;
          io_buf[1] = 0xfe;
          io_pos = 2;
        }
        else
          io_state = 0;
      }
      else if (io_state == 2) {
        if (io_pos == 6) {
          io_len = b;
        }
        io_buf[io_pos++] = b;
        if (io_pos == io_len) {
          data_write(io_buf, io_pos);
          io_pos = 0;
          io_len = -1; 
          io_state = 0; 
        }
      }

      if (io_pos > MAX_IO_BUFFER) {
        DEBUG("SPARKIO IO_PROCESS_IN_BLOCKS OVERRUN");
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
      if (b == blk_hdr[rb_state]) {
        rb_state++;
      }
      else {
        Serial.print(rb_state);
        Serial.print(" ");
        Serial.print(b, HEX);
        Serial.print(" ");
        Serial.print(blk_hdr[rb_state], HEX);
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
      rb->add(b);
      rb_len--;
      if (rb_len == 0) {
        rb_state = 0;
        rb->commit();
      }
    }
      
    // checking for rb_state 0 is done separately so that if a prior step finds a mismatch
    // and resets the state to 0 it can be processed here for that byte - saves missing the 
    // first byte of the header if that was misplaced
    
    if (rb_state == -1) 
      if (b == blk_hdr[0]) 
        rb_state = 1;
  }
}

void SparkBlockIn::set(bool pass, RingBuffer *ring_buffer, uint8_t *hdr) {
  rb = ring_buffer;
  blk_hdr = hdr;
  pass_through = pass;
}

bool SparkBlockIn::data_available() {
  return sp_available();
}

uint8_t SparkBlockIn::data_read(){
  return sp_read();
}
void SparkBlockIn::data_write(uint8_t *buf, int len){
  app_write(buf, len);
}

void AppBlockIn::set(bool pass, RingBuffer *ring_buffer, uint8_t *hdr) {
  rb = ring_buffer;
  blk_hdr = hdr;
  pass_through = pass;
}

bool AppBlockIn::data_available() {
  return app_available();
}

uint8_t AppBlockIn::data_read(){
  return app_read();
}

void AppBlockIn::data_write(uint8_t *buf, int len){
  sp_write(buf, len);
}

// CHUNK INPUT ROUTINES
// Routine to read chunks from the in_chunk ring buffer and copy to a in_message msgpack buffer


void SparkChunkIn::set(RingBuffer *chunks, RingBuffer *messages, bool *ok, uint8_t *seq) {
  in_chunk = chunks;
  in_message = messages;
  ok_to_send = ok;
  *ok_to_send = true;
  rec_seq = seq;
}

void AppChunkIn::set(RingBuffer *chunks, RingBuffer *messages, bool *ok, uint8_t *seq) {
  in_chunk = chunks;
  in_message = messages;
  ok_to_send = ok;
  *ok_to_send = true;
  rec_seq = seq;
}

void ChunkIn::process() {
  uint8_t b;
  bool boo;
  unsigned int len;
  uint8_t len_h, len_l;

  while (!in_chunk->is_empty()) {      
    boo = in_chunk->get(&b);
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
        *rec_seq = b;
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
        // in here for amp responses - only triggered by the amp
        if ((rc_cmd == 0x04 || rc_cmd == 0x05) && rc_sub == 0x01) {
          ////////////// THIS FEELS CLUNKY - HOW CAN THIS BE DONE BETTER? <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
          if (*ok_to_send == false) {
            *ok_to_send = true;
            DEBUG("Unblocked");
          }
        }
        
        // set up for the main data loop - rc_state 10
        rc_bitmask = 0x80;
        rc_calc_checksum = 0;
        rc_data_pos = 0;
        
        // check for multi-chunk
        if (rc_sub == 1 && (rc_cmd == 3 || rc_cmd == 1))  
          rc_multi_chunk = true;
        else {
          rc_multi_chunk = false;
          in_message_bad = false;
          in_message->add(rc_cmd);
          in_message->add(rc_sub);
          in_message->add(0);
          in_message->add(0);
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
              in_message->drop();
            }
            else {
              len = in_message->get_len();
              uint_to_bytes(len, &len_h, &len_l);

              in_message->set_at_index(2, len_h);
              in_message->set_at_index(3, len_l);
              in_message->commit();
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
              in_message->add(rc_cmd);
              in_message->add(rc_sub);
              in_message->add(0);
              in_message->add(0);
            }
            else if (rc_this_chunk != rc_last_chunk+1)
              in_message_bad = true;
          }
          else if (rc_multi_chunk && rc_data_pos == 3) 
            rc_chunk_len = b;
          else {  
            in_message->add(b);             
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



// MESSAGE INPUT ROUTINES
// Routine to read messages from the in_message ring buffer and copy to a message C structurer


void SparkMessageIn::set(RingBuffer *messages) {
  in_message = messages;
}

void AppMessageIn::set(RingBuffer *messages) {
  in_message = messages;
}

void MessageIn::read_byte(uint8_t *b)
{
  uint8_t a;
  in_message->get(&a);
  *b = a;
}   

void MessageIn::read_uint(uint8_t *b)
{
  uint8_t a;
  in_message->get(&a);
  if (a == 0xCC)
    in_message->get(&a);
  *b = a;
}
   
void MessageIn::read_string(char *str)
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

void MessageIn::read_prefixed_string(char *str)
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

void MessageIn::read_float(float *f)
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

void MessageIn::read_onoff(bool *b)
{
  uint8_t a;
   
  read_byte(&a);
  if (a == 0xc3)
    *b = true;
  else // 0xc2
    *b = false;
}

// The functions to get the message

bool MessageIn::get_message(unsigned int *cmdsub, SparkMessage *msg, SparkPreset *preset)
{
  uint8_t cmd, sub, len_h, len_l;
  unsigned int len;
  unsigned int cs;
   
  uint8_t junk;
  int i, j;
  uint8_t num;

  if (in_message->is_empty()) return false;
  
//  in_message->dump3();

  read_byte(&cmd);
  read_byte(&sub);
  read_byte(&len_h);
  read_byte(&len_l);
  
  bytes_to_uint(len_h, len_l, &len);
  bytes_to_uint(cmd, sub, &cs);

  *cmdsub = cs;
  switch (cs) {

    // 0x02 series - requests
    // get preset information
    case 0x0201:
      read_byte(&msg->param1);
      read_byte(&msg->param2);
      for (i=0; i < 30; i++) read_byte(&junk); // 30 bytes of 0x00
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
    case 0x022a:
    case 0x032a:
      // the data is a fixed array of four bytes (0x94 00 01 02 03)
      read_byte(&junk);
      read_uint(&msg->param1);
      read_uint(&msg->param2);
      read_uint(&msg->param3);
      read_uint(&msg->param4);
    // get firmware version - this is a request with no payload
    case 0x022f:
      break;
    // change effect parameter
    case 0x0104:
      read_string(msg->str1);
      read_byte(&msg->param1);
      read_float(&msg->val);
      break;
    // change of effect model
    case 0x0306:
    case 0x0106:
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
    case 0x0115:
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
    case 0x0138:
      read_byte(&msg->param1);
      read_byte(&msg->param2);
      break;
    // license key
    case 0x0170:
      for (i = 0; i < 64; i++)
        read_uint(&junk);
      break;
    // response to a request for a full preset
    case 0x0301:
    case 0x0101:
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
    case 0x470:
      read_byte(&junk); //debug
    // acks - no payload to read - no ack sent for an 0x0104
    case 0x0401:
    case 0x0501:
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

void MessageOut::start_message(int cmdsub)
{
  int om_cmd, om_sub;
  
  om_cmd = (cmdsub & 0xff00) >> 8;
  om_sub = cmdsub & 0xff;

  out_message->add(om_cmd);
  out_message->add(om_sub);
  out_message->add(0);      // placeholder for length
  out_message->add(0);      // placeholder for length

  out_msg_chksum = 0;
}

void MessageOut::end_message()
{
  unsigned int len;
  uint8_t len_h, len_l;
  
  len = out_message->get_len();
  uint_to_bytes(len, &len_h, &len_l);
  
  out_message->set_at_index(2, len_h);   
  out_message->set_at_index(3, len_l);
  out_message->commit();
}

void MessageOut::write_byte_no_chksum(byte b)
{
  out_message->add(b);
}

void MessageOut::write_byte(byte b)
{
  out_message->add(b);
  out_msg_chksum += int(b);
}

void MessageOut::write_uint(byte b)
{
  if (b > 127) {
    out_message->add(0xCC);
    out_msg_chksum += int(0xCC);  
  }
  out_message->add(b);
  out_msg_chksum += int(b);
}

void MessageOut::write_uint32(uint32_t w)
{
  int i;
  uint8_t b;
  uint32_t mask;

  mask = 0xFF000000;
  
  out_message->add(0xCE);
  out_msg_chksum += int(0xCE);  

  for (i = 3; i >= 0; i--) {
    b = (w & mask) >> (8*i);
    mask >>= 8;
    write_uint(b);
//    out_message->add(b);
//    out_msg_chksum += int(b);
  }
}


void MessageOut::write_prefixed_string(const char *str)
{
  int len, i;

  len = strnlen(str, STR_LEN);
  write_byte(byte(len));
  write_byte(byte(len + 0xa0));
  for (i=0; i<len; i++)
    write_byte(byte(str[i]));
}

void MessageOut::write_string(const char *str)
{
  int len, i;

  len = strnlen(str, STR_LEN);
  write_byte(byte(len + 0xa0));
  for (i=0; i<len; i++)
    write_byte(byte(str[i]));
}      
  
void MessageOut::write_long_string(const char *str)
{
  int len, i;

  len = strnlen(str, STR_LEN);
  write_byte(byte(0xd9));
  write_byte(byte(len));
  for (i=0; i<len; i++)
    write_byte(byte(str[i]));
}

void MessageOut::write_float (float flt)
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

void MessageOut::write_onoff (bool onoff)
{
  byte b;

  if (onoff)
  // true is 'on'
    b = 0xc3;
  else
    b = 0xc2;
  write_byte(b);
}


void MessageOut::change_effect_parameter (char *pedal, int param, float val)
{
   if (cmd_base == 0x0100) 
     start_message (cmd_base + 0x04);
   else
     start_message (cmd_base + 0x37);
   write_prefixed_string (pedal);
   write_byte (byte(param));
   write_float(val);
   end_message();
}

void MessageOut::change_effect (char *pedal1, char *pedal2)
{
   start_message (cmd_base + 0x06);
   write_prefixed_string (pedal1);
   write_prefixed_string (pedal2);
   end_message();
}



void MessageOut::change_hardware_preset (uint8_t curr_preset, uint8_t preset_num)
{
   // preset_num is 0 to 3

   start_message (cmd_base + 0x38);
   write_byte (curr_preset);
   write_byte (preset_num)  ;     
   end_message();  
}


void MessageOut::turn_effect_onoff (char *pedal, bool onoff)
{
   start_message (cmd_base + 0x15);
   write_prefixed_string (pedal);
   write_onoff (onoff);
   end_message();
}

void MessageOut::get_serial()
{
   start_message (0x0223);
   end_message();  
}

void MessageOut::get_name()
{
   start_message (0x0211);
   end_message();  
}

void MessageOut::get_hardware_preset_number()
{
   start_message (0x0210);
   end_message();  
}

void MessageOut::save_hardware_preset(uint8_t curr_preset, uint8_t preset_num)
{
   start_message (cmd_base + 0x27);
//   start_message (0x0327);
   write_byte (curr_preset);
   write_byte (preset_num);  
   end_message();
}

void MessageOut::send_firmware_version(uint32_t firmware)
{
   start_message (0x032f);
   write_uint32(firmware);  
   end_message();
}

void MessageOut::send_serial_number(char *serial)
{
   start_message (0x0323);
   write_prefixed_string(serial);
   end_message();
}

void MessageOut::send_ack(unsigned int cmdsub) {
   start_message (cmdsub);
   end_message();
}

void MessageOut::send_0x022a_info(byte v1, byte v2, byte v3, byte v4)
{
   start_message (0x032a);
   write_byte(0x94);
   write_uint(v1);
   write_uint(v2);
   write_uint(v3);
   write_uint(v4);      
   end_message();
}

void MessageOut::send_key_ack()
{
   start_message (0x0470);
   write_byte(0x00);
   end_message();
}

void MessageOut::send_preset_number(uint8_t preset_h, uint8_t preset_l)
{
   start_message (0x0310);
   write_byte(preset_h);
   write_byte(preset_l);
   end_message();
}

void MessageOut::get_preset_details(unsigned int preset)
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

void MessageOut::create_preset(SparkPreset *preset)
{
  int i, j, siz;

  start_message (cmd_base + 0x01);

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
  end_message();
}

void SparkMessageOut::set(RingBuffer *messages) {
  out_message = messages;
  cmd_base = 0x0100;
}

void AppMessageOut::set(RingBuffer *messages) {
  out_message = messages;
  cmd_base = 0x0300;
}

void ChunkOut::out_store(uint8_t b)
{
  uint8_t bits;
  
  if (oc_bit_mask == 0x80) {
    oc_bit_mask = 1;
    oc_bit_pos = out_chunk->get_pos();
    out_chunk->add(0);
  }
  
  if (b & 0x80) {
    out_chunk->set_bit_at_index(oc_bit_pos, oc_bit_mask);
    oc_checksum ^= oc_bit_mask;
  }
  out_chunk->add(b & 0x7f);
  oc_checksum ^= (b & 0x7f);

  oc_len++;
  oc_bit_mask *= 2;
}

void ChunkOut::process() {
  int i, j, len;
  int checksum_pos;
  uint8_t b;
  uint8_t len_h, len_l;

  uint8_t num_chunks, this_chunk, this_len;
 
  while (!out_message->is_empty()) {
    out_message->get(&oc_cmd);
    out_message->get(&oc_sub);
    out_message->get(&len_h);
    out_message->get(&len_l);
    bytes_to_uint(len_h, len_l, &oc_len);
    len = oc_len -4;

    if (len > chunk_size) { //this is a multi-chunk message
      num_chunks = int(len / chunk_size) + 1;
      for (this_chunk = 0; this_chunk < num_chunks; this_chunk++) {
       
        // create chunk header
        out_chunk->add(0xf0);
        out_chunk->add(0x01);

        // WATCH OUT THIS CODE IS IN TWO PLACES!!! NEED TO CHANGE BOTH IF CHANGING EITHER
        // Feels clunky to use a 'global' variable but how else to get the sequence number from the input to the output?
        if (oc_cmd == 0x04 || oc_cmd == 0x05 || oc_cmd == 0x03)  // response, so use other sequence counter
          out_chunk->add(*rec_seq);
        else {
          out_chunk->add(oc_seq);
          oc_seq++;
          if (oc_seq == 0x7f) oc_seq = 0x40;  // for sending from amp to app
          if (oc_seq == 0x3f) oc_seq = 0x01;  // for sending from app to amp
        };

        
        checksum_pos = out_chunk->get_pos();
        out_chunk->add(0); // checksum
        
        out_chunk->add(oc_cmd);
        out_chunk->add(oc_sub);

        if (num_chunks == this_chunk+1) 
          this_len = len % chunk_size; 
        else 
          this_len = chunk_size;

        oc_bit_mask = 0x80;
        oc_checksum = 0;
        
        // create chunk sub-header          
        out_store(num_chunks);
        out_store(this_chunk);
        out_store(this_len);
        
        for (i = 0; i < this_len; i++) {
          out_message->get(&b);
          out_store(b);
        }
        out_chunk->set_at_index(checksum_pos, oc_checksum);        
        out_chunk->add(0xf7);
      }
    } 
    else { 
    // create chunk header
      out_chunk->add(0xf0);
      out_chunk->add(0x01);

      // Feels clunky to use a 'global' variable but how else to get the sequence number from the input to the output?
//      if (oc_cmd == 0x04 || oc_cmd == 0x05 || (oc_cmd == 0x03 && (oc_sub != 0x27 && oc_sub != 0x37 && oc_sub != 0x38)))  // response, so use other sequence counter

      // Patch from Paul 10/11/2021
      if (oc_cmd == 0x04 || oc_cmd == 0x05 || (oc_cmd == 0x03 && (oc_sub != 0x27 && oc_sub != 0x37 && oc_sub != 0x38)&& oc_sub != 0x15))  // response, so use other sequence counter
        out_chunk->add(*rec_seq);
      else {
        out_chunk->add(oc_seq);
        oc_seq++;
        if (oc_seq == 0x7f) oc_seq = 0x40;  // for sending from amp to app
        if (oc_seq == 0x3f) oc_seq = 0x01;  // for sending from app to amp
      };

      checksum_pos = out_chunk->get_pos();
      out_chunk->add(0); // checksum

      out_chunk->add(oc_cmd);
      out_chunk->add(oc_sub);

      oc_bit_mask = 0x80;
      oc_checksum = 0;
      for (i = 0; i < len; i++) {
        out_message->get(&b);
        out_store(b);
      }
     out_chunk->set_at_index(checksum_pos, oc_checksum);        
     out_chunk->add(0xf7);
    }
    out_chunk->commit();
//    out_chunk->dump3();
  }
}

void SparkChunkOut::set(RingBuffer *chunks, RingBuffer *messages, uint8_t *seq) {
  out_chunk = chunks;
  out_message = messages;
  chunk_size = 0x80;
  oc_seq = 0x01;
  rec_seq = seq;
}

void AppChunkOut::set(RingBuffer *chunks, RingBuffer *messages, uint8_t *seq) {
  out_chunk = chunks;
  out_message = messages;
  chunk_size = 0x19;
  oc_seq = 0x40;
  rec_seq = seq;
}


void BlockOut::process() {
  int i;
  int len;
  uint8_t b;  
  uint8_t cmd, sub;

  while (!out_chunk->is_empty() && *ok_to_send) {
    ob_pos = 16;

    for (i=0; i < 16; i++) 
      out_block[i]= blk_hdr[i];
      
    b = 0;

    // This condition is complex because sending to the Spark is always a block ending in 0xf7 and chunk aligned
    // but the Spark sends a slew of data until it runs out, and the block contains multiple and partial
    // chunks, and with a different block size - inconsistent
    
    while (    (!to_spark && (ob_pos < block_size && !out_chunk->is_empty())) 
            || ( to_spark && (b != 0xf7))   ) {
      out_chunk->get(&b);
      
      // look for cmd and sub in the stream and set blocking to true if 0x0101 found - multi chunk
      // not sure if that should be here because it means the block send needs to understand the chunks content
      // perhaps it should be between converting msgpack to chunks and put flow control in there
      if (ob_pos == 20) 
        cmd = b;
      if (ob_pos == 21)
        sub = b;

      out_block[ob_pos++] = b;
    }
    
    out_block[6] = ob_pos;
    data_write(out_block, ob_pos);
/*
    for (i=0; i<ob_pos;i++) {
      b=out_block[i];
      if (b<16) Serial.print("0");
      Serial.print(b, HEX);
      Serial.print(" ");
      if (i%16 ==15) Serial.println();
    }
    Serial.println();
*/
    if (to_spark) {
      if (cmd == 0x01 && sub == 0x01) {// need to ensure we are only blocking on send to spark!
          *ok_to_send = false;
          last_sent_time = millis();  
          DEBUG("Blocked");
      }
    }
  }
}

void SparkBlockOut::set(RingBuffer *chunks, uint8_t *hdr, bool *ok) {
  out_chunk = chunks;
  blk_hdr = hdr;
  block_size = 0xad;
  to_spark = true;
  
  ok_to_send = ok;
  *ok_to_send = true;    
  last_sent_time = millis();
}

void SparkBlockOut::data_write(uint8_t *buf, int len){
  sp_write(buf, len);
}

void AppBlockOut::set(RingBuffer *chunks, uint8_t *hdr, bool *ok) {
  out_chunk = chunks;
  blk_hdr = hdr;
  block_size = 0x6a;
  to_spark = false;

  // not sure we need flow control to the app
  ok_to_send = ok;
  *ok_to_send = true;
  last_sent_time = millis();
}

void AppBlockOut::data_write(uint8_t *buf, int len){
  app_write(buf, len);
}
