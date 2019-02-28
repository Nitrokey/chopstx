#define B0       0 /* POSIX to hang up */

/* POSIX supports B75 to B300 */
#define B600        16
#define B1200       17
#define B2400       19
#define B9600       21
#define B19200      22
#define B57600      24
#define B115200     25
#define B230400     26
#define B460800     27
#define B921600     28
#define BSCARD      63
#define MASK_BAUD 0x3f

/* POSIX supports 5, 6.  USB suppots 16 */
#define CS7       (2 << 6)
#define CS8       (3 << 6)
#define MASK_CS   (0x7 << 6)

#define STOP0B5	  (0 << 9) /* USART Hardware only */
#define STOP1B	  (1 << 9) /* USB, POSIX  */
#define STOP1B5	  (2 << 9) /* USB         */
#define STOP2B	  (3 << 9) /* USB, POSIX  */
#define MASK_STOP (0x3 << 9)

#define PARENB    (1 << 11)
#define PARODD    (2 << 11)
#define MASK_PAR  (0x7 << 11)
/* USB  0: none, 1: odd, 2: even, 3: mark, 4: space */

#define CRTSCTS   (1 << 14)
#define MASK_FLOW (0x1 << 14)

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

/* non-POSIX, non-USB-CDC configs */
#define MODE_SMARTCARD (1 << 30)
#define MODE_IRDA      (2UL << 30)
#define MODE_IRDA_LP   (3UL << 30)
#define MASK_MODE      (0x3UL << 30)
/* 0: standard, 1: smartcard, 2: IrDA, 3: IrDA-LP */

struct usart_stat {
  uint8_t dev_no;

  uint32_t tx;
  uint32_t rx;
  uint32_t rx_break;
  uint32_t err_notify_overflow;
  uint32_t err_rx_overflow;	/* software side */
  uint32_t err_rx_overrun;	/* hardware side */
  uint32_t err_rx_noise;
  uint32_t err_rx_parity;
};


void usart_init (uint16_t prio, uintptr_t stack_addr, size_t stack_size,
		 int (*ss_notify_callback) (uint8_t dev_no, uint16_t notify_bits));
int usart_config (uint8_t dev_no, uint32_t config_bits);
int usart_read (uint8_t dev_no, char *buf, uint16_t buflen);
int usart_write (uint8_t dev_no, char *buf, uint16_t buflen);
const struct usart_stat *usart_stat (uint8_t dev_no);
int usart_send_break (uint8_t dev_no);
void usart_config_clken (uint8_t dev_no, int on);
