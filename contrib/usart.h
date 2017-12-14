#define B0       0 /* POSIX to hang up */

/* POSIX supports B75 to B300 */
#define B600     16
#define B1200    17
#define B2400    19
#define B9600    21
#define B19200   22
#define B57600   24
#define B115200  25
#define B230400  26
#define B460800  27
#define B921600  28

/* POSIX supports 5, 6.  USB suppots 16 */
#define CS7      (2 << 6)
#define CS8      (3 << 6)

#define STOP0B5	 (0 << 9) /* Driver only */
#define STOP1B	 (1 << 9) /* USB, POSIX  */
#define STOP1B5	 (2 << 9) /* USB         */
#define STOP2B	 (3 << 9) /* USB, POSIX  */

#define PARENB   (1 << 11)
#define PARODD   (2 << 11)
/* USB  0: none, 1: odd, 2: even, 3: mark, 4: space */

#define CRTSCTS  (1 << 14)

/*
BAUD_BITS 6
CS_BITS   3
STOP_BITS 2
PAR_BITS  3
*/
/* USB: SET_CONTROL_LINE_STATE
   DTR RTS */
/* USB: SERIAL_STATE
   DSR DCD RI */
