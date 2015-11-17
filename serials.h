#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

void set_speed(int fd, int speed);

int set_Parity(int fd,int databits,int stopbits,int parity);

int openSerial(char *Dev);
