#import "WProgram.h"       /* Needed for access to Serial library */
#import <assert.h>
#import "display.h"

MENU_ENTRY entries[10];
int currEntry;
int currChoice;
int currMsg;
char recMessages[2][32];          

void display_clear(void) {
#ifdef CLEAR_FAST
  Serial.print("                                "); // 32 spaces
#else
  Serial.print (0xFE, BYTE); 
  Serial.print (0x01, BYTE);
#endif
}

void display_draw_entry(MENU_ENTRY entry)
{
  display_clear();
  for(int i = 0; i < 16; i++) 
    Serial.print(entry.message[i]);
    
  Serial.print("<");
  for(int i = 0; i < 6; i++)
    Serial.print(entry.choices[currChoice].choice[i]);
    
  Serial.print("  ");
  Serial.print("NEXT  ");
}

void display_setup(void) 
{
  currChoice = 0;
  char *tmpStr = "MAIN MENU       ";
  memcpy(entries[0].message, tmpStr, 16);
  
  tmpStr = "INBOX ";
  memcpy(entries[0].choices[0].choice, tmpStr, 6);
  entries[0].choices[0].callback = &display_inbox;
  
  tmpStr = "STATUS";
  memcpy(entries[0].choices[1].choice, tmpStr, 6);
  entries[0].choices[1].callback = &display_status; 
  
  tmpStr = "NEWMSG";
  memcpy(entries[0].choices[2].choice, tmpStr, 6);
  entries[0].choices[2].callback = &display_newmsg;

  tmpStr = "CONFIG";
  memcpy(entries[0].choices[3].choice, tmpStr, 6);
  entries[0].choices[3].callback = &display_config;    
  
  strcpy(recMessages[0], "This is message 0.");
  strcpy(recMessages[1], "This is message 1.");
  display_inbox_setup();
}

void display_next(void)
{  
  currChoice = (currChoice + 1) % 2; // <-- This mod value can't be hardcoded (varies depending on the current menu)
  display_draw_entry(entries[currEntry]);
}

void display_mainmenu(void)
{
  currEntry = 0;
  display_clear();
  display_draw_entry(entries[currEntry]);
}

void display_inbox_setup(void)
{  
  char* tmpStr = "INBOX           ";
  memcpy(entries[1].message, tmpStr, 16);
  
  tmpStr = "MSG 1 ";  
  memcpy(entries[1].choices[0].choice, tmpStr, 6);
  
  tmpStr = "MSG 2 ";
  memcpy(entries[1].choices[1].choice, tmpStr, 6);
}

void display_show_currMsg(void)
{
   display_clear();
   for(int i = 0; i < strlen(recMessages[currMsg]) ; i++)
     Serial.print(recMessages[currMsg][i]);
}

void display_inbox(void)
{
  currEntry = 1; 
  for(currMsg = 0; strlen(recMessages[currMsg]) > 0; currMsg++)
  {
     entries[1].choices[currMsg].callback = &display_show_currMsg;
  }
  display_draw_entry(entries[currEntry]);
  
}

void display_status(void)
{
  // To do
}

void display_newmsg(void)
{
  // To do
}

void display_config(void)
{
  // To do
}

/* Process a "choice" for the current MENU_ENTRY 'entry'. Basically,
   just call the callback! */
void display_process(MENU_ENTRY entry, int choice)
{

//  assert(choice < 2 && choice >= 0);
  entry.choices[choice].callback();
}
  
