#include "serials.h"

void set_speed(int fd, int speed)
{
    int   i;
    int   status;
    struct termios   Opt;

    int speed_arr[] = {B38400, B19200, B9600, B4800, B2400, B1200, B300,
        B38400, B19200, B9600, B4800, B2400, B1200, B300, };
    int name_arr[] = {38400,  19200,  9600,  4800,  2400,  1200,  300,
        38400,  19200,  9600, 4800, 2400, 1200,  300, };
    if(tcgetattr(fd, &Opt)!=0)
        perror("set_speed");
    for (i= 0;i<sizeof(speed_arr)/sizeof(int);i++) {
        if(speed == name_arr[i]) {
            tcflush(fd, TCIOFLUSH);
            cfsetispeed(&Opt, speed_arr[i]);
            cfsetospeed(&Opt, speed_arr[i]);
            status = tcsetattr(fd, TCSANOW, &Opt);
            if(status != 0)
                perror("tcsetattr fd1");
            return;
        }
    tcflush(fd,TCIOFLUSH);
    }
}

int set_Parity(int fd,int databits,int stopbits,int parity)
{
    struct termios options;
    if(tcgetattr(fd,&options)!=0) {
        perror("SetupSerial 1");
        return(0);
    }
    options.c_cflag &= ~CSIZE;
    switch (databits) {
        case 7:
            options.c_cflag |= CS7;
            break;
        case 8:
            options.c_cflag |= CS8;
            break;
        default:
            fprintf(stderr,"Unsupported data size\n");
            return (0);
    }
    switch (parity) {
        case 'n':
        case 'N':
            options.c_cflag &= ~PARENB;   /* Clear parity enable */
            options.c_iflag &= ~INPCK;     /* Enable parity checking */
            break;
        case 'o':
        case 'O':
            options.c_cflag |= (PARODD | PARENB);  /* ÉèÖÃÎªÆæÐ§Ñé*/ 
            options.c_iflag |= INPCK;             /* Disnable parity checking */
            break;
        case 'e':
        case 'E':
            options.c_cflag |= PARENB;     /* Enable parity */
            options.c_cflag &= ~PARODD;   /* ×ª»»ÎªÅ¼Ð§Ñé*/  
            options.c_iflag |= INPCK;       /* Disnable parity checking */
            break;
        case 'S':
        case 's':  /*as no parity*/
            options.c_cflag &= ~PARENB;
            options.c_cflag &= ~CSTOPB;
            break;
        default:
            fprintf(stderr,"Unsupported parity\n");
            return (0);
    }
    switch (stopbits) {
        case 1:
            options.c_cflag &= ~CSTOPB;
            break;
        case 2:
            options.c_cflag |= CSTOPB;
            break;
        default:
            fprintf(stderr,"Unsupported stop bits\n");
            return (0);
    }
  /* Set input parity option */
    if (parity!='n')
        options.c_iflag |= INPCK;
    options.c_cc[VTIME] = 150; // 15 seconds
    options.c_cc[VMIN] = 0;

    tcflush(fd,TCIFLUSH); /* Update the options and do it NOW */
    if (tcsetattr(fd,TCSANOW,&options)!=0) {
        perror("SetupSerial 3");
        return (0);
    }
    return (1);
}

int openSerial(char *Dev)
{
    int fd = open(Dev, O_RDWR); //| O_NOCTTY | O_NDELAY
    if (-1 == fd) {
        perror("Can't Open Serial Port");
        return -1; 
    } else {
        set_speed(fd, 19200);
        if (set_Parity(fd, 8, 1, 'N')==0) {
            printf("Set Parity Error\n");
            exit (0);
        }   
        return fd; 
    }   
}

