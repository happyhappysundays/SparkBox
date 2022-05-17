#ifndef _BANKS_H
#define _BANKS_H

#include <Arduino.h>
#include "config.h"
#include "SparkStructures.h"

enum eMode_t {MODE_PRESETS, MODE_EFFECTS, MODE_BANKS, MODE_CONFIG, MODE_TUNER, MODE_BYPASS, MODE_MESSAGE, MODE_LEVEL};

#define JSON_SIZE 96 * (NUM_BANKS+1)

typedef struct {
  char bank_name[STR_LEN+1] = "";
  uint8_t start_chan = 255;
} tBankConfig;

typedef struct {
  uint8_t active_bank = 0;
  uint8_t active_chan = 0;
  eMode_t active_mode = MODE_PRESETS;
} tPedalCfg;

#endif
