#include "Banks.h"

tBankConfig bankConfig[NUM_BANKS+1];
tPedalCfg pedalCfg;
String bankConfigFile = "/.bank_config.json";


eMode_t curMode = MODE_PRESETS;
eMode_t oldMode = MODE_PRESETS;
eMode_t returnMode = MODE_PRESETS;
eMode_t mainMode = MODE_PRESETS;
