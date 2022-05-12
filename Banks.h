#ifndef _BANKS_H
#define _BANKS_H

#include <Arduino.h>
#include "config.h"

#define JSON_SIZE 96 * (NUM_BANKS+1)

typedef struct {
  char bank_name[20] = "";
  uint8_t active_chan = 255;
} tBankConfig;


#endif
