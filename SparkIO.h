#ifndef SparkIO_h
#define SparkIO_h

#include "RingBuffer.h"
#include "Spark.h"
#include "SparkComms.h"

#define MAX_BT_BUFFER 5000

class SparkIO
{
  public:
    SparkIO(bool passthru);
    ~SparkIO();

    // overall processing

    void process();

    //  receiving data   
    
    void process_in_blocks();
    void process_in_chunks();

    void process_out_chunks();
    void process_out_blocks();

    // processing received messages    
    bool get_message(unsigned int *cmdsub, SparkMessage *msg, SparkPreset *preset);
    // creating messages to send

    void create_preset(SparkPreset *preset);

    void turn_effect_onoff(char *pedal, bool onoff);
    void change_hardware_preset(uint8_t preset_num);
    void change_effect(char *pedal1, char *pedal2);
    void change_effect_parameter(char *pedal, int param, float val);

    void get_serial();
    void get_name();
    void get_hardware_preset_number();
    void get_preset_details(unsigned int preset);
    
    // sending data

    void out_store(uint8_t b);

    // chunk variables (read from bluetooth into a chunk ring buffer)
    // in_chunk.is_empty() is false when there is data to read

    RingBuffer in_chunk;
    int rb_state;
    int rb_len;

    // message variables (read from chunk read buffer into in_message store - a single message
    // in_mesage_ready is true when there is a full message to read

    RingBuffer in_message;
    int rc_state;
    bool in_message_bad;
    
    int rc_seq;
    int rc_cmd;
    int rc_sub;
    int rc_checksum;
    
    int rc_calc_checksum;

    bool rc_multi_chunk;
    int rc_data_pos;
    uint8_t rc_bitmask;
    int rc_bits;

    int rc_total_chunks;
    int rc_this_chunk;
    int rc_chunk_len;
    int rc_last_chunk;

    int rd_pos;
    
    // message variables for sending

    // out_message
    
    RingBuffer out_message;
    int om_cmd;
    int om_sub;
    int out_msg_chksum;

    // out_chunk

    RingBuffer out_chunk;
    uint8_t oc_seq;
    uint8_t oc_cmd;
    uint8_t oc_sub;
    unsigned int oc_len;

    uint8_t oc_bit_mask;
    int oc_bit_pos;
    uint8_t oc_checksum;

    // out_block
    uint8_t out_block[0xad];
    int ob_pos;
    bool ob_ok_to_send;
    uint8_t ob_last_seq_sent;
    unsigned int ob_last_sent_time;

        // passthrough

    uint8_t bt_buf[MAX_BT_BUFFER];
    int bt_pos;
    int bt_len;
    int bt_state;
    bool pass_through;
    
  private:
   
    // routines to read the msgpack data
    void read_string(char *str);
    void read_prefixed_string(char *str);
    void read_onoff(bool *b);
    void read_float(float *f);
    void read_byte(uint8_t *b);

    // routines to write the msgfmt data
    void start_message(int cmdsub);
    void end_message();
    void write_byte(byte b);
    void write_byte_no_chksum(byte b);
    void write_prefixed_string(const char *str);
    void write_long_string(const char *str);
    void write_string(const char *str);
    void write_float(float flt);
    void write_onoff(bool onoff);

};

#endif
      
