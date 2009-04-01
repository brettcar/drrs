#include <EEPROM.h>
#import "WProgram.h"
#import "config.h"
#import "display.h"

//CONFIG g_config;
boolean inSetID;
char id_list[] = {'A','B','C','D'};


void config_setup(void)
{
  display_clear();
  delay(50);
  Serial.print("Config Setup Complete");
}

void config_set_id(void)
{
  // TODO
}

uint8_t config_get_id(void)
{
  return EEPROM.read(0);
}

// Return the ASCII/char version of the id
char config_get_text_id(void) {
  return id_list[EEPROM.read(0) & 0x3];
}

void config_next_id(void)
{
  // TODO
  config_set_id();
}

void config_set_freq(void)
{
  // TODO
}

void config_set_bright(void)
{
  // TODO
}
