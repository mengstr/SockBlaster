#include <string.h>
#include <ncurses.h>
#include "key.h"


char *GetCommandFromKeyboard(WINDOW *win, int y) {
  static char previousLine[MAXKEYLINELENGTH];
  static char line[MAXKEYLINELENGTH];
  int ch;
  int len;
  
  ch=mvwgetch(win, y, strlen(line));
  if (ch==ERR) return NULL;
  
  // Return with string to caller if got ENTER on an
  // non-empty string
  if ((ch==10) && (strlen(line)>0)) {
    strcpy(previousLine, line);
    line[0]=0;
    scroll(win);
    wrefresh(win);
    return previousLine;
  }
  
  // If BACKSPACE and non-empty string, delete & backup cursor
  if ((ch==8) && (strlen(line)>0)) {
    line[strlen(line)-1]=0;
    mvwaddch(win,y,strlen(line),' ');
    wrefresh(win);
    return NULL;
  }

  // If normal key and length<MAX than add keypress to string
  if ((ch>=' ') && (ch<127) && (strlen(line)<MAXKEYLINELENGTH)) {
    len=strlen(line);
    mvwaddch(win,y,len,ch);
    line[len]=ch;
    line[len+1]=0;
  }

  return NULL;  
}
