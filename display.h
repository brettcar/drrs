// Prototypes and typedef'd structures used by display module

typedef struct {
  char choice[6];          /* 6-character string for this choice. No terminator, use spaces for unused characters. */
  void (*callback)(void);  /* Callback to execute when this choice is selected */
} CHOICE;  

typedef struct {
  char message[16];        /* 16-character string to display on top line for this menu entry. No terminator, use spaces for unused chars. */
  CHOICE choices[4];       /* Two choices to display on second line. */
} MENU_ENTRY;

const char keymap[][4] = {{' ','1','1','1',}, {'A','B','C','2'}, {'D','E','F','3'}, {' ',' ',' ',' '}, 
                      {'G','H','I','4'}, {'J','K','L','5'}, {'M','N','O','6'}, {' ',' ',' ',' '},
                      {'P','Q','R','7'}, {'T','U','V','8'}, {'W','X','Y','9'}, {' ',' ',' ',' '},
                      {' ', ' ', ' ',' '}, {'Z','Z','Z','0'}};
                      
void display_inbox_setup(void);
void display_clear(void);
void display_inbox(void);
void display_next(void);
void display_config(void);
void display_newmsg(void);
void display_status(void);
void display_process(MENU_ENTRY entry, int choice);