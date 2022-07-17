/* All examples have been derived from miniterm.c                          */
/* Don't forget to give the appropriate serial ports the right permissions */
/* (e. g.: chmod a+rw /dev/ttyS0)                                          */

#include "rs232.h"

struct termios oldtio,newtio;

int init_rs232()
{int fd;
 fd=open(HC11DEVICE, O_RDWR | O_NOCTTY ); 
 if (fd <0) {perror(HC11DEVICE); exit(-1); }
 tcgetattr(fd,&oldtio); /* save current serial port settings */
 bzero(&newtio, sizeof(newtio)); /* clear struct for new port settings */
// newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD; 
 newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;  /* _no_ CRTSCTS */
 newtio.c_iflag = IGNPAR; // | ICRNL |IXON; 
 newtio.c_oflag = IGNPAR; //ONOCR|ONLRET|OLCUC;
// newtio.c_lflag = ICANON; 
// newtio.c_cc[VINTR]    = 0;     /* Ctrl-c */ 
// newtio.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
// newtio.c_cc[VERASE]   = 0;     /* del */
// newtio.c_cc[VKILL]    = 0;     /* @ */
// newtio.c_cc[VEOF]     = 4;     /* Ctrl-d */
 newtio.c_cc[VTIME]    = 0;     /* inter-character timer unused */
 newtio.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */
// newtio.c_cc[VSWTC]    = 0;     /* '\0' */
// newtio.c_cc[VSTART]   = 0;     /* Ctrl-q */ 
// newtio.c_cc[VSTOP]    = 0;     /* Ctrl-s */
// newtio.c_cc[VSUSP]    = 0;     /* Ctrl-z */
// newtio.c_cc[VEOL]     = 0;     /* '\0' */
// newtio.c_cc[VREPRINT] = 0;     /* Ctrl-r */
// newtio.c_cc[VDISCARD] = 0;     /* Ctrl-u */
// newtio.c_cc[VWERASE]  = 0;     /* Ctrl-w */
// newtio.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
// newtio.c_cc[VEOL2]    = 0;     /* '\0' */
 tcflush(fd, TCIFLUSH);tcsetattr(fd,TCSANOW,&newtio);
// printf("RS232 Initialization done\n");
 return(fd);
}

void sendcmd(int fd,char *buf)
{unsigned int i,j;
 if((write(fd,buf,strlen(buf)))<strlen(buf)) 
    {printf("\n No connection...\n");exit(-1);}
 for (j=0;j<5;j++) for (i=0;i<3993768;i++) {}
 /* usleep(attente); */
}

void free_rs232(int fd)
{tcsetattr(fd,TCSANOW,&oldtio);close(fd);} /* restore the old port settings */
