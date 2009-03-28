typedef struct {
  uint8_t msgheader;
  uint8_t id;
  uint8_t msglen; 
  uint8_t msgpayload[31];
} PACKET;

// Packet Type Codes
enum { NORMAL = 0, ACK, RESERVED_0, RESERVED_1 };

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
