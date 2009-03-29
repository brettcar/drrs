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
  register int config_id = EEPROM.read(0);
  config_id = 0; // A
  EEPROM.write(0, config_id);
  Serial.print(g_configid,HEX);
  delay(1000);
}
void config_set_id(void)
{
  
}

// TODO: Fix this. RECEIVER is the recvr, then its the txmitter.
 #define RECEIVER
uint8_t config_get_id(void)
{
  #ifdef RECEIVER
    return 0;
  #else
    return 1;
  #endif
//  return EEPROM.read(0);
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
