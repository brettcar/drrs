#include "WProgram.h"
void
setup ();
void
setup_lcd ();
void
loop ();
void txvr_isr();
void
receive_payload ();
char
transmit_payload (const char *data);
char
spi_transfer (volatile char data);
void write_txvr_reg(char reg, char val);
char read_txvr_reg(char reg);
void
setup_txvr (void);
void
set_txvr_rx_pw_p0 (unsigned char length);
void
set_txvr_rx_addr_p0 (unsigned char *addr);
void
set_txvr_tx_addr (unsigned char *addr);
void
set_txvr_rf_setup_reg (void);
void
set_txvr_frequency (int offset);
char
set_txvr_prim_rx (bool enable);
char
set_txvr_pwr_up (void);
const int txvr_irq_port = 2;
const int txvr_ce_port = 9;
const int txvr_csn_port = 10;
const int txvr_mosi_port = 11;
const int txvr_miso_port = 12;
const int txvr_sck_port = 13;

const char TXVR_NOP_CMD = 0xFF;

volatile bool txvr_rx_irq = false;
volatile bool txvr_tx_irq = false;

#define BOARD_1   (1) //BOARD_1 = Primary Transmitter


const char *messages[] = { "Hello", "Epic!", "Goal!", "Pasta", "DKCX." };
const char ack[] = {'A', 'C', 'K', '.', ' '};

void
setup ()
{
  pinMode (txvr_csn_port, OUTPUT);
  pinMode (txvr_sck_port, OUTPUT);
  pinMode (txvr_mosi_port, OUTPUT);
  pinMode (txvr_miso_port, INPUT);
  pinMode (txvr_ce_port, OUTPUT);
  digitalWrite(txvr_ce_port, LOW);
  pinMode (txvr_irq_port, INPUT);
  digitalWrite (txvr_csn_port, HIGH);

  SPCR = 0b01010010;
  int clr = SPSR;
  clr = SPDR;
  delay (10);

  setup_txvr ();
  setup_lcd ();
  delay(1000); 
  Serial.print ("Setup complete. ");
  delay (2000);
}

void
setup_lcd () 
{
  Serial.begin (9600);
  // Clear LCD
  Serial.print (0xFE, BYTE); 
  Serial.print (0x01, BYTE);
}

void
loop ()
{
  #ifdef BOARD_1
    static int i = 0;
    unsigned long start_time = millis();
    transmit_payload(messages[i]);
   
    set_txvr_prim_rx(true);
    digitalWrite(txvr_ce_port, HIGH);
    
    Serial.print("TXed ");
    Serial.print(i, HEX);
    while ((!txvr_rx_irq) && (millis() - start_time < 5000)) {
     ; // Do nothing while waiting for ACK
    }
    digitalWrite(txvr_ce_port, LOW);
    if (txvr_rx_irq) {
      Serial.print("RXed ");   
      receive_payload();
      i += 1;
      if (i == 5) i = 0;
      txvr_rx_irq = false;
    } else {
      Serial.print("TIME OUT ");
    }
  #else
    set_txvr_prim_rx(true);
    digitalWrite(txvr_ce_port, HIGH);
    while (!txvr_rx_irq) {
     ; // Do nothing while waiting for packet
    }
    digitalWrite(txvr_ce_port, LOW);    
    if (txvr_rx_irq) {
      receive_payload();
      txvr_rx_irq = false;
    }

    // Send Ack command!
    delay(1000);
    transmit_payload(ack);
  #endif
}

void txvr_isr()
{
  volatile char value = read_txvr_reg(7);
  // Check if RX_DR bit is set
  if (0b01000000 & value) {
    txvr_rx_irq = true;    
  }
  value |= 0b1110000;  
  write_txvr_reg(7, value);
    
}

void
receive_payload ()
{
  char payload[6];
  digitalWrite (txvr_csn_port, LOW);
  spi_transfer (0x61);
  payload[0] = spi_transfer (TXVR_NOP_CMD);
  payload[1] = spi_transfer (TXVR_NOP_CMD);
  payload[2] = spi_transfer (TXVR_NOP_CMD);
  payload[3] = spi_transfer (TXVR_NOP_CMD);
  payload[4] = spi_transfer (TXVR_NOP_CMD);
  payload[5] = '\0';
  digitalWrite (txvr_csn_port, HIGH);
  Serial.print(0xFE, BYTE);
  Serial.print(0x01, BYTE);
  delay(150);
  Serial.print (payload);
}

char
transmit_payload (const char *data)
{
  set_txvr_prim_rx (false);
  digitalWrite (txvr_csn_port, LOW);
  spi_transfer (0xA0);
  spi_transfer (data[0]);
  spi_transfer (data[1]);
  spi_transfer (data[2]);
  spi_transfer (data[3]);
  spi_transfer (data[4]);
  digitalWrite (txvr_csn_port, HIGH);
  digitalWrite (txvr_ce_port, HIGH);
  delayMicroseconds(200);
  digitalWrite (txvr_ce_port, LOW);
}

char
spi_transfer (volatile char data)
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



// write an 8-bit reg
void write_txvr_reg(char reg, char val) 
{
  char cmd = 0x20 | (reg & 0b00011111);
  digitalWrite (txvr_csn_port, LOW);
  spi_transfer(cmd);
  spi_transfer(val);
  digitalWrite (txvr_csn_port, HIGH);
}

// Read an 8-bit register, returns 8-bit char
char read_txvr_reg(char reg)
{
  digitalWrite(txvr_csn_port, LOW);
  spi_transfer(reg);
  char value = spi_transfer (TXVR_NOP_CMD);
  digitalWrite(txvr_csn_port, HIGH);
  return value;
}

void
setup_txvr (void)
{
  set_txvr_pwr_up ();
  delay (100);
  set_txvr_rf_setup_reg ();
  set_txvr_frequency (0);

  //Set radio address width in SETUP_AW
  digitalWrite (txvr_csn_port, LOW);
  spi_transfer (0x23);
  spi_transfer (0x01);
  digitalWrite (txvr_csn_port, HIGH);

  // Disable auto retransmit
  // Set radio address width in SETUP_AW
  digitalWrite (txvr_csn_port, LOW);
  spi_transfer (0x24);
  spi_transfer (0x00);
  digitalWrite (txvr_csn_port, HIGH);

  //Disable auto ack
  digitalWrite (txvr_csn_port, LOW);
  spi_transfer (0x21);
  spi_transfer (0x00);
  digitalWrite (txvr_csn_port, HIGH);

  //Set our unique address
  unsigned char addr[] = { 0xDA, 0xBE, 0xEF };
  set_txvr_rx_addr_p0 (addr);
  set_txvr_rx_pw_p0 (5);

  set_txvr_tx_addr (addr);

  //Set pipe 0 as enabled for receive
  digitalWrite (txvr_csn_port, LOW);
  spi_transfer (0x22);
  spi_transfer (0x01);
  digitalWrite (txvr_csn_port, HIGH);

  //Attach interrupt to dataRecIF
  attachInterrupt(0, txvr_isr, LOW);

}

//Set the static payload length for pipe 0
//              length is specified as 1 for 1 byte, 2 for
	      //2 bytes,..., 31 for 31 bytes, up to 32.
void
set_txvr_rx_pw_p0 (unsigned char length)
{
  digitalWrite (txvr_csn_port, LOW);
  spi_transfer (0x31);
//Write to rx_pw_p0 register
  spi_transfer (length);
  digitalWrite (txvr_csn_port, HIGH);
}

//addr must be a pointer to 3 - byte memory, MSByte first
void
set_txvr_rx_addr_p0 (unsigned char *addr)
{
  digitalWrite (txvr_csn_port, LOW);
  spi_transfer (0x2A);
  //Write to rx_addr_p0 register
  spi_transfer (addr[2]);
  spi_transfer (addr[1]);
  spi_transfer (addr[0]);
  digitalWrite (txvr_csn_port, HIGH);
}

//addr must be a pointer to 3 - byte memory, MSByte first
void
set_txvr_tx_addr (unsigned char *addr)
{
  digitalWrite (txvr_csn_port, LOW);
  spi_transfer (0x30);
  //Write to tx_addr register
  spi_transfer (addr[2]);
  spi_transfer (addr[1]);
  spi_transfer (addr[0]);
  digitalWrite (txvr_csn_port, HIGH);
}

void
set_txvr_rf_setup_reg (void)
{
  digitalWrite (txvr_csn_port, LOW);
  spi_transfer (0x26);
  //write to RF_SETUP
  spi_transfer (0b00000001);
  //data to write
  digitalWrite (txvr_csn_port, HIGH);
}

//Carrier is set to 2400 MHz + offset[MHz]
void
set_txvr_frequency (int offset)
{
  unsigned char data = (offset & 0b01111111);
  digitalWrite (txvr_csn_port, LOW);
  spi_transfer (0x25);
  spi_transfer (data);
  digitalWrite (txvr_csn_port, HIGH);
}


//enable PRIM_RX
char
set_txvr_prim_rx (bool enable)
{
  // read CONFIG register
  char value = read_txvr_reg (0x00);

  if (enable) {
    value |= 0b00000001;
  } else {
    value &= 0b11111110;
  }
  
  //set the PRIM_RX bit to 1 if enable is true
  // and to 0 if enable is false
  write_txvr_reg(0x00, value);
}

char
set_txvr_pwr_up (void)
{
  digitalWrite (txvr_csn_port, LOW);
  spi_transfer (0x00);
  //read the CONFIG register
  char value = spi_transfer (TXVR_NOP_CMD);
  //save the value in the CONFIG register
  digitalWrite (txvr_csn_port, HIGH);

  //set the PWR_UP bit and mask off unneeded interrupts
  value |= 0b00110010;

  digitalWrite (txvr_csn_port, LOW);
  spi_transfer (0b00100000);
  spi_transfer (value);
  digitalWrite (txvr_csn_port, HIGH);
}

int main(void)
{
	init();

	setup();
    
	for (;;)
		loop();
        
	return 0;
}

