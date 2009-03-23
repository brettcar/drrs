typedef struct {
  uint8_t msgheader;
  uint8_t msglen; 
  uint8_t msgpayload[32];
} PACKET;

typedef struct {
  uint8_t head;
  uint8_t tail;
  PACKET msgs[2];  
}C_QUEUE;

/* 
  header |= (destination & 0x7) << 5;        
  header |= (sender & 0x7) << 2;        
  and for the message type:
  header |= 0, 1, 2, or 3;
*/

extern volatile bool txvr_rx_if;
extern volatile bool txvr_tx_if;

const int txvr_irq_port = 2;
const int txvr_ce_port = 9;
const int txvr_csn_port = 10;
const int txvr_mosi_port = 11;
const int txvr_miso_port = 12;
const int txvr_sck_port = 13;

char read_txvr_reg(char reg);
void txvr_receive_payload (void);
void txvr_set_frequency (int offset);
char txvr_set_prim_rx (bool enable);
char txvr_set_pwr_up (void);
void txvr_set_rf_setup_reg (void);
void txvr_set_rx_addr_p0 (unsigned char *addr);
void txvr_set_rx_pw_p0 (unsigned char length);
void txvr_set_tx_addr (unsigned char *addr);
void txvr_setup (void);
void txvr_setup_ports (void);
char txvr_transmit_payload (const char *data);
void write_txvr_reg(char reg, char val);
