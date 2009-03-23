#include <EEPROM.h>
#import "WProgram.h"
#import "config.h"
#import "display.h"

extern volatile uint8_t g_configid;
//CONFIG g_config;
boolean inSetID;
char id_list[] = {'A','B','C','D'};

void config_setup(void)
{
  display_clear();
  delay(50);
  Serial.print("hello world");
  g_configid = 2;
  Serial.print(g_configid,HEX);
  delay(1000);
}
void config_set_id(void)
{
  register int x = EEPROM.read(0);
  inSetID = true;
  display_clear(); 
  delay(100);
  Serial.print("CURRENT ID: ");
  delay(50);
  Serial.print(++x, HEX);
  delay(50);
  Serial.print("   ");
  delay(50);
  Serial.print("NEXT for new ID");
  EEPROM.write(0, x);
}

void config_next_id(void)
{
   //g_configid = (g_configid + 1) % 4;
   config_set_id();
}

void config_set_freq(void)
{
   // Todo 
}

void config_set_bright(void)
{
   // Todo 
}