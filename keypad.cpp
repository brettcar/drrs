#import "WProgram.h"       /* Needed for access to Serial library & define boolean */
#import "keypad.h"
#import "display.h"
#import "config.h"

// Yikes! We need a better way to avoid the use of globals!
extern int currChoice;
extern int currEntry;
extern boolean inNewMsg;
extern MENU_ENTRY entries[10];

volatile boolean keypad_if;

const int keypad_D0 = 5;
const int keypad_D1 = 6;
const int keypad_D2 = 7;
const int keypad_D3 = 8;

const int keypad_irq_port = 3; // Keypad interrupt pin d3
const int keypad_uart_port = 0;

int8_t prevKey = -1;
int8_t currKey = -1;

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
}

void keypad_service(void)
{
 if(keypad_if & 1) 
 {
   uint8_t val;
   keypad_if = false; // Immediately remove interrupt flag, so we
		      // don't miss any keypresses while processing.

   val = digitalRead(keypad_D0) | (digitalRead(keypad_D1) << 1) | (digitalRead(keypad_D2) << 2) | (digitalRead(keypad_D3) << 3);

   // Assumption: data is ready on keypad
   // Read whatever is needed
   // Call display_process if it was a * or #  
   if(inNewMsg & 1)
   {
     prevKey = currKey;
     currKey = val;
     // Remap the 0 (0/OPER) and the 4 (4/GHI) keys to button 0 (1) and button 3 (A)          
     if(prevKey != -1) 
     {
       if(val == 0xD)
         currKey = 0x0;
     else if(val != 0x04)
         currKey = val + 0x01;
         
    //   Serial.print(keymap[prevKey][currKey]);
       prevKey = -1;
       currKey = -1;  
     }
   } 
   else if ((inSetID & 1) && val == 0x0F) 
   {
     config_next_id();
   }
   else if (val == 0x0C)  // Choice 0 - *
   {
     display_process(entries[currEntry], currChoice);   // Yikes, entries is an external global!
   }
   else if (val == 0x0F)  // Choice 1 - D
   { 
      display_next();
   }    
   else // Else, for now, just CLEAR and then print to LCD panel
   {   
     // TODO: Remove the behavior for production code.
     display_clear();
     Serial.print(val, HEX);
   } 
 }
}

