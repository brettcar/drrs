#import "display.h"
#import "keypad.h"
#import "txvr.h"

/*
#define BOARD_1   // BOARD_1 = Primary Transmitter
#deinfe BOARD_2   // BOARD_2 = Primarty Receiver
*/

#define KEYPAD_DEBUG // For keypad debugging

const char *messages[] = { "Hello", "Epic!", "Goal!", "Pasta", "DKCX." };
const char ack[] = {'A', 'C', 'K', '.', ' '};

volatile boolean keypad_if;

void setup (void)
{
  txvr_setup_ports();
  keypad_setup_ports();

  SPCR = 0b01010010;
  int clr = SPSR;
  clr = SPDR;
  delay (10);

  delay(5); 
  txvr_setup ();
  delay(100);
  display_setup_lcd ();
  keypad_setup();
  display_setup();
  Serial.print ("Setup complete. ");
  delay(500);
  display_mainmenu();
}

void loop(void)
{
  #ifdef KEYPAD_DEBUG
  // Do nothing, wait for keypad interrupt    
    keypad_service();
  #endif
  #ifdef BOARD_1
    static int i = 0;
    unsigned long start_time = millis();
    transmit_payload(messages[i]);
   
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
    transmit_payload(ack);
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
