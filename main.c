#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ncurses.h>

#include "logo.h"
#include "key.h"

#define MAXSOCKS 	1000


//#define MYADDR		"209.85.175.105"	// www.google.com
//#define MYPORT		80

//#define MYADDR	"69.164.208.28"
//#define MYPORT 8000

#define S_DISABLED		0	// 
#define S_DLYCONNECT		1	// 
#define S_CONNECTING		2	// 
#define S_DLYWRITE		3	// 
#define S_READING		4	// 
#define S_ERROR1		5	//
#define S_ERROR2		6	//

#define STATUSCHARS	"._-=weE789"


int status[MAXSOCKS];
long wait[MAXSOCKS];
int fd[MAXSOCKS];
struct sockaddr_in addr[MAXSOCKS];
int concurrent=0;

WINDOW *wHeader;
WINDOW *wData;
WINDOW *wLine;
WINDOW *wInput;
WINDOW *wStatus;


unsigned long GetMS() {
  struct timeval tv;
  if (gettimeofday(&tv, NULL)!=0) return 0;
  return (unsigned long)((tv.tv_sec*1000ul)+(tv.tv_usec/1000));
}



void HandleCommand(char *s) {
}




void SetStatus(int no, int st) {
  int y,x;
  
  status[no]=st;
  y=no/80;
  x=no-(y*80);
  mvwaddch(wData,y,x,STATUSCHARS[st]);
  wrefresh(wData);
}



void SetupScreen(void) {
  char 	tmps[100];
  int 	i;
  int 	majorV=0;
  int 	minorV=1;

  // Init ncurses
  initscr();
//  getmaxyx(stdscr, rows, cols);
  raw();
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE);
  noecho();
  refresh();
   
  // Create windows
  wHeader=newwin( 2, 80,  0, 0);
  wData=  newwin(13, 80,  2, 0);
  wLine=  newwin( 1, 80, 15, 0);
  wInput= newwin( 7, 80, 16, 0);
  wStatus=newwin( 2, 90, 23, 0);  

  //
  sprintf(tmps,"SockBlaster v%d.%d",majorV, minorV);  
  mvwprintw(wHeader,0,80-strlen(tmps),tmps);
  mvwhline(wHeader,1,0,ACS_HLINE,80);
  wrefresh(wHeader);

  for (i=0; i<13; i++) {
//    mvwprintw(wData,i,0,"DATA LINE %d",i);
  }
  wrefresh(wData);

  mvwhline(wLine,0,0,ACS_HLINE,80);
  wrefresh(wLine);

  mvwprintw(wInput,0,0,LOGOLINE1);
  mvwprintw(wInput,1,0,LOGOLINE2);
  mvwprintw(wInput,2,0,LOGOLINE3);
  mvwprintw(wInput,3,0,LOGOLINE4);

  wrefresh(wInput);
  scrollok(wInput, TRUE);
  keypad(wInput, TRUE);
  nodelay(wInput, TRUE);

  mvwhline(wStatus,0,0,ACS_HLINE,80);
  mvwprintw(wStatus,1,0,"***STATUS***");
  wrefresh(wStatus);
}




int main(int argc, char *argv[]) {
  char tmps[1024];
  int 	i;
  int	res;
  int 	n;
  fd_set rset, wset, eset;
  unsigned long now;
  unsigned long statusUpdate;
  socklen_t errlen;
  struct timeval waitd;
  int msgCnt;
  char keybuf[100];
  int lastkey=0;
  char *cmd;
  char cmd1[100], cmd2[100], cmd3[100], cmd4[100];
  char host[100]="127.0.0.1";
  unsigned int port=80;

  SetupScreen();
  keybuf[0]=0;


  for (i=0; i<MAXSOCKS; i++) {
    SetStatus(i,S_DISABLED);
  }
  
  for (i=0; i<concurrent; i++) {
    SetStatus(i,S_DLYCONNECT);
    wait[i]=GetMS()+rand()%3000;
  }


  statusUpdate=GetMS()+1000ul;
  msgCnt=0;
  int c[10];
  for (;;) {
    now=GetMS();
    if (now>statusUpdate) {
      statusUpdate=now+1000ul;
      c[0]=0; c[1]=0; c[2]=0; c[3]=0; c[4]=0; c[5]=0; c[6]=0; c[7]=0;
      for (i=0; i<MAXSOCKS; i++) {
        c[status[i]]++;
      }
      mvwprintw(wStatus,1,0,"%3d MPS %4d%c %4d%c %4d%c %4d%c %4d%c %4d%c %4d%c",
        msgCnt,
        c[0],STATUSCHARS[0],
        c[1],STATUSCHARS[1],
        c[2],STATUSCHARS[2],
        c[3],STATUSCHARS[3],
        c[4],STATUSCHARS[4],
        c[5],STATUSCHARS[5],
        c[6],STATUSCHARS[6]);
      wrefresh(wStatus);
      msgCnt=0;  
//      mvwprintw(wStatus,1,60,"%d %s   ",lastkey, keybuf);
    }

    for (i=0; i<MAXSOCKS; i++) {

      // Write some data to server
      if ((status[i]==S_DLYWRITE) && (now>wait[i])) {
        msgCnt++;
        res=write(fd[i],"HELLO!!\n",8);
        if (res==8) {
          SetStatus(i,S_READING);
        } else {
          SetStatus(i,S_ERROR1);
        }
      }      

      // Create a new socket and try to connect to server
      if ((status[i]==S_DLYCONNECT) && (now>wait[i])) {
        memset(&addr[i],0, sizeof(addr[0]));
        addr[i].sin_family=AF_INET;
        addr[i].sin_port=htons(port);
        addr[i].sin_addr.s_addr=inet_addr(host);
        fd[i]=socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (fd[i]<1) {
          sprintf(tmps,"Error opening socket #%d",i);
          perror(tmps);
          exit(1);
        }
        res=connect(fd[i],(struct sockaddr *)&addr[i], sizeof(addr[0]));
        if (res<0) {
          if (errno!= EINPROGRESS) {
            sprintf(tmps,"Connect(%d) failed",i);
            perror(tmps);
            exit(1);
          }
          SetStatus(i,S_CONNECTING);
        }
      }
    }

    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_ZERO(&eset);
    for (i=0; i<MAXSOCKS; i++) {
      if (status[i]==S_CONNECTING) {
        FD_SET(fd[i],&rset);
        FD_SET(fd[i],&wset);
        FD_SET(fd[i],&eset);
      }
      if (status[i]==S_READING) {
        FD_SET(fd[i],&rset);
        FD_SET(fd[i],&eset);
      }
    }

    waitd.tv_sec=0;
    waitd.tv_usec=10000;
    n=select(1024, &rset, &wset, &eset, &waitd);

    now=GetMS();
    for (i=0; i<MAXSOCKS; i++) {
      if (status[i]==S_CONNECTING) {
        if (FD_ISSET(fd[i], &wset)) {
          SetStatus(i,S_DLYWRITE);
          wait[i]=now+1000ul+(rand()%20)*500ul;
        }
      }
      if (status[i]==S_READING) {
        if (FD_ISSET(fd[i], &rset)) {
          res=read(fd[i],tmps,1024);
          if (res>0) {
            SetStatus(i,S_DLYWRITE);
            wait[i]=now+1000ul+(rand()%20)*500ul;
          } else {
            SetStatus(i,S_ERROR2);
          }
        }
      }
    }

  
    cmd=GetCommandFromKeyboard(wInput,6);
    if (cmd) {
      cmd1[0]=0;
      cmd2[0]=0;
      cmd3[0]=0;
      cmd4[0]=0;
      sscanf(cmd,"%s %s %s %s", cmd1, cmd2, cmd3, cmd4);      
      if (strcmp(cmd1,"exit")==0) break;
      if (strcmp(cmd1,"ip")==0) {
        strcpy(host, cmd2);
        mvwprintw(wInput,6,0,"Host IP set to %s", host);
      } else if (strcmp(cmd1,"port")==0) {
        port=atol(cmd2);
        mvwprintw(wInput,6,0,"Host PORT set to %d", port);
      } else if (strcmp(cmd1,"run")==0) {
        concurrent=atol(cmd2);
        mvwprintw(wInput,6,0,"Concurrent connections set to %d", concurrent);
        for (i=0; i<concurrent; i++) {
          SetStatus(i,S_DLYCONNECT);
          wait[i]=GetMS()+rand()%3000;
        }
      } else {
        mvwprintw(wInput,6,0,"Unknown command");
      }
      scroll(wInput);
      wrefresh(wInput);
    }  
    
  } // for(;;)       
/*
  
  
  }
  printf("Queueing done\r\n");

  int error;

  for (;;) {

    printf("select returned %d\r\n",n);
    for (i=0; i<MAXSOCKS; i++) {
      if (FD_ISSET(fd[i], &rset)) printf("R"); else printf("r");
    }
    printf(" ");
    for (i=0; i<MAXSOCKS; i++) {
      if (FD_ISSET(fd[i], &wset)) printf("W"); else printf("w");
    }
    printf(" ");
    for (i=0; i<MAXSOCKS; i++) {
      if (FD_ISSET(fd[i], &eset)) printf("E"); else printf("e");
    }
    printf("\r\n");


    for (i=0; i<MAXSOCKS; i++) {
      if (status[i]==S_CONNECTING) {
        if (FD_ISSET(fd[i], &wset)) {
          status[i]=S_CONNECTED;
          write(fd[i],"hello\n",6);
          printf("Wrote to number  %d\r\n",i);
        }
      }
      if (status[i]==S_CONNECTED) {
        if (FD_ISSET(fd[i], &rset)) {
          res=read(fd[i],tmps,1024);
          printf("Read %d bytes from number %d\r\n",res,i);
        }
      }
    }
  } 
*/

  endwin();
  printf("\r\n");
  return 0;
}
