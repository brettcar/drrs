#import "WProgram.h"       /* Needed for access to Serial library */
#import <assert.h>

typedef struct {
  char choice[6];          /* 6-character string for this choice. No terminator, use spaces for unused characters. */
  void (*callback)(void);  /* Callback to execute when this choice is selected */
} CHOICE;  

typedef struct {
  char message[16];        /* 16-character string to display on top line for this menu entry. No terminator, use spaces for unused chars. */
  CHOICE choices[2];       /* Two choices to display on second line. */
} MENU_ENTRY;


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
  Serial.print(entry.message);
  Serial.print("<");
  Serial.print(entry.choices[0].choice);
  Serial.print("  ");
  Serial.print(entry.choices[1].choice);
}

/* Process a "choice" for the current MENU_ENTRY 'entry'. Basically,
   just call the callback! */
void display_process(MENU_ENTRY entry, int choice)
{
  // Currently we only suppose 2 possible choices
  assert(choice < 2 && choice >= 0);
  entry.choices[choice].callback();
}
  
