
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/time.h>
// #include <linux/serial.h>

int get_data(int des, unsigned char *buf, int len)
{
    int ind = 0;
    while(ind < len) {
	int r_len = read(des, buf + ind, len - ind);
	if (r_len < 1) {
	    printf("Read(%d) returned %d\n", len - ind, r_len);
	    perror("read");
	    return -1;
	}
	ind += r_len;
    }
    return ind;
}

int set_state(int fd, int brk,int rts)
{
  int flags = 0; 
  int setflags = 0, clearflags = 0;
  
  if (brk)
    setflags   |= TIOCM_DTR;
  else
    clearflags |= TIOCM_DTR;

  if (rts)
    setflags   |= TIOCM_RTS;
  else
    clearflags |= TIOCM_RTS;

  errno = 0;
  if (ioctl(fd, TIOCMGET, &flags) < 0){
      //    fprintf(stderr, "open: Ioctl TIOCMGET failed %s\n", strerror(errno));
    flags = 0x166;
  } 

  //  printf("Flags[0x%X]\n", flags);

  flags |= setflags;
  flags &= ~clearflags;

  if (ioctl(fd, TIOCMSET, &flags) < 0){
    fprintf(stderr, "open: Ioctl TIOCMSET failed %s\n", strerror(errno));
    return (-1);
  }
  return(0);
}

/* int set_baudrate(int fd, int rate){ */

/*  { */
/*    struct serial_struct old_serinfo; */
/*    struct serial_struct new_serinfo; */
/*    struct termios       old_termios; */
/*    struct termios       new_termios; */

/*    if (ioctl(fd, TIOCGSERIAL, &old_serinfo) < 0){ */
/*      perror("Cannot get serial info"); return -1; */
/*    } */

/*    new_serinfo = old_serinfo; */
/*    new_serinfo.custom_divisor = new_serinfo.baud_base / rate; */
/*    new_serinfo.flags =   (new_serinfo.flags & ~ASYNC_SPD_MASK) | ASYNC_SPD_CUST; */

/*    if (ioctl(fd, TIOCSSERIAL, &new_serinfo) < 0){ */
/*      perror("Cannot set serial info");return -1; */
/*    } */

/*     if (tcgetattr(fd, &old_termios) < 0){ */
/*       perror("Cannot get terminal attributes"); return -1; */
/*     } */
   
/*     new_termios = old_termios; */
/*     /\* Change settings to 38400 baud.  The driver will */
/*      * substitute this with the custom baud rate.  *\/ */

/*     cfsetospeed(&new_termios, B38400); */
/*     cfsetispeed(&new_termios, B0); */
/*     if (tcsetattr(fd, TCSANOW, &new_termios) < 0){ */
/*       perror("Cannot set terminal attributes"); return -1; */
/*     } */
/*  } */
/*  return 0; */
/* } */

unsigned char calc_csum(unsigned char *start, int len)
{
    int csum = 0;
    while(len--) {
	csum += *start++;
    }
    return csum % 0x100;
}

int main(int argc, char *argv[])
{
    int des;
    struct termios options;
    unsigned char buf[128];
    int len;
    int i;

    if (argc != 2) {
	printf("Usgae: %s tty-device\n", argv[0]);
	exit(255);
    }

    if ((des = open(argv[1], O_RDWR)) == -1) {
	perror(argv[1]);
	exit(255);
    }

    write(des, "", 0);
    if (tcgetattr(des, &options) < 0){
	perror("Cannot get terminal attributes"); return -1;
    }
   
    /*
     * Change settings to 38400 baud.  The driver will
     * substitute this with the custom baud rate.  
     */
    cfmakeraw(&options);
    options.c_cflag = CS8 | CLOCAL | CREAD;
    options.c_iflag = IGNBRK | IGNPAR; // Ignore break, parity and map cr->nl
    options.c_oflag = 0;
    options.c_cc[VTIME] = 0; // 200 msec tout
    options.c_cc[VMIN] = 1;
    cfsetispeed(&options, B4800);
    cfsetospeed(&options, B4800);

    if (tcsetattr(des, TCSANOW, &options) < 0){
	perror("Cannot set terminal attributes"); return -1;
    }
   
    // Not needed since we run std 4800 BPS.
    //     if(set_baudrate(des, 4800) != 0)
    // 	 return -1;

    set_state(des, 1, 1);
    usleep(1800000);
    set_state(des, 0, 1);
    usleep(200000);
    set_state(des, 1, 0);
    usleep(200000);
    set_state(des, 0, 0);
    usleep(100000);

    // Send init string
    buf[0] = 0x80; // Hdr start 
    buf[1] = 0x10; // Target=ECU
    buf[2] = 0xF0; // Source=Diagnostic tool
    buf[3] = 0x01;    // Lengnth=Total length (including checksum) - 5. (Lenght of payload only).
    buf[4] = 0xBF; // Command = ECU init.
    buf[5] = calc_csum(buf, 5);

    printf("write1=%d\n", write(des, buf, 6));

    len = get_data(des, buf, 68);
    printf("len[%d]\n", len);

    for (i = 0; i < len; ++i) {
	printf("byte[%d] = 0x%.2X / %d\n", i, buf[i] & 0xFF, buf[i] & 0xFF);
    }


    //     set_state(des, 0, 1);
    while(1) {
	len = 0;
	buf[len++] = 0x80; // Hdr start 
	buf[len++] = 0x10; // Target=ECU
	buf[len++] = 0xF0; // Source=Diagnostic tool
	buf[len++] = 0x00; // FIlled in below
	buf[len++] = 0xA8;
	buf[len++] = 0x00;
	buf[len++] = 0x00;
	buf[len++] = 0x00;
	buf[len++] = 0x08;
	buf[len++] = 0x00;
	buf[len++] = 0x00;
	buf[len++] = 0x0E;
	buf[len++] = calc_csum(buf, len);
	buf[3] = len - 5;

	//	printf("write len[%d]\n", len);

	len = write(des, buf,len);
	printf("write2=%d\n", len);
	len = get_data(des, buf, len + 1);
 	printf("len[%d]\n", len);
	for (i = 0; i < len; ++i) {
	    printf("byte[%d] = 0x%.2X / %d\n", i, buf[i] & 0xFF, buf[i] & 0xFF);
	}
	printf("rpm[%d]\n", ((buf[19] << 8 | buf[20]) & 0xFFFF) / 4);
    }
//     set_state(des, 1, 0);

//     // Send init string
//     buf[0] = 0x80; // Hdr start 
//     buf[1] = 0x10; // Target=ECU
//     buf[2] = 0xF0; // Source=Diagnostic tool
//     buf[3] = 0x01;    // Lengnth=Total length (including checksum) - 5. (Lenght of payload only).
//     buf[4] = 0xBF; // Command = ECU init.
//     buf[5] = calc_csum(buf, 5);

//     printf("write3=%d\n", write(des, buf, 6));
//     len = read(des, buf, 100);
//     printf("len[%d]\n", len);
//     for (i = 0; i < len; ++i) {
// 	printf("byte[%d] = 0x%.2F / %d\n", i, buf[i] & 0xFF, buf[i] & 0xFF);
//     }

//     set_state(des, 1, 1);

//     // Send init string
//     buf[0] = 0x80; // Hdr start 
//     buf[1] = 0x10; // Target=ECU
//     buf[2] = 0xF0; // Source=Diagnostic tool
//     buf[3] = 0x01;    // Lengnth=Total length (including checksum) - 5. (Lenght of payload only).
//     buf[4] = 0xBF; // Command = ECU init.
//     buf[5] = calc_csum(buf, 5);

//     printf("write4=%d\n", write(des, buf, 6));
//     len = read(des, buf, 100);
//     printf("len[%d]\n", len);
//     for (i = 0; i < len; ++i) {
// 	printf("byte[%d] = 0x%.2F / %d\n", i, buf[i] & 0xFF, buf[i] & 0xFF);
//     }
}
