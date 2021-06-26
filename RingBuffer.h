#ifndef RingBuffer_h
#define RingBuffer_h

class RingBuffer
{
  public:
    RingBuffer();
    bool add(uint8_t b);
    bool get(uint8_t *b);
    bool set_at_index(int ind, uint8_t b);
    bool get_at_index(int ind, uint8_t *b);
    bool set_bit_at_index(int ind, uint8_t b);
    int  get_len();
    int  get_pos();
    bool is_empty();
    void commit();
    void drop();
    void clear();
    void dump();
    void dump2();
  private:
    static const int RB_BUFF_MAX = 5000;
    uint8_t rb[RB_BUFF_MAX];
    int st, en, len, t_len;
 };

#endif
