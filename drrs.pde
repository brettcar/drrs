#include <EEPROM.h>
#import "display.h"
#import "keypad.h"
#import "txvr.h"
#import "config.h"
#import "WProgram.h"

void setup (void)
{
  txvr_setup_ports();
  keypad_setup_ports();
    
  /* Begin Configure SPI */
  SPCR = 0b01010010;
  int clr = SPSR;
  clr = SPDR;
  delay (10);
  /* End Configure SPI */

  delay(5); 

  txvr_setup ();

  delay(100);

  display_setup_lcd ();
  display_setup();

  keypad_setup();

  config_setup();
  //Serial.print ("Setup complete. ");
  delay(1000);

  display_clear();
  display_mainmenu();
  
  //  if (config_get_id() == 1)
  //  list_test_send();
}

void loop(void)
{
  
  process_ack_queue();
  queue_transmit();
  //  txvr_list_print();
  keypad_service();
  display_update();
  delay(random(100, 300));  
}
