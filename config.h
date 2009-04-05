#include "WProgram.h"
extern int currChoice;
extern int currEntry;
extern boolean inSetID;

#define EEPROM_CONFIG_ADDR     0
#define EEPROM_FREQ_ADDR       1
#define EEPROM_BRIGHT_ADDR     2

uint8_t config_get_id(void);
void config_set_id_callback(void);
void config_next_id(void);
void config_set_freq_callback();
void config_set_bright_callback();
void config_setup(void);
void config_set_id(uint8_t newId);
void config_set_freq(uint8_t offset);
void config_set_bright(uint8_t level);
