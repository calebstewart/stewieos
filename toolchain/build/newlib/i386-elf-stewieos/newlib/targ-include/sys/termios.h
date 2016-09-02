#ifndef _SYS_TERMIOS_H_
#define _SYS_TERMIOS_H_

enum _cc_indexes
{
	VEOF,
	VEOL,
	VERASE,
	VINTR,
	VKILL,
	VMIN,
	VQUIT,
	VSTART,
	VSTOP,
	VSUSP,
	VTIME,
	NCCS
};

typedef union 
{
	struct termios* termios;
	struct {
		struct termios* termios;
		int brk;
	} brk;
	struct {
		struct termios* termios;
		int when;
	} attr;
} tcioctlparm_t;

// tty ioctl commands
#define TCDRAIN		0 // tcdrain()
#define TCFLOW		1 // tcflow()
#define TCFLUSH		2 // tcflush()
#define TCGETATTR	3 // tcgetattr()
#define TCGETSID	4 // tcgetsid()
#define TCSNDBRK	5 // tcsendbreak()
#define TCSETATTR	6 // tcsetattr()


// c_iflag values
#define BRKINT	0x0001
#define ICRNL	0x0002
#define IGNBRK	0x0004
#define IGNCR	0x0008
#define IGNPAR	0x0010
#define INLCR	0x0020
#define INPCK	0x0040
#define ISTRIP	0x0080
#define IUCLC	0x0100
#define IXANY	0x0200
#define IXOFF	0x0400
#define PARMRK	0x0800
// c_oflag values
#define OPOST	0x0001
#define OLCUC	0x0002
#define ONLCR	0x0004
#define OCRNL	0x0008
#define ONOCR	0x0010
#define ONLRET	0x0020
#define OFILL	0x0040
#define NLDLY	0x0080
#define NL0	0x0000
#define NL1	0x0080
#define CRDLY	0x0300
#define CR0	0x0000
#define CR1	0x0100
#define CR2	0x0200
#define CR3	0x0300
#define TABDLY	0x0C00
#define TAB0	0x0000
#define TAB1	0x0400
#define TAB2	0x0800
#define TAB3	0x0C00
#define VTDLY	0x1000
#define VT0	0x0000
#define VT1	0x1000
#define FFDLY	0x2000
#define FF0	0x0000
#define FF1	0x2000

// Baud Rates
#define B0	0
#define B50	1
#define B75	2
#define B110	3
#define B134	4
#define B150	5
#define B200	6
#define B300	7
#define B600	8
#define B1200	9
#define B1800	10
#define B2400	11
#define B4800	12
#define B9600	13
#define B19200	14
#define B38400	15

// c_cflag values
#define CSIZE	0x0003
#define CS5	0x0000
#define CS6	0x0001
#define CS7	0x0002
#define CS8	0x0003
#define CSTOPB	0x0004
#define CREAD	0x0008
#define PARENB	0x0010
#define PARODD	0x0020
#define HUPCL	0x0040
#define CLOCAL	0x0080

// c_lflag values
#define ECHO	0x0001
#define ECHOE	0x0002
#define ECHOK	0x0004
#define ECHONL	0x0008
#define ICANON	0x0010
#define IEXTEN	0x0020
#define ISIG	0x0040
#define NOFLSH	0x0080
#define TOSTOP	0x0100
#define XCASE	0x0200

// tcsetattr
#define TCSANOW 0
#define TCSADRAIN 1
#define TCSAFLUSH 2

// tcflush
#define TCIFLUSH 0
#define TCIOFLUSH 1
#define TCOFLUSH 2

// tcflow
#define TCIOFF 0
#define TCION 1
#define TCOOFF 2
#define TCOON 3

typedef char cc_t;
typedef unsigned char speed_t;
typedef unsigned int tcflag_t;

struct termios
{
	tcflag_t c_iflag;
	tcflag_t c_oflag;
	tcflag_t c_cflag;
	tcflag_t c_lflag;
	speed_t c_ispeed;
	speed_t c_ospeed;
	cc_t c_cc[NCCS];
};

speed_t cfgetispeed(const struct termios* termios);
speed_t cfgetospeed(const struct termios* termios);
int	cfsetispeed(const struct termios* termios, speed_t speed);
int	cfsetospeed(const struct termios* termios, speed_t speed);
int	tcdrain(int fd);
int	tcflow(int fd, int cmd);
int	tcflush(int fd, int cmd);
int	tcgetattr(int fd, struct termios* termios);
pid_t	tcgetsid(int fd);
int	tcsendbreak(int fd, int brk);
int	tcsetattr(int fd, int when, struct termios* termios);

#endif