#ifndef SparkIO_h
#define SparkIO_h

#include "RingBuffer.h"
#include "Spark.h"
#include "SparkComms.h"

#define MAX_IO_BUFFER 5000

// BLOCK INPUT CLASS
class BlockIn
{
  public:
    BlockIn() {};
    void process();
    virtual bool data_available();
    virtual uint8_t data_read();
    virtual void data_write(uint8_t *buf, int len);

    // processing received block
    uint8_t *blk_hdr;
    RingBuffer *rb;
    int rb_state = 0;
    int rb_len = 0;
    // passthru
    uint8_t io_buf[MAX_IO_BUFFER];
    int io_pos = 0;
    int io_len = -1;
    int io_state = 0;
    bool pass_through = true;
};

class SparkBlockIn: public BlockIn
{
  public:
    SparkBlockIn() {};
    bool data_available();
    uint8_t data_read();
    void data_write(uint8_t *buf, int len);
    void set(bool pass, RingBuffer *ring_buffer, uint8_t *hdr);
};

class AppBlockIn: public BlockIn
{
  public:
    AppBlockIn() {};
    bool data_available();
    uint8_t data_read();
    void data_write(uint8_t *buf, int len);
    void set(bool pass, RingBuffer *ring_buffer, uint8_t *hdr);
};

// CHUNK INPUT CLASS
class ChunkIn
{
  public:
    ChunkIn() {};
    void process();
    // processing received block
    RingBuffer *in_chunk;
    RingBuffer *in_message;
    bool *ok_to_send;

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

    uint8_t *rec_seq;    // last received sequence from app or amp
};

class SparkChunkIn: public ChunkIn
{
  public:
    SparkChunkIn() {};
    void set(RingBuffer *chunks, RingBuffer *messages, bool *ok, uint8_t *seq);
};

class AppChunkIn: public ChunkIn
{
  public:
    AppChunkIn() {};
    void set(RingBuffer *chunks, RingBuffer *messages, bool *ok, uint8_t *seq);
};

// MESSAGE INPUT CLASS
class MessageIn
{
  public:
    MessageIn() {};
    bool get_message(unsigned int *cmdsub, SparkMessage *msg, SparkPreset *preset);
    RingBuffer *in_message;

    void read_string(char *str);
    void read_prefixed_string(char *str);
    void read_onoff(bool *b);
    void read_float(float *f);
    void read_uint(uint8_t *b);
    void read_byte(uint8_t *b);
};

class SparkMessageIn: public MessageIn
{
  public:
    SparkMessageIn() {};
    void set(RingBuffer *messages);
};

class AppMessageIn: public MessageIn
{
  public:
    AppMessageIn() {};
    void set(RingBuffer *messages);
};

// MESSAGE OUTPUT CLASS
class MessageOut
{
  public:
    MessageOut() {};
    
    // creating messages to send
    void start_message(int cmdsub);
    void end_message();
    void write_byte(byte b);
    void write_byte_no_chksum(byte b);
    
    void write_uint(byte b);
    void write_prefixed_string(const char *str);
    void write_long_string(const char *str);
    void write_string(const char *str);
    void write_float(float flt);
    void write_onoff(bool onoff);
    void write_uint32(uint32_t w);

    void create_preset(SparkPreset *preset);
    void turn_effect_onoff(char *pedal, bool onoff);
    void change_hardware_preset(uint8_t curr_preset, uint8_t preset_num);
    void change_effect(char *pedal1, char *pedal2);
    void change_effect_parameter(char *pedal, int param, float val);
    void get_serial();
    void get_name();
    void get_hardware_preset_number();
    void get_preset_details(unsigned int preset);
    void save_hardware_preset(uint8_t curr_preset, uint8_t preset_num);
    void send_firmware_version(uint32_t firmware);
    void send_0x022a_info(byte v1, byte v2, byte v3, byte v4);  
    void send_preset_number(uint8_t preset_h, uint8_t preset_l);
    void send_key_ack();
    void send_serial_number(char *serial);
    void send_ack(unsigned int cmdsub);

    RingBuffer *out_message;
    int cmd_base;
    int out_msg_chksum;
};

class SparkMessageOut: public MessageOut
{
  public:
    SparkMessageOut() {};
    void set(RingBuffer *messages);
};

class AppMessageOut: public MessageOut
{
  public:
    AppMessageOut() {};
    void set(RingBuffer *messages);
};

// CHUNK INPUT CLASS
class ChunkOut
{
  public:
    ChunkOut() {};
    void process();

    void out_store(uint8_t b);
    // processing received block
    RingBuffer *out_chunk;
    RingBuffer *out_message;
    int chunk_size;

    uint8_t *rec_seq;
    
    uint8_t oc_cmd;
    uint8_t oc_sub;
    unsigned int oc_len;
    uint8_t oc_seq;

    uint8_t oc_bit_mask;
    int oc_bit_pos;
    uint8_t oc_checksum;
};

class SparkChunkOut: public ChunkOut
{
  public:
    SparkChunkOut() {};
    void set(RingBuffer *chunks, RingBuffer *messages, uint8_t *seq);
};

class AppChunkOut: public ChunkOut
{
  public:
    AppChunkOut() {};
    void set(RingBuffer *chunks, RingBuffer *messages, uint8_t *seq);
};

// CHUNK INPUT CLASS
class BlockOut
{
  public:
    BlockOut() {};
    void process();
    virtual void data_write(uint8_t *buf, int len);
    
    RingBuffer *out_chunk;
    bool *ok_to_send;
    unsigned int last_sent_time;
    bool to_spark;
        
    int block_size;
    uint8_t *blk_hdr;
    
    uint8_t out_block[0xad];
    int ob_pos;
};

class SparkBlockOut: public BlockOut
{
  public:
    SparkBlockOut() {};
    void set(RingBuffer *chunks, uint8_t *hdr, bool *ok);
    void data_write(uint8_t *buf, int len);
};

class AppBlockOut: public BlockOut
{
  public:
    AppBlockOut() {};
    void set(RingBuffer *chunks, uint8_t *hdr, bool *ok);
    void data_write(uint8_t *buf, int len);
};

/////////////


    void spark_start(bool passthru);

    void spark_process();

    // processing received messages    
//    bool spark_get_message(unsigned int *cmdsub, SparkMessage *msg, SparkPreset *preset);

    // sending data

    SparkBlockIn sp_bin;
    RingBuffer sp_in_chunk;
    SparkChunkIn sp_cin;
    RingBuffer sp_in_message;
    SparkMessageIn spark_msg_in;

    SparkMessageOut spark_msg_out;
    RingBuffer sp_out_message;
    SparkChunkOut sp_cout;
    RingBuffer sp_out_chunk;
    SparkBlockOut sp_bout;

    bool sp_ok_to_send;          
    bool app_ok_to_send;

    uint8_t sp_rec_seq;
    uint8_t app_rec_seq;
    
    void app_process();

    AppBlockIn app_bin;
    RingBuffer app_in_chunk;
    AppChunkIn app_cin;
    RingBuffer app_in_message;
    AppMessageIn app_msg_in;

    AppMessageOut app_msg_out;
    RingBuffer app_out_message;
    AppChunkOut app_cout;
    RingBuffer app_out_chunk;
    AppBlockOut app_bout;


#endif
      
