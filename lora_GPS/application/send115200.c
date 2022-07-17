// gcc -o send115200 send115200.c rs232.c

#include <stdio.h>
#include <stdlib.h>
#include "rs232.h"
#include <sys/time.h>

#define N 500 // 108 crash

int main(int argc,char **argv)
{time_t t1,t2;
 struct timeval tv1,tv2;
 struct timezone tz;
 char t[N],r;
 int k;

 int fd,cnt=0;FILE *f;
 unsigned char cmd;
 fd=init_rs232(); 
 for (k=0;k<N;k++) t[k]=(char)((k+33)&0x7f);

 while (1)
   {write(fd,t,N);
    sleep(1);
   }
// time(&t1);
// gettimeofday(&tv1, &tz);

// while (cmd!='z') {read(fd,&cmd,1);printf("%x ",cmd&0xff);fflush(stdout);}
// time(&t2);
// gettimeofday(&tv2, &tz);
// printf("\nExecution = %d s ou  %d us\n",t2-t1,(tv2.tv_sec-tv1.tv_sec)*1000000+tv2.tv_usec-tv1.tv_usec);
}
