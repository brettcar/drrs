#include <EEPROM.h>
#import "WProgram.h"
#import "txvr.h"
#import "config.h"
#import "list.h"

static DList pktList;

extern char spi_transfer (volatile char data);

volatile bool txvr_rx_if = false;
volatile bool txvr_tx_if = false;

const char TXVR_NOP_CMD = 0xFF;  

void txvr_setup_ports (void)
{
  pinMode (txvr_csn_port, OUTPUT);
  pinMode (txvr_sck_port, OUTPUT);
  pinMode (txvr_mosi_port, OUTPUT);
  pinMode (txvr_miso_port, INPUT);
  pinMode (txvr_ce_port, OUTPUT);
  digitalWrite(txvr_ce_port, LOW);
  pinMode (txvr_irq_port, INPUT);
  digitalWrite (txvr_csn_port, HIGH);
}

void txvr_isr()
{
  volatile char value = read_txvr_reg(7);
  // Check if RX_DR bit is set
  if (0b01000000 & value) {
    txvr_rx_if = true;    
  }
  value |= 0b1110000;  
  write_txvr_reg(7, value);
    
}

void txvr_setup (void)
{
  dlist_init(&pktList, free);
  
  txvr_set_pwr_up ();
  delay (100);
  txvr_set_rf_setup_reg ();
  txvr_set_frequency (0);

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
  txvr_set_rx_addr_p0 (addr);
  txvr_set_rx_pw_p0 (5);

  txvr_set_tx_addr (addr);

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
void txvr_set_rx_pw_p0 (unsigned char length)
{
  digitalWrite (txvr_csn_port, LOW);
  spi_transfer (0x31);
//Write to rx_pw_p0 register
  spi_transfer (length);
  digitalWrite (txvr_csn_port, HIGH);
}

//addr must be a pointer to 3 - byte memory, MSByte first
void txvr_set_rx_addr_p0 (unsigned char *addr)
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
void txvr_set_tx_addr (unsigned char *addr)
{
  digitalWrite (txvr_csn_port, LOW);
  spi_transfer (0x30);
  //Write to tx_addr register
  spi_transfer (addr[2]);
  spi_transfer (addr[1]);
  spi_transfer (addr[0]);
  digitalWrite (txvr_csn_port, HIGH);
}

void txvr_set_rf_setup_reg (void)
{
  digitalWrite (txvr_csn_port, LOW);
  spi_transfer (0x26);
  //write to RF_SETUP
  spi_transfer (0b00000001);
  //data to write
  digitalWrite (txvr_csn_port, HIGH);
}

//Carrier is set to 2400 MHz + offset[MHz]
void txvr_set_frequency (int offset)
{
  unsigned char data = (offset & 0b01111111);
  digitalWrite (txvr_csn_port, LOW);
  spi_transfer (0x25);
  spi_transfer (data);
  digitalWrite (txvr_csn_port, HIGH);
}


//enable PRIM_RX
char txvr_set_prim_rx (bool enable)
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

char txvr_set_pwr_up (void)
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

void txvr_receive_payload (void)
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

char txvr_transmit_payload (PACKET * packet)
{
  // Assumption: txvr is in RX mode
  

  digitalWrite(txvr_ce_port, LOW);
  digitalWrite(txvr_csn_port, LOW);
  
  spi_transfer(0xA0);
  spi_transfer(packet->msgheader);
  spi_transfer(packet->id);
  spi_transfer(packet->msglen);
  for(int i = 0; i < 29; i++)
    spi_transfer(packet->msgpayload[i]);
  
  digitalWrite(txvr_csn_port, HIGH);
  txvr_set_prim_rx(false);
  digitalWrite(txvr_ce_port, HIGH);
  delayMicroseconds(200);
  digitalWrite(txvr_ce_port, HIGH);

  // txr instructions    
  // Write FIFO payload (include csn_port toggle)
  // txvr_set_prim_rx (false);
  // digitalWrite (txvr_ce_port, HIGH);
  // Delay
  // digitalWrite (txvr_ce_port, LOW);
  // txvr_set_prim_rx (true); 
  // Delay
  // digitalWrite (txvr_ce_port, HIGH);
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

static inline uint8_t DESTINATION(PACKET * bpkt) {
  return bpkt->msgheader & 0xE0 >> 5;
}

static inline uint8_t SENDER(PACKET * apkt) {
  return apkt->msgheader & 0x1C >> 2;
}

static inline uint8_t TYPE(PACKET * apkt) {
  return apkt->msgheader & 0x02;
}

static inline uint8_t ID(PACKET * apkt) {
  return apkt->id;
}

static inline void packet_set_header(PACKET * pkt, uint8_t sender, uint8_t receiver, uint8_t type) {
  pkt->msgheader = 0;
  pkt->msgheader |= type & 0x3;
  pkt->msgheader |= (sender & 0x7) << 2;
  pkt->msgheader |= (receiver & 0x7) << 5;        
}

void packet_print(PACKET * pkt) {
  Serial.print("[PKT: Dst(");
  Serial.print(DESTINATION(pkt), HEX);
  Serial.print(") Src(");
  delay(100);
  Serial.print(SENDER(pkt), HEX);
  Serial.print(") Type(");
  switch(TYPE(pkt), HEX) {
    case NORMAL:
      Serial.print("NORMAL");
      break;
    case ACK:
      Serial.print("ACK");
      break;
    default:
      Serial.print("UNK");
      break;
  }
  Serial.print(") Id(");
  delay(1000);
  Serial.print(ID(pkt), HEX);
  Serial.print("Msglen(");
  Serial.print(pkt->msglen, HEX);
  Serial.print(") Data: ");
  delay(250);
  /*for (int i = 0; i < 20; i++) {
    Serial.print(pkt->msgpayload[i], HEX);
    delay(50);
   
  }*/
  Serial.print("]");
}


void list_test_send(void)
{
  // Build a proper packet with some information and send it off
  void * data;
  PACKET * pkt = (PACKET*) malloc(sizeof(PACKET));
  uint8_t sender = 1;
  uint8_t receiver = 0;
  uint8_t type = NORMAL;
  uint8_t id = 0;
  uint8_t msglen = 29;
  
  packet_set_header(pkt, sender, receiver, type);
  pkt->id = id;
  pkt->msglen = msglen;
  memset(pkt->msgpayload, 'A', 29);
  
  // Put the packet in the list
  dlist_ins_next(&pktList, dlist_head(&pktList), pkt);
  // Transmit the packet
  queue_transmit();
  Serial.print("success?");
}

#if 0
void list_test_insert(void)
{
  void * data; 
  // Build backet, insert at head
  PACKET * pkt = (PACKET*) malloc(sizeof(PACKET));
  memset(pkt, 1, sizeof(PACKET));
  dlist_ins_next(&pktList, dlist_head(&pktList), pkt);
  
  // Build packet, insert at head
  pkt = (PACKET*) malloc(sizeof(PACKET));
  memset(pkt, 0, sizeof(PACKET));
  dlist_ins_next(&pktList, dlist_head(&pktList), pkt);
  
  // Now get a packet.
  DListElmt * elmt = NULL;
  elmt = dlist_head(&pktList);

  if (elmt == NULL)
   Serial.print("SHIT BROKE");
  else if (dlist_size(&pktList) != 2)
    Serial.print("FUBAR");
  
  dlist_remove(&pktList, elmt, &data);
  free(data);
  if(dlist_size(&pktList) != 1)
    Serial.print("error");
  if (dlist_data(dlist_head(&pktList)) != pkt)
    Serial.print("wtf");
    
  Serial.print("itsallgood");
}
#endif


void queue_transmit(void) 
{
  // Iterate through our list and find every message that is not intended for us
  // These messages will be txed
  if(dlist_size(&pktList) == 0)
    return;
  DListElmt * thisElement;
  thisElement = dlist_head(&pktList);
  do
  {
    PACKET * thisPacket = (PACKET*) dlist_data(thisElement);
    uint8_t dest = DESTINATION(thisPacket);
    if(dest != config_get_id())
    {
       // Transmit
      txvr_transmit_payload(thisPacket);
    }
    thisElement = dlist_next(thisElement);
  }while(thisElement != NULL);      
}

void queue_receive(void) {
  // Loop through queue checking for messages destined for us. If they
  // are for us, turn the LED on. If they are not destined for us,
  // check the entire loop for ACKs for that message. If an ACK is
  // found, then remove the ACK and the message from the queue.
  register bool foundPacket = false;
  if (dlist_size(&pktList) == 0)
    return;
  
  DListElmt * thisElement;
  thisElement = dlist_head(&pktList);
  
   do{
    PACKET * thisPacket = (PACKET*)dlist_data(thisElement);
    uint8_t dest = DESTINATION(thisPacket);
    uint8_t src  = SENDER(thisPacket);
    uint8_t id   = ID(thisPacket);

    if (dest == config_get_id() 
	&& TYPE(thisPacket) == NORMAL)
      {
	foundPacket = true;
	// LED TURN ON
	// Put an ACK into the queue for it.
	PACKET * ack = (PACKET*)malloc(sizeof(PACKET));
	if (ack == NULL)
	  Serial.print("OUT OF MEMORY");

	packet_set_header(ack, config_get_id(), src, ACK);
	ack->id = id;
	ack->msglen = 0;
      }
    else if (TYPE(thisPacket) == NORMAL) {
      // Packet not for us, search for an ACK with the same DEST and
      // ID.
      bool foundAck = false;
      
      DListElmt *subElmt  = dlist_head(&pktList);
      do {
        PACKET * potentialACK = (PACKET*)dlist_data(subElmt);
	if (DESTINATION(potentialACK) == dest
	    && SENDER(potentialACK) == src
	    && ID(potentialACK) == id
	    && TYPE(potentialACK) == ACK)
	    {
	      dlist_remove(&pktList, subElmt, NULL);
	      foundAck = true;
	    }
       subElmt = dlist_next(subElmt);      
      }while(subElmt != NULL);
       
      if (foundAck == true)
	dlist_remove(&pktList, thisElement, NULL);
    } // End else if(TYPE...
    
    thisElement = dlist_next(thisElement);
  }while(thisElement != NULL); // End while

  if (foundPacket == false)
    ; // Turn LED OFF.
}  
