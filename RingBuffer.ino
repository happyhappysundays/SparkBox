#include "RingBuffer.h" 

//
// RingBuffer class
//

/* Implementation of a ring buffer - with a difference
 * New data is written to a temp area with add_to_buffer() and committed into the buffer once done - commit()
 * Commited data is read from the head of the buffer with get_from_buffer()
 * Data in this area can be read and updated by index - get_at_index() and set_at_index()
 * 
 * If the temp data is not required it can be ignored using drop() rather than commit()
 * 
 *    +----------------------------------++----------------------------------------------------------------------+
 *    |  0 |  1 |  2 |  3 |  4 |  5 |  6 ||  7 |  8 |  9 | 10 | 11 | 12 | 13 || 14 | 15 | 16 | 17 | 18 | 19 | 20 |                                              |
 *    +----------------------------------++----------------------------------------------------------------------+  
 *       ^                             ^     ^                             ^
 *       st ---------- len ------------+     +----------- t_len --------- en
 * 
 *           committed data                      temporary data                                   empty
 */

RingBuffer::RingBuffer() {
  st = 0;
  en = 0;
  len = 0;
  t_len = 0;
}

bool RingBuffer::add(uint8_t b) {
  if (len + t_len < RB_BUFF_MAX) {
    rb[en] = b;
    t_len++;
    en++; 
    if (en >= RB_BUFF_MAX) en = 0; 
    return true;
  }
  else
    return false;
}

bool RingBuffer::get(uint8_t *b) {
  if (len > 0) {
    *b = rb[st];
    len--;
    st++; 
    if (st >= RB_BUFF_MAX) st = 0; 
    return true;
  }
  else
    return false;  
}

// set a value at a location in the temp area
bool RingBuffer::set_at_index(int ind, uint8_t b) {
  if (ind >= 0 && ind < t_len) {
    rb[(st+len+ind) % RB_BUFF_MAX] = b;
    return true;
  }
  else
    return false;
}

// get a value from a location in the temp area
bool RingBuffer::get_at_index(int ind, uint8_t *b) {
  if (ind >= 0 && ind < t_len) {
    *b = rb[(st+len+ind) % RB_BUFF_MAX];
    return true;
  }
  else
    return false;
}

bool RingBuffer::set_bit_at_index(int ind, uint8_t b) {
  if (ind >= 0 && ind < t_len) {
    rb[(st+len+ind) % RB_BUFF_MAX] |= b;    
    return true;
  }
  else
    return false; 
}

int RingBuffer::get_len() { // total temp len
  return t_len;
}

int RingBuffer::get_pos() { // current position
  return t_len;
}

void RingBuffer::commit() {
  len += t_len;
  t_len = 0;
}

void RingBuffer::drop() {
  en = st + len;
  t_len = 0;
}

void RingBuffer::clear() {
  en = st;
  len = 0;
}

bool RingBuffer::is_empty() {
  return (len == 0);
}

void RingBuffer::dump() {
  int i;

  for (i=0; i<len; i++) {
    Serial.print("S ");
    Serial.print(st+i);
    Serial.print(" ");
    Serial.print((st+i) % RB_BUFF_MAX);
    Serial.print(" ");    
    Serial.println(rb[(st+i) % RB_BUFF_MAX], HEX);
  };
  for (i=0; i<t_len; i++) {
    Serial.print("T ");
    Serial.print(st+len+i);
    Serial.print(" ");
    Serial.print((st+len+i) % RB_BUFF_MAX);
    Serial.print(" ");    
    Serial.println(rb[(st+len+i) % RB_BUFF_MAX], HEX);
  };
}

void RingBuffer::dump2() {
  int i;
  uint8_t v;

//  Serial.println();
  for (i=0; i<len; i++) {
    v=rb[(st+i) % RB_BUFF_MAX];
    if (v < 16) Serial.print("0");
    Serial.print(v, HEX);
    Serial.print(" ");
  };
  for (i=0; i<t_len; i++) {
    v=rb[(st+len+i) % RB_BUFF_MAX];
    if (v < 16) Serial.print("0");
    Serial.print(v, HEX);
    Serial.print(" ");
  };
  Serial.println();
}

void RingBuffer::dump3() {
  int i;
  uint8_t v;

  Serial.print("               ");
  for (i=0; i<len; i++) {
    v=rb[(st+i) % RB_BUFF_MAX];
    if (v < 16) Serial.print("0");
    Serial.print(v, HEX);
    Serial.print(" ");
    if (i % 16 == 15) {
      Serial.println();
      Serial.print("               ");
    };
  };
  Serial.println();
}
