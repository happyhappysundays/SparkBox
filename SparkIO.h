#ifndef SparkIO_h
#define SparkIO_h

#include "RingBuffer.h"
#include "Spark.h"
#include "SparkComms.h"

#define MAX_BT_BUFFER 5000
#define MAX_SER_BUFFER 5000

class SparkIO
{
  public:
    SparkIO(bool passthru);

    // SPARK MESSAGES

    // overall processing

    void sp_process();

    //  receiving data   
    
    void sp_process_in_blocks();
    void sp_process_in_chunks();

    void sp_process_out_chunks();
    void sp_process_out_blocks();

    // processing received messages    
    bool sp_get_message(unsigned int *cmdsub, SparkMessage *msg, SparkPreset *preset);
    // creating messages to send

    void sp_create_preset(SparkPreset *preset);

    void sp_turn_effect_onoff(char *pedal, bool onoff);
    void sp_change_hardware_preset(uint8_t preset_num);
    void sp_change_effect(char *pedal1, char *pedal2);
    void sp_change_effect_parameter(char *pedal, int param, float val);

    void sp_get_serial();
    void sp_get_name();
    void sp_get_hardware_preset_number();
    void sp_get_preset_details(unsigned int preset);
    
    // sending data

    void sp_out_store(uint8_t b);

    // chunk variables (read from bluetooth into a chunk ring buffer)
    // in_chunk.is_empty() is false when there is data to read

    RingBuffer sp_in_chunk;
    int sp_rb_state;
    int sp_rb_len;

    // message variables (read from chunk read buffer into in_message store - a single message
    // in_mesage_ready is true when there is a full message to read

    RingBuffer sp_in_message;
    int sp_rc_state;
    bool sp_in_message_bad;
    
    int sp_rc_seq;
    int sp_rc_cmd;
    int sp_rc_sub;
    int sp_rc_checksum;
    
    int sp_rc_calc_checksum;

    bool sp_rc_multi_chunk;
    int sp_rc_data_pos;
    uint8_t sp_rc_bitmask;
    int sp_rc_bits;

    int sp_rc_total_chunks;
    int sp_rc_this_chunk;
    int sp_rc_chunk_len;
    int sp_rc_last_chunk;

    int sp_rd_pos;
    
    // message variables for sending

    // out_message
    
    RingBuffer sp_out_message;
    int sp_om_cmd;
    int sp_om_sub;
    int sp_out_msg_chksum;

    // out_chunk

    RingBuffer sp_out_chunk;
    uint8_t sp_oc_seq;
    uint8_t sp_oc_cmd;
    uint8_t sp_oc_sub;
    unsigned int sp_oc_len;

    uint8_t sp_oc_bit_mask;
    int sp_oc_bit_pos;
    uint8_t sp_oc_checksum;

    // out_block
    uint8_t sp_out_block[0xad];
    int sp_ob_pos;
    bool sp_ob_ok_to_send;
    uint8_t sp_ob_last_seq_sent;
    unsigned int sp_ob_last_sent_time;

    // passthrough
    uint8_t sp_bt_buf[MAX_BT_BUFFER];
    int sp_bt_pos;
    int sp_bt_len;
    int sp_bt_state;
    bool sp_pass_through;
    
  private:
   
    // routines to read the msgpack data
    void sp_read_string(char *str);
    void sp_read_prefixed_string(char *str);
    void sp_read_onoff(bool *b);
    void sp_read_float(float *f);
    void sp_read_byte(uint8_t *b);

    // routines to write the msgfmt data
    void sp_start_message(int cmdsub);
    void sp_end_message();
    void sp_write_byte(byte b);
    void sp_write_byte_no_chksum(byte b);
    void sp_write_prefixed_string(const char *str);
    void sp_write_long_string(const char *str);
    void sp_write_string(const char *str);
    void sp_write_float(float flt);
    void sp_write_onoff(bool onoff);




    // overall processing

    // APP MESSAGES
  public:
    void app_process();

    //  receiving data   
    
    void app_process_in_blocks();
    void app_process_in_chunks();

    void app_process_out_chunks();
    void app_process_out_blocks();

    // processing received messages    
    bool app_get_message(unsigned int *cmdsub, SparkMessage *msg, SparkPreset *preset);
    // creating messages to send

    void app_create_preset(SparkPreset *preset);

    void app_turn_effect_onoff(char *pedal, bool onoff);
    void app_change_hardware_preset(uint8_t preset_num);
    void app_change_effect(char *pedal1, char *pedal2);
    void app_change_effect_parameter(char *pedal, int param, float val);
    void app_save_hardware_preset(uint8_t preset_num);

  
    // sending data

    void app_out_store(uint8_t b);

    // chunk variables (read from bluetooth into a chunk ring buffer)
    // in_chunk.is_empty() is false when there is data to read

    RingBuffer app_in_chunk;
    int app_rb_state;
    int app_rb_len;

    // message variables (read from chunk read buffer into in_message store - a single message
    // in_mesage_ready is true when there is a full message to read

    RingBuffer app_in_message;
    int app_rc_state;
    bool app_in_message_bad;
    
    int app_rc_seq;
    int app_rc_cmd;
    int app_rc_sub;
    int app_rc_checksum;
    
    int app_rc_calc_checksum;

    bool app_rc_multi_chunk;
    int app_rc_data_pos;
    uint8_t app_rc_bitmask;
    int app_rc_bits;

    int app_rc_total_chunks;
    int app_rc_this_chunk;
    int app_rc_chunk_len;
    int app_rc_last_chunk;

    int app_rd_pos;
    
    // message variables for sending

    // out_message
    
    RingBuffer app_out_message;
    int app_om_cmd;
    int app_om_sub;

    // out_chunk

    RingBuffer app_out_chunk;
    uint8_t app_oc_seq;
    uint8_t app_oc_cmd;
    uint8_t app_oc_sub;
    unsigned int app_oc_len;

    uint8_t app_oc_bit_mask;
    int app_oc_bit_pos;
    uint8_t app_oc_checksum;

    // out_block
    uint8_t app_out_block[0xad];
    int app_ob_pos;
    bool app_ob_ok_to_send;
    uint8_t app_ob_last_seq_sent;
    unsigned int app_ob_last_sent_time;

    // passthrough

    uint8_t app_ser_buf[MAX_SER_BUFFER];
    int app_ser_pos;
    int app_ser_len;
    int app_ser_state;
    bool app_pass_through;


  private:
   
    // routines to read the msgpack data
    void app_read_string(char *str);
    void app_read_prefixed_string(char *str);
    void app_read_onoff(bool *b);
    void app_read_float(float *f);
    void app_read_byte(uint8_t *b);

    // routines to write the msgfmt data
    void app_start_message(int cmdsub);
    void app_end_message();
    void app_write_byte(byte b);
    void app_write_prefixed_string(const char *str);
    void app_write_long_string(const char *str);
    void app_write_string(const char *str);
    void app_write_float(float flt);
    void app_write_onoff(bool onoff);


};

#endif
      
