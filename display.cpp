#import "WProgram.h"       /* Needed for access to Serial library */
#import <assert.h>
#import <stdio.h>
#import "display.h"
#import "config.h"
#import "txvr.h"

MENU_ENTRY entries[4]; 
int currEntry;  // Used to index through MainMenu, Inbox, Config, etc.
int currChoice; // Used to index through items in above menus (mgs, config options etc.)
boolean inNewMsg;

static uint8_t realDisplay[32]; // PRIVATE
uint8_t currDisplay[32]; // Code should only modify currDisplay. realDisplay is private.

void display_update(void) {
  for (uint8_t i = 0; i < 32; i++)
  {
    if (realDisplay[i] != currDisplay[i])
    { 
      memcpy(realDisplay, currDisplay, 32); 
      display_clear();
      for (register uint8_t i = 0; i < 32; i++)
      {
        Serial.print(currDisplay[i]);
      }
      break;
    }
  }
}

void display_clear(void) {
#ifdef CLEAR_FAST
  Serial.print("                                "); // 32 spaces
#else
 //Serial.println(" ");
 Serial.print (0xFE, BYTE); 
 Serial.print (0x01, BYTE);
#endif
}

void display_draw_entry(MENU_ENTRY entry)
{
 display_clear();
 register int i;
 memset(currDisplay, ' ', 32);
 uint8_t * ptr = currDisplay;
 memcpy(ptr, entry.message, 16);
 ptr += 16;
 *ptr = '<';  
 ptr++;
 memcpy(ptr, entry.choices[currChoice].choice, 6);
 ptr += 6;
 const char * trailer = "    NEXT>"; 
 memcpy(ptr, trailer, strlen(trailer));
}

void display_setup_lcd(void)
{
  Serial.begin (9600);  
  // Clear LCD
  Serial.print (0xFE, BYTE); 
  Serial.print (0x01, BYTE);
  memset(currDisplay, ' ', 32);
}

void display_setup(void) 
{
  currChoice = 0;
  char *tmpStr = " MAIN MENU      ";
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
  
  display_inbox_setup();
  display_config_setup();
  

  // TODO: Erase this code
  // Create dummy packet
  PACKET *pkt = (PACKET*) malloc(sizeof(PACKET));
  if(pkt == NULL)
    Serial.print("Out of mem");
    
  memset(pkt, 0, sizeof(PACKET));
  pkt->id = 120;
  strcpy((char*) pkt->msgpayload, "Hi there, face here.");
  pkt->msglen = strlen((char*)pkt->msgpayload);
  txvr_add_inbox(pkt);
  
  pkt = (PACKET*) malloc(sizeof(PACKET));
  pkt->id = 121;
  strcpy((char*) pkt->msgpayload, "Hello world.");
  pkt->msglen = strlen((char*)pkt->msgpayload);
  txvr_add_inbox(pkt);
}

void display_next(void)
{  
  switch(currEntry)
  {
    // Main Menu
    case 0:
       currChoice = (currChoice + 1) % 4; 
       break;       
    // Inbox  
    case 1:
      currChoice = (currChoice + 1) % (dlist_size(&inboxList) == 0 ? 2 : (dlist_size(&inboxList) + 1));
      break;
    // New Message
    case 2:
      // newMsg choices here
      break;
    // Config  
    case 3:
      currChoice = (currChoice + 1) % 4;
      break;
    default:
      break;
  }    
  display_draw_entry(entries[currEntry]);
}


void display_back(void)
{
   currChoice = 0; // Reset currChoice
   currEntry = 0;  // Go back to menu entry
   display_draw_entry(entries[currEntry]); 
}

// index is the index to add the BACK item in
void display_addback(int entry, int index)
{
  char *tmpStr = "BACK  ";
  memcpy(entries[entry].choices[(index)].choice, tmpStr, 6);
  entries[entry].choices[(index)].callback = &display_back;
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
  
  tmpStr = "EMPTY ";  
  memcpy(entries[1].choices[0].choice, tmpStr, 6);
  entries[1].choices[0].callback = &display_back;
}

void display_config_setup(void)
{  
 // Config is entries[3]
  char* tmpStr = "CONFIG          ";
  memcpy(entries[3].message, tmpStr, 16);
  
  tmpStr = "ID    ";
  memcpy(entries[3].choices[0].choice, tmpStr, 6);
  
  tmpStr = "FREQ  ";
  memcpy(entries[3].choices[1].choice, tmpStr, 6);
  
  tmpStr = "BRIGHT";
  memcpy(entries[3].choices[2].choice, tmpStr, 6);
  entries[3].choices[0].callback = &config_set_id_callback;
  entries[3].choices[1].callback = &config_set_freq_callback;
  entries[3].choices[2].callback = &config_set_bright_callback;
  
  display_addback(3,3);
}

void display_config(void)
{
  // TODO
  currEntry = 3;
  currChoice = 0;
  display_draw_entry(entries[currEntry]);
}

void display_show_currMsg(void)
{
   display_clear();
   memset(currDisplay, ' ', 32);
   
   DListElmt * thisElement;
   thisElement = dlist_head(&inboxList);
   for(int i = 0; i < currChoice; i++)
      thisElement = dlist_next(thisElement);
      
   PACKET * pkt = (PACKET*)dlist_data(thisElement);
   if (pkt != NULL)
     memcpy(currDisplay, pkt->msgpayload, pkt->msglen);
}

void display_inbox(void)
{  
  // Inbox is MENU_ENTRY 1 (entries[1])
  currChoice = 0;
  currEntry = 1; 
  
  // Start a for loop that's going to find all the msgs in the inbox 
  // And assign them the proper callback function
  int i;
  for(i = 0; i < dlist_size(&inboxList); i++)
  {
     char tmp[7];
     snprintf(tmp, 7, "MSG %2i", (i+1));           
     memcpy(entries[currEntry].choices[i].choice, tmp, 6);
     entries[currEntry].choices[i].callback = &display_show_currMsg;
  }
  display_addback(1,i == 0 ? 1 : i);
  display_draw_entry(entries[currEntry]);    
}

void display_status(void)
{
  // TODO
}

void display_newmsg(void)
{
  currChoice = 0;
  currEntry = 2;
  inNewMsg = true;
  display_clear();
}

/* Process a "choice" for the current MENU_ENTRY 'entry'. Basically,
   just call the callback! */
void display_process(MENU_ENTRY entry, int choice)
{
  entry.choices[choice].callback();
}

void display_set_bright(uint8_t level)
{
  // Ignore invalid levels.
  if (level < 128 || level > 157)
    return;
  Serial.print(0x7C, BYTE);
  Serial.print(level, BYTE);
}
