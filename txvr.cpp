#include <EEPROM.h>
#import "WProgram.h"
#import "txvr.h"
#import "config.h"
#import "display.h"

uint8_t g_lastid;
static DList pktList;  /* List used to keep track of normal packets we
			  need to transmit */
DList inboxList;
		
static DList ackList; /* List used to queue ACKs for trannsmission. */

const char TXVR_NOP_CMD = 0xFF;  

volatile bool txvr_rx_if = false;
volatile bool txvr_tx_if = false;

const int led_red = 18;
const int led_green = 17;

char spi_transfer (volatile char data)
{
  SPDR = data;
  // Start the transmission
  while (!(SPSR & (1 << SPIF)));     // Wait the end of the transmission
  return SPDR;   // Return the received byte
}

void txvr_list_print(void) {
  // TODO: Remove this function.
  Serial.print("--> ");
  Serial.print(dlist_size(&ackList), HEX);
  Serial.print(" ");
  Serial.print(dlist_size(&inboxList), HEX);
  Serial.print(" ");
  Serial.println(dlist_size(&pktList), HEX);
}

void txvr_setup_ports (void)
{
  pinMode (txvr_csn_port, OUTPUT);
  digitalWrite (txvr_csn_port, HIGH);
  pinMode (txvr_sck_port, OUTPUT);
  pinMode (txvr_mosi_port, OUTPUT);
  pinMode (txvr_miso_port, INPUT);
  pinMode (txvr_ce_port, OUTPUT);
  digitalWrite(txvr_ce_port, LOW);
  pinMode (txvr_irq_port, INPUT);
}

void txvr_isr()
{
  //Serial.print("in ISR");
  volatile char value = read_txvr_reg(7);
  // Check if RX_DR bit is set
  if (0b01000000 & value) {
    txvr_rx_if = true;    
  }
  value |= 0b1110000;  
  write_txvr_reg(7, value);
  while(!(txvr_receive_payload() & 0x0E)); /* Receive payloads until
					      the FIFO is empty. */
}

void txvr_setup (void)
{
  g_lastid = 0;
  
  pinMode(led_red, OUTPUT);
  pinMode(led_green, OUTPUT);
  
  digitalWrite(led_red, HIGH);
  dlist_init(&pktList, free);
  dlist_init(&inboxList, free);  
  dlist_init(&ackList, free);
  
  txvr_set_pwr_up ();
  delay (100);
  
  // Write the RF_SETUP reg to use lowest power and lowest data rate.
  write_txvr_reg(0x06, 0x01);

  // Set frequency to 2400 MHz
  txvr_set_frequency (0);

  // Set radio address width in SETUP_AW
  write_txvr_reg(0x03, 0x01);

  // Disable auto retransmit
  write_txvr_reg(0x04, 0x00);

  // Disable auto ack
  write_txvr_reg(0x01, 0x00);

  // All radios share Shockburst address of addr for RX
  const unsigned char addr[] = { 0xDA, 0xBE, 0xEF };
  txvr_set_rx_addr_p0 (addr);

  // Set the FIFO pipe width for pipe 0.
  txvr_set_rx_pw_p0 (32);

  // And choose to TX as the shared address as well.
  txvr_set_tx_addr (addr);

  // Set pipe 0 as enabled for receive
  write_txvr_reg(0x02, 0x01);

  // Attach interrupt to monitor for received packets
  attachInterrupt(0, txvr_isr, LOW);
  
  // Enable RX mode
  txvr_set_prim_rx(true); 
  digitalWrite(txvr_ce_port, HIGH); 
}

// Set the static payload length for pipe 0 length is specified as 1
// for 1 byte, 2 for 2 bytes,..., 31 for 31 bytes, up to 32.
void txvr_set_rx_pw_p0 (unsigned char length)
{
  write_txvr_reg(0x11, length);
}

// Addr must be a pointer to 3 - byte memory, MSByte first
void txvr_set_rx_addr_p0 (const unsigned char *addr)
{
  digitalWrite (txvr_csn_port, LOW);
  spi_transfer (0x2A);
  // Write to rx_addr_p0 register
  spi_transfer (addr[2]);
  spi_transfer (addr[1]);
  spi_transfer (addr[0]);
  digitalWrite (txvr_csn_port, HIGH);
}

// Addr must be a pointer to 3 - byte memory, MSByte first
void txvr_set_tx_addr (const unsigned char *addr)
{
  digitalWrite (txvr_csn_port, LOW);
  spi_transfer (0x30);
  // Write to tx_addr register
  spi_transfer (addr[2]);
  spi_transfer (addr[1]);
  spi_transfer (addr[0]);
  digitalWrite (txvr_csn_port, HIGH);
}

// Carrier is set to 2400 MHz + offset[MHz]
void txvr_set_frequency (uint8_t offset)
{
  unsigned char data = (offset & 0b01111111);
  digitalWrite (txvr_csn_port, LOW);
  spi_transfer (0x25);
  spi_transfer (data);
  digitalWrite (txvr_csn_port, HIGH);
}


// Enable or disable PRIM_RX flag in the CONFI register
char txvr_set_prim_rx (bool enable)
{
  // Read CONFIG register
  char value = read_txvr_reg (0x00);

  /* Set the PRIM_RX bit to 1 if enable is true and to 0 if enable is
     false */
  if (enable == true) {
    value |= 0b00000001;
  } else {
    value &= 0b11111110;
  }
  
  write_txvr_reg(0x00, value);
}

char txvr_set_pwr_up (void)
{
  // Read the CONFIG register
  char value = read_txvr_reg(0x00);

  // Set the PWR_UP bit and mask off unneeded interrupts
  value |= 0b00110010;

  // Write back to CONFIG  
  write_txvr_reg(0x00, value);
}

static inline uint8_t DESTINATION(const PACKET * bpkt) {
  uint8_t tmp = bpkt->msgheader & 0xE0;
  tmp >>= 5;
  return tmp;
}

static inline uint8_t SENDER(const PACKET * apkt) {
  uint8_t tmp = apkt->msgheader & 0x1C;
  tmp >>= 2;
  return tmp;
}

static inline uint8_t TYPE(const PACKET * apkt) {
  return apkt->msgheader & 0x03;
}

static inline uint8_t ID(const PACKET * apkt) {
  return apkt->id;
}

void packet_set_header(PACKET * pkt, uint8_t sender, uint8_t receiver, uint8_t type) {
  pkt->msgheader = 0;
  pkt->msgheader |= type & 0x3;
  pkt->msgheader |= (sender & 0x7) << 2;
  pkt->msgheader |= (receiver & 0x7) << 5;        
}

/* Assumption: newPkt is an ack.  Find if there's a packet in our
 * pktList that matches this ACK (if there is, delete it).  If the ack
 * packet is for us, we're done.  If the ack packet is not for us,
 * transmit it (once).
 */
static void txvr_handle_ack(PACKET * newPkt)
{
    DListElmt * thisElement;
    thisElement = dlist_head(&pktList);
    do
    {
      PACKET * thisPacket = (PACKET*) dlist_data(thisElement);
      // Find a packet in our list, that we sent to the person who just sent us this ack packet
      // Toss this packet out of our list 
      if(DESTINATION(thisPacket) == SENDER(newPkt)
        && SENDER(thisPacket) == DESTINATION(newPkt) 
        && ID(thisPacket) == ID(newPkt))
      {
        void * data;   
	// No longer need to retransmit thisPacket; so remove it from
	// the list
        dlist_remove(&pktList, thisElement, &data);
        free(data);
        // Assumption: only one of thisPacket packets exists
        break;
      }
      thisElement = dlist_next(thisElement);
    } while(thisElement != NULL);      
    // If this ack packet is NOT for us put it in the ackList
    if(DESTINATION(newPkt) != config_get_id())
    {
      dlist_ins_next(&ackList, dlist_head(&ackList), newPkt);     
    } else {
      free(newPkt); 
    }  
}


void txvr_submit_packet(PACKET * pkt)
{
  dlist_ins_next(&pktList, dlist_head(&pktList), pkt);  
}

// TODO: remove for production
void txvr_add_inbox(PACKET *pkt)
{
  dlist_ins_next(&inboxList, dlist_head(&inboxList), pkt);
}

uint8_t txvr_receive_payload (void)
{
  uint8_t reg = read_txvr_reg(0x07); // STATUS register
  if (reg & 0x0E) {
    // FIFO is empty but we were told to receive?
    return reg;
  }
  
  PACKET * newPkt = (PACKET*) malloc(32);
  if (newPkt == NULL) {
    Serial.print("Out of Memory");
    return reg;
  }
  
  uint8_t * ptr = (uint8_t*) newPkt;
  
  // Get packet bytes over SPI interface
  digitalWrite(txvr_csn_port, LOW);
  spi_transfer(0x61); // R_RX_PAYLOAD command
  for (register int i = 0; i < 32; i++) {
    *ptr = spi_transfer(TXVR_NOP_CMD);
    ptr++;  
  }
  digitalWrite(txvr_csn_port, HIGH);
  
  // If this is a packet sent to US by US... Toss it out!
  // Garbage data packet, toss out.  
 if(SENDER(newPkt) == config_get_id() || DESTINATION(newPkt) > 3 || TYPE(newPkt == RESERVED_0)
 {
   free(newPkt);
 } 
  else if(TYPE(newPkt) == ACK) {
    // Check if this received packet is an ack intended for us. 
    // If it is, then deal with it and do not add it to the linked list
    //Serial.print("Call-handle-ack ");
    txvr_handle_ack(newPkt);
  }
  else if(TYPE(newPkt) != ACK && DESTINATION(newPkt) == config_get_id()) {
    // Else if this is a normal packet intended for us, put it in the inboxList
    // and send out an ack msg.
    boolean packet_duped = false;
    // Check to see if we already have this message in our inbox. If we do, do not put it there, just re-send ACK.
    DListElmt * thisElement;
    thisElement = dlist_head(&inboxList);    
    
    if (thisElement != NULL) {
      do
      {
        PACKET * thisPacket = (PACKET*) dlist_data(thisElement);
        //TODO: fix this logic (add msgLength to condition for duped packet)
        if(ID(thisPacket) == ID(newPkt)) {
          packet_duped = true;
          break;
        } 
        thisElement = dlist_next(thisElement);
      } while(thisElement != NULL);      
    }    
    // We found a message intended for us.
    // Send out an ack. 
    PACKET  * ack = (PACKET*)malloc(sizeof(PACKET));
    if (ack == NULL) {
      //Serial.print("OOMem"); 
    } else {
      packet_set_header(ack, config_get_id(), SENDER(newPkt), ACK);
      ack->id = ID(newPkt);
      ack->msglen = 0;      
      dlist_ins_next(&ackList, dlist_head(&ackList), ack);
      //Serial.print("QUEUED ACK");
    }
    
    // If this isn't a duplicate packet, then put it inbox list.
    if(packet_duped == false) {
      //Serial.print("Put-in-inbox ");
      dlist_ins_prev(&inboxList, dlist_head(&inboxList), newPkt);
      //packet_print(newPkt);
      digitalWrite(led_green, HIGH);
    } else {
      // TODO
      // free(newPkt);
    }
  } else {
    // for any other kind of packet, ACK or NORMAL, put it in the list
    //Serial.print("Other-pkt ");
    //packet_print(newPkt);
    
    boolean packet_duped = false;
    // Check to see if we already have this message in our inbox. If
    // we do, do not put it there, just re-send ACK.
    DListElmt * thisElement;
    thisElement = dlist_head(&pktList);    
    
    if (thisElement != NULL) {
      do {
	PACKET * thisPacket = (PACKET*) dlist_data(thisElement);
	if(ID(thisPacket) == ID(newPkt)) {
          packet_duped = true;
          break;
        } 
        thisElement = dlist_next(thisElement);
      } while(thisElement != NULL);      
    }
    
    if(packet_duped == false) {
      dlist_ins_next(&pktList, dlist_head(&pktList), newPkt);  
      //Serial.print("put in txList");
    } else {
      // TODO
      // free(newPkt);
    }
  }
  return reg;
}

char txvr_transmit_payload (const PACKET * packet)
{
  // Assumption: txvr is in RX mode
  digitalWrite(txvr_ce_port, LOW);
  digitalWrite(txvr_csn_port, LOW);  
  spi_transfer(0xA0);
  spi_transfer(packet->msgheader);
  spi_transfer(packet->id);
  spi_transfer(packet->msglen);
  for(int i = 0; i < 29; i++) {
    spi_transfer(packet->msgpayload[i]);
  }  
  digitalWrite(txvr_csn_port, HIGH);
  txvr_set_prim_rx(false);
  digitalWrite(txvr_ce_port, HIGH);
  delay(100);
  digitalWrite(txvr_ce_port, LOW);
  txvr_set_prim_rx(true);
  digitalWrite(txvr_ce_port, HIGH);
  delay(10);
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


void packet_print(const PACKET * pkt) {
  Serial.print("[PKT: Dst(");
  Serial.print(DESTINATION(pkt), HEX);
  Serial.print(") Src(");
  //  delay(1000);
  Serial.print(SENDER(pkt), HEX);
  Serial.print(") Type(");
  switch(TYPE(pkt)) {
    case NORMAL:
      Serial.print("N");
      break;
    case ACK:
      Serial.print("A");
      break;
    case RESERVED_0:
      Serial.print("R0");
      break;
    case RESERVED_1:
      Serial.print("R1");
      break;
  }
  Serial.print(") Id(");
  //  delay(1000);
  Serial.print(ID(pkt), HEX);
  Serial.print(") Msglen(");
  Serial.print(pkt->msglen, HEX);
  Serial.print(") Data: ");
  for(int i = 0; i < pkt->msglen; i++)
    Serial.print(pkt->msgpayload[i]);
    
  Serial.print(" ");
  //  delay(1000);
}

void list_test_send(void)
{
  // Build a proper packet with some information and send it off
  void * data;
  PACKET * pkt = (PACKET*) malloc(sizeof(PACKET));
  uint8_t sender = 1;
  uint8_t receiver = 0;
  uint8_t type = NORMAL;
  
  packet_set_header(pkt, sender, receiver, type);
  pkt->id = 0;
  pkt->msglen = 29;
  memset(pkt->msgpayload, 'A', 29);
  
  // Put the packet in the list
  dlist_ins_next(&pktList, dlist_head(&pktList), pkt);
}

#if 0
void list_test_insert(void)
{
  void * data; 
  // Build packet, insert at head
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

// Retruns true if the packet has timed out
static boolean txvr_handle_tx_packet(PACKET * packet)
{
  uint8_t sender = SENDER(packet);
  uint8_t dest = DESTINATION(packet);
  switch(TYPE(packet))
  {
    // If this is normal packet, transmit it and change its type to
    // RESERVED_0
    case NORMAL:
      txvr_transmit_payload(packet);
      packet_set_header(packet, sender, dest, RESERVED_0);      
      break;
    case RESERVED_0:
      packet_set_header(packet, sender, dest, NORMAL);
      txvr_transmit_payload(packet);
      packet_set_header(packet, sender, dest, RESERVED_1);      
      break;
    case RESERVED_1:
      txvr_transmit_payload(packet);
      return true;
      // Undefined case
    default:
      return true;
  }
  return false;
}

void queue_transmit(void) 
{
  // Iterate through our list and find every message that is not intended for us
  // These messages will be txed
  if(dlist_size(&pktList) == 0)
    return;
  
  DListElmt * thisElement;
  thisElement = dlist_head(&pktList);
  
  do {
    PACKET * thisPacket = (PACKET*) dlist_data(thisElement);
    if(txvr_handle_tx_packet(thisPacket) == true) {
      // if handle_tx_packets is true, delete this element
      void * data;
      DListElmt * tmp = thisElement;
      thisElement = dlist_next(thisElement);
      dlist_remove(&pktList, tmp, &data);
      free(data);
    } else {
      // else keep iterating like normal
      thisElement = dlist_next(thisElement);
    }
  } while(thisElement != NULL);      
}

void process_ack_queue(void) {
  if (dlist_size(&ackList) == 0)
    return;
  DListElmt * thisAck = dlist_head(&ackList);
  do {
    txvr_transmit_payload((PACKET*)dlist_data(thisAck));
    //Serial.print("SENT-AN-ACK ");
    thisAck = dlist_next(thisAck);
  } while (thisAck != NULL);

  // Remove all elements from the list
  dlist_destroy(&ackList);
  dlist_init(&ackList, free);
}
