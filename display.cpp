#import "WProgram.h"       /* Needed for access to Serial library */
#import <assert.h>

void display_clear(void);
void display_inbox(void);
void display_next(void);
void display_config(void);
void display_newmsg(void);
void display_status(void);

typedef struct {
  char choice[6];          /* 6-character string for this choice. No terminator, use spaces for unused characters. */
  void (*callback)(void);  /* Callback to execute when this choice is selected */
} CHOICE;  

typedef struct {
  char message[16];        /* 16-character string to display on top line for this menu entry. No terminator, use spaces for unused chars. */
  CHOICE choices[4];       /* Two choices to display on second line. */
} MENU_ENTRY;


MENU_ENTRY entries[10];
int currChoice;



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
    Serial.print(entry.choices[0].choice[i]);
    
  Serial.print("  ");
  Serial.print("NEXT  ");
}

void display_setup(void) 
{
  int size = 5 * sizeof(MENU_ENTRY);
  currChoice = 0;
  
  char *tmpStr = "MAIN MENU        ";
  memcpy(entries[0].message, tmpStr, 16);

  tmpStr = "INBOX ";
  memcpy(entries[0].choices[0].choice, tmpStr, 6);
  entries[0].choices[0].callback = &display_inbox;
  /*
  entries[0].choices[1].choice = "STATUS";
  entries[0].choices[1].callback = &display_status; 

  entries[0].choices[2].choice = "NEWMSG";
  entries[0].choices[2].callback = &display_newmsg;

  entries[0].choices[3].choice = "CONFIG";
  entries[0].choices[3].callback = &display_config;  
  */
}

void display_next(void)
{
  currChoice = (currChoice + 1) % 4;
//  MENU_ENTRY * currentEntry = (MENU_ENTRY*)entries[0];
  display_draw_entry(entries[0]);
}

void display_mainmenu(void)
{
  display_clear();
  display_draw_entry(entries[0]);
}
void display_inbox(void)
{
  // To do
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
  // Currently we only suppose 2 possible choices
  assert(choice < 2 && choice >= 0);
  entry.choices[choice].callback();
}
  
