// Prototypes and typedef'd structures used by display module

typedef struct {
  char choice[6];          /* 6-character string for this choice. No terminator, use spaces for unused characters. */
  void (*callback)(void);  /* Callback to execute when this choice is selected */
} CHOICE;  

typedef struct {
  char message[16];        /* 16-character string to display on top line for this menu entry. No terminator, use spaces for unused chars. */
  CHOICE choices[10];       /* Two choices to display on second line. */
} MENU_ENTRY;

const char keymap[][5] = {{'1',' ',' ',' ',' '}, {'2','A','B','C',' '}, {'3','D','E','F', ' '}, {' ',' ',' ',' ',' '}, 
                      {'4','G','H','I', ' '}, {'5','J','K','L', ' '}, {'6','M','N','O', ' '}, {' ',' ',' ',' '},
                      {'7','P','Q','R','S'}, {'8','T','U','V',' '}, {'9','W','X','Y','Z'}, {' ',' ',' ',' '},
                      {' ', ' ', ' ',' '}, {'0',' ',' ',' ',' '}};
void display_update(void);
void display_clear(void);
void display_config(void);
void display_back(void);
void display_inbox(void);
void display_inbox_setup(void);
void display_mainmenu(void);
void display_newmsg(void);
void display_next(void);
void display_process(MENU_ENTRY entry, int choice);
void display_setup(void);
void display_setup_lcd(void);
void display_status(void);
void display_config_setup(void);
void display_config(void);
void display_draw_entry(MENU_ENTRY entry);
void display_set_bright(uint8_t level);

extern uint8_t currDisplay[32];
