#import "WProgram.h"       /* Needed for access to Serial library */
#import "display.h"

// Yikes! We need a better way to avoid the use of globals!
extern int currChoice;
extern int currEntry;
extern boolean inNewMsg;
extern MENU_ENTRY entries[10];


extern int keypad_if;

void keypad_isr(void);

const int keypad_D0 = 5;
const int keypad_D1 = 6;
const int keypad_D2 = 7;
const int keypad_D3 = 8;


int prevKey = -1;
int currKey = -1;
// Set the Serial Baud Rate
// Attach an interrupt handler for pin d3 
// Call keypad_isr when the interreupt is triggered (LOW)
void setup_keypad()
{
  //d5 -> LSB ... d8 -> MSB
  Serial.print("Setup Keypad");
  pinMode(keypad_D0, INPUT);
  pinMode(keypad_D1, INPUT);
  pinMode(keypad_D2, INPUT);
  pinMode(keypad_D3, INPUT);
  attachInterrupt(1, keypad_isr, FALLING);
  Serial.print("Setup Complete");
  display_clear();  
}

void keypad_isr()
{
  keypad_if = true;  
}

void keypad_service()
{
 if(keypad_if) 
 {
  uint8_t val = digitalRead(keypad_D0) | (digitalRead(keypad_D1) << 1) | (digitalRead(keypad_D2) << 2) | (digitalRead(keypad_D3) << 3); 
   // Assumption: data is ready on keypad
   // Read whatever is needed
   // Call display_process if it was a * or #  
   if(inNewMsg)
   {
     prevKey = currKey;
     currKey = val;  
     if(prevKey != -1) 
     {
       Serial.print(keymap[prevKey][currKey]);
       prevKey = -1;
       currKey = -1;  
     }
     keypad_if = false;
   }
   // Choice 0 - *
   else if (val == 0x0C) 
   {
     display_process(entries[currEntry], currChoice);   // Yikes, entries is an external global!
     currChoice = 0;                                    // Reset currChoice after an action item has been selected    
   }
   else if (val == 0x0F)  // Choice 1 - D
   { 
      display_next();
   }  
   else // Else, for now, just CLEAR and then print to LCD panel
   {   
     display_clear();
     Serial.print(val, HEX);
   } 
   keypad_if = false; 
 }
  
}

