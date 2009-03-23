#include <EEPROM.h>
#import "display.h"
#import "keypad.h"
#import "txvr.h"
#import "config.h"
#import "WProgram.h"
/*
#define BOARD_1   // BOARD_1 = Primary Transmitter
#define BOARD_2   // BOARD_2 = Primarty Receiver
*/

//#define KEYPAD_DEBUG // For keypad debugging
#define UNIT_TEST

//const char *messages[] = { "Hello", "Epic!", "Goal!", "Pasta", "DKCX." };
//const char ack[] = {'A', 'C', 'K', '.', ' '};

volatile boolean keypad_if;
volatile uint8_t g_configid;
extern const int txvr_ce_port;

// Memory Test
#define PACKET_RING_SIZE	3
PACKET packets[PACKET_RING_SIZE];

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
  keypad_setup();
  display_setup();
  config_setup();
  Serial.print ("Setup ack. ");
  delay(500);
  display_mainmenu();
  keypad_if = false;
  test_ram_write();
  
}

void loop(void)
{
  #ifdef UNIT_TEST
    test_ram_read();
  #endif  
  
  #ifdef KEYPAD_DEBUG
  //Do nothing, wait for keypad interrupt    
    keypad_service();
  #endif
  #ifdef BOARD_1
    static int i = 0;
    unsigned long start_time = millis();
    txvr_transmit_payload(messages[i]);
   
    set_txvr_prim_rx(true);
    digitalWrite(txvr_ce_port, HIGH);
    
    Serial.print("TXed ");
    Serial.print(i, HEX);
    while ((!txvr_rx_if) && (millis() - start_time < 5000)) {
     ; // Do nothing while waiting for ACK
    }
    digitalWrite(txvr_ce_port, LOW);
    if (txvr_rx_if) {
      Serial.print("RXed ");   
      txvr_receive_payload();
      i += 1;
      if (i == 5) i = 0;
      txvr_rx_if = false;
    } else {
      Serial.print("TIME OUT ");
    }
  #endif
  #ifdef BOARD_2
    txvr_set_prim_rx(true);
    digitalWrite(txvr_ce_port, HIGH);
    while (!txvr_rx_if) {
     ; // Do nothing while waiting for packet
    }
    digitalWrite(txvr_ce_port, LOW);    
    if (txvr_rx_if) {
      txvr_receive_payload();
      txvr_rx_if = false;
    }

    // Send Ack command!
    delay(1000);
    txvr_transmit_payload(ack);
  #endif
}

char spi_transfer (volatile char data)
{
  SPDR = data;
  //Start the transmission
  while (!(SPSR & (1 << SPIF)))
    //Wait the end of the transmission
    {
    };
  return SPDR;
  //return the received byte
}

#ifdef UNIT_TEST
void test_ram_write()
{  
  for (int i = 0; i < PACKETS_RING_SIZE; i++) {
    packets[i].msgheader = 0xBC;
    packets[i].msglen = 0xAA;
    memset(packets[i].msgpayload, 0xCC, 32);
  }
}

void test_ram_read()
{
  bool failure = false;
  for (int i = 0; i < PACKETS_RING_SIZE; i++) {
    if (packets[i].msgheader != 0xBC)
      failure = true;
    if (packets[i].msglen != 0xAA)
      failure = true;
    for (int j = 0; j < 32; j++)
      if (packets[i].msgpayload[j] != 0xCC)
	failure = true;
  }
  Serial.print("TestRAM ");
  if (failure == true)
    Serial.print("FAILED");
  else
    Serial.print("PASSED");
}
#endif
