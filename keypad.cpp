#import "WProgram.h"       /* Needed for access to Serial library */
#import "display.h"

// Yikes! We need a better way to avoid the use of globals!
extern int currChoice;
extern int currMsg;
extern MENU_ENTRY entries[10];


void keypad_isr(void);

const int keypad_D0 = 5;
const int keypad_D1 = 6;
const int keypad_D2 = 7;
const int keypad_D3 = 8;
  
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
 uint8_t val = digitalRead(keypad_D0) | (digitalRead(keypad_D1) << 1) | (digitalRead(keypad_D2) << 2) | (digitalRead(keypad_D3) << 3); 
 // Assumption: data is ready on keypad
 // Read whatever is needed
 // Call display_process if it was a * or #
  
 // Choice 0 - *
 if (val == 0x0C) 
 {
    if(currChoice == 0) // Inbox
      display_process(entries[currChoice], currMsg);   // Yikes, entries is an external global!
    
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
}

