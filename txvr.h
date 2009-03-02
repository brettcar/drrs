extern volatile bool txvr_rx_irq;
extern volatile bool txvr_tx_irq;

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
