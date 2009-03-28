#include "WProgram.h"
// Config Header File which defines the config struct
typedef struct 
{
  int id;
  double freq;
  int brightness;
} CONFIG;

//extern CONFIG g_config;
extern int currChoice;
extern int currEntry;
extern boolean inSetID;

uint8_t config_get_id(void);
void config_set_id(void);
void config_set_freq(void);
void config_set_bright(void);
void config_next_id(void);
void config_setup(void);
