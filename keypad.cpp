#import "WProgram.h"       /* Needed for access to Serial library & define boolean */
#import "keypad.h"
#import "display.h"
#import "config.h"
#import "txvr.h"

// Yikes! We need a better way to avoid the use of globals!
extern int currChoice;
extern int currEntry;
extern boolean inNewMsg;
extern MENU_ENTRY entries[10];

volatile boolean keypad_if;

const int keypad_D0 = 5;
const int keypad_D1 = 6;
const int keypad_D2 = 7;
const int keypad_D3 = 19;

const int keypad_irq_port = 3; // Keypad interrupt pin d3
const int keypad_uart_port = 0;

int8_t prevKey = -1;
int8_t currKey = -1;

int keypad_val;

void keypad_isr(void);

void keypad_setup_ports(void)
{
  pinMode(keypad_uart_port, INPUT);
  pinMode(keypad_irq_port, INPUT);
  pinMode(keypad_D0, INPUT);
  pinMode(keypad_D1, INPUT);
  pinMode(keypad_D2, INPUT);
  pinMode(keypad_D3, INPUT);
}

// Attach an interrupt handler for pin d3 
// Call keypad_isr when the interreupt is triggered (LOW)
void keypad_setup(void)
{
  //d5 -> LSB ... d8 -> MSB
  Serial.print("Setup Keypad");
  keypad_if = false;
  attachInterrupt(1, keypad_isr, FALLING);
  Serial.print("Setup Keypad Complete");
  display_clear();  
}

void keypad_isr()
{
  keypad_if = true;  
  keypad_val = digitalRead(keypad_D0) | (digitalRead(keypad_D1) << 1) | (digitalRead(keypad_D2) << 2) | (digitalRead(keypad_D3) << 3);
}

void keypad_service(void)
{
 if(keypad_if & 1) 
 {
   keypad_if = false; // Immediately remove interrupt flag, so we
		      // don't miss any keypresses while processing.

   // Assumption: data is ready on keypad
   // Read whatever is needed
   // Call display_process if it was a * or #  
   if(inNewMsg & 1)
   {
     static boolean initial = true;
     static PACKET * pktStart;
     static uint8_t * pktData;
     if(initial == true)
     {
        pktStart = (PACKET*)malloc(sizeof(PACKET));
        if (pktStart == NULL) {
           // Oh fuck
        }
        memset(pktStart, 0, sizeof(PACKET));
        pktData = (uint8_t*)pktStart->msgpayload;
        initial = false;
     } else {
       prevKey = currKey;
       currKey = keypad_val;
       // Remap the 0 (0/OPER) and the 4 (4/GHI) keys to button 0 (1) and button 3 (A)          
      if(prevKey != -1) 
      {
         switch(keypad_val)
         {
           case 0x0D:
              currKey = 0x0;
              break;
           default:
             if(keypad_val != 0x04)
               currKey = keypad_val + 0x01;
             break;
         }
         *pktData = keymap[prevKey][currKey];
         Serial.print(*pktData);
         pktData++;
         pktStart->msglen++;
         prevKey = -1;
         currKey = -1;  
     }
     else if(keypad_val == 0x03 || keypad_val == 0x07 || keypad_val == 0x0B || keypad_val == 0x0F)  // Submit mode
     {
        switch(keypad_val)
        {
          case 0x03: // A
            packet_set_header(pktStart, config_get_id(), 0, NORMAL);
            break;
          case 0x07: // B
            packet_set_header(pktStart, config_get_id(), 1, NORMAL);
            break;
          case 0x0B: // C
            packet_set_header(pktStart, config_get_id(), 2, NORMAL);
            break;
          case 0x0F:  // D
            packet_set_header(pktStart, config_get_id(), 3, NORMAL);
            break;       
          default:
            break; 
        }
        pktStart->id = ++g_lastid;
        txvr_submit_packet(pktStart);   
        // Message submitted, go back to main menu
        display_back();
        inNewMsg = false;
        initial = true;
    }        
   }
   
   } 
   else if ((inSetID & 1) && keypad_val == 0x0F) 
   {
     config_next_id();
   }
   else if (keypad_val == 0x0C)  // Choice 0 - *
   {
     display_process(entries[currEntry], currChoice);   // Yikes, entries is an external global!
   }
   else if (keypad_val == 0x0F)  // Choice 1 - D
   { 
      display_next();
   }    
   else // Else, for now, just CLEAR and then print to LCD panel
   {   
     // TODO: Remove the behavior for production code.
     display_clear();
     Serial.print(keypad_val, HEX);
   } 
 }
}
