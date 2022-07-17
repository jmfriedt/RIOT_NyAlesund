#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>                  /* declaration of bzero() */
#include <fcntl.h>
#include <termios.h>
int init_rs232();
void free_rs232();
void sendcmd(int,char*);

#define BAUDRATE B115200
#define HC11DEVICE "/dev/ttyUSB0"

