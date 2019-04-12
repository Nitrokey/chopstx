#include <stdint.h>
#include <stdlib.h>
#include <chopstx.h>
#include <mcu/stm32l.h>
#include <contrib/usart.h>


#define RCC_APB1_1_USART2     (1 << 17)
#define RCC_APB2_USART1       (1 << 14)

/* Hardware registers */
struct USART {
  volatile uint32_t CR1;
  volatile uint32_t CR2;
  volatile uint32_t CR3;
  volatile uint32_t BRR;
  volatile uint32_t GTPR;
  volatile uint32_t RTOR;
  volatile uint32_t RQR;
  volatile uint32_t ISR;
  volatile uint32_t ICR;
  volatile uint32_t RDR;
  volatile uint32_t TDR;
};

#define USART1_BASE           (APB2PERIPH_BASE + 0x3800)

#define USART2_BASE           (APB1PERIPH_BASE + 0x4400)
#define USART2 ((struct USART *)USART2_BASE)

#define USART_ISR_CTS	(1 << 10)
#define USART_ISR_LBDF	(1 << 8)
#define USART_ISR_TXE	(1 << 7)
#define USART_ISR_TC	(1 << 6)
#define USART_ISR_RXNE	(1 << 5)
#define USART_ISR_IDLE	(1 << 4)
#define USART_ISR_ORE	(1 << 3)
#define USART_ISR_NE	(1 << 2)
#define USART_ISR_FE	(1 << 1)
#define USART_ISR_PE	(1 << 0)

#define USART_CR1_M1		(1 << 28)
#define USART_CR1_M0		(1 << 12)
#define USART_CR1_WAKE		(1 << 11)
#define USART_CR1_PCE		(1 << 10)
#define USART_CR1_PS		(1 <<  9)
#define USART_CR1_PEIE		(1 <<  8)
#define USART_CR1_TXEIE		(1 <<  7)
#define USART_CR1_TCIE		(1 <<  6)
#define USART_CR1_RXNEIE	(1 <<  5)
#define USART_CR1_IDLEIE	(1 <<  4)
#define USART_CR1_TE		(1 <<  3)
#define USART_CR1_RE		(1 <<  2)
#define USART_CR1_UESM		(1 <<  1)
#define USART_CR1_UE		(1 <<  0)

#define USART_CR3_CTSE		(1 <<  9)
#define USART_CR3_RTSE		(1 <<  8)
#define USART_CR3_SCEN		(1 <<  5)
#define USART_CR3_NACK		(1 <<  4)

static struct usart_stat usart2_stat;

static struct chx_intr usart2_intr;

#define BUF_A2H_SIZE 256
#define BUF_H2A_SIZE 512
static uint8_t buf_usart2_rb_a2h[BUF_A2H_SIZE];
static uint8_t buf_usart2_rb_h2a[BUF_H2A_SIZE];

static struct rb usart2_rb_a2h;
static struct rb usart2_rb_h2a;

static chopstx_poll_cond_t usart2_app_write_event;

/* Global variables so that it can be easier to debug.  */
static int usart2_tx_ready;

#define INTR_REQ_USART2 38

#define USART_DEVNO_START 2
#define USART_DEVNO_END   2

struct usart {
  struct USART *USART;
  struct usart_stat *stat;
  struct chx_intr *intr;
  struct rb *rb_a2h;
  struct rb *rb_h2a;
  uint8_t *buf_a2h;
  uint8_t *buf_h2a;
  chopstx_poll_cond_t *app_write_event;
  int *tx_ready;
  uint8_t irq_num;
};


static const struct usart usart_array[] =
  {
   { USART2, &usart2_stat, &usart2_intr, &usart2_rb_a2h, &usart2_rb_h2a,
     buf_usart2_rb_a2h, buf_usart2_rb_h2a, &usart2_app_write_event,
     &usart2_tx_ready, INTR_REQ_USART2 },
  };
#define NUM_USART ((int)(sizeof (usart_array) / sizeof (struct usart)))

static int handle_intr (struct USART *USARTx, struct rb *rb2a, struct usart_stat *stat);
static int handle_tx (struct USART *USARTx, struct rb *rb2h, struct usart_stat *stat);
static void usart_config_recv_enable (struct USART *USARTx, int on);

#include "usart-common.c"

/* We assume 40MHz f_CK */

static const struct brr_setting brr_table[] = {
  { B600,    66667 },
  { B1200,   33333 },
  { B2400,   16667 },
  { B9600,    4167 },
  { B19200,   2083 },
  { B57600,    694 },
  { B115200,   347 },
  { B230400,   174 },
  { B460800,    87 },
  { B921600,    43 },
  { BSCARD,   4167 },
};

void
usart_config_brr (uint8_t dev_no, uint16_t brr_value)
{
  struct USART *USARTx = get_usart_dev (dev_no);
  uint32_t save_bits;

  save_bits = USARTx->CR1 & (USART_CR1_TE | USART_CR1_RE);
  USARTx->CR1 &= ~(USART_CR1_TE | USART_CR1_RE | USART_CR1_UE);
  USARTx->BRR = brr_value;
  USARTx->CR1 |= (save_bits | USART_CR1_UE);
}

/* XXX: not sure if needed */
static void
usart_config_recv_enable (struct USART *USARTx, int on)
{
  if (on)
    USARTx->CR1 |= USART_CR1_RE;
  else
    USARTx->CR1 &= ~USART_CR1_RE;
}

void
usart_config_clken (uint8_t dev_no, int on)
{
  struct USART *USARTx = get_usart_dev (dev_no);

  if (on)
    USARTx->CR2 |= (1 << 11);
  else
    USARTx->CR2 &= ~(1 << 11);
}


int
usart_config (uint8_t dev_no, uint32_t config_bits)
{
  struct USART *USARTx = get_usart_dev (dev_no);
  uint8_t baud_spec = (config_bits & MASK_BAUD);
  int i;
  uint32_t cr1_config = (USART_CR1_UE | USART_CR1_RXNEIE
			 | USART_CR1_TE | USART_CR1_RE);
				/* TXEIE/TCIE will be enabled when
				   putting char */
				/* No CTSIE, PEIE, IDLEIE, LBDIE */
  if (USARTx == NULL)
    return -1;

  /* Disable USART before configure.  */
  USARTx->CR1 &= ~USART_CR1_UE;

  if ((config_bits & MASK_CS) == CS7 && !(config_bits & PARENB))
    cr1_config |=  USART_CR1_M1;
  else if (((config_bits & MASK_CS) == CS7 && (config_bits & PARENB))
      || ((config_bits & MASK_CS) == CS8 && (config_bits & PARENB) == 0))
    ;
  else if ((config_bits & MASK_CS) == CS8)
    cr1_config |=  USART_CR1_M0;
  else
    return -1;

  if ((config_bits & PARENB))
    cr1_config |=  (USART_CR1_PCE | USART_CR1_PEIE);

  if ((config_bits & PARODD))
    cr1_config |=  USART_CR1_PS;

  if ((config_bits & MASK_STOP) == STOP0B5)
    USARTx->CR2 = (0x1 << 12);
  else if ((config_bits & MASK_STOP) == STOP1B)
    USARTx->CR2 = (0x0 << 12);
  else if ((config_bits & MASK_STOP) == STOP1B5)
    USARTx->CR2 = (0x3 << 12);
  else /* if ((config_bits & MASK_STOP) == STOP2B) */
    USARTx->CR2 = (0x2 << 12);

  for (i = 0; i < NUM_BAUD; i++)
    if (brr_table[i].baud_spec == baud_spec)
      break;

  if (i >= NUM_BAUD)
    return -1;

  USARTx->BRR = brr_table[i].brr_value;

  if ((config_bits & MASK_FLOW))
    USARTx->CR3 = USART_CR3_CTSE | USART_CR3_RTSE;
  else
    USARTx->CR3 = 0;

  if ((config_bits & MASK_MODE))
    {
      if ((config_bits & MASK_MODE) == MODE_SMARTCARD)
	{
	  USARTx->GTPR = (1 << 8) | 5;
	  USARTx->CR3 |= (USART_CR3_SCEN | USART_CR3_NACK);
	}
      else if ((config_bits & MASK_MODE) == MODE_IRDA)
	USARTx->CR3 |= (1 << 1);
      else if ((config_bits & MASK_MODE) == MODE_IRDA_LP)
	USARTx->CR3 |= (1 << 2) | (1 << 1);
    }

  USARTx->CR1 = cr1_config;

  return 0;
}


void
usart_init0 (int (*cb) (uint8_t dev_no, uint16_t notify_bits))
{
  int i;

  ss_notify_callback = cb;

  for (i = 0; i < NUM_USART; i++)
    {
      usart_array[i].stat->dev_no = i + USART_DEVNO_START;
      chopstx_claim_irq (usart_array[i].intr, usart_array[i].irq_num);
    }

  /* Enable USART2 clock, and strobe reset.  */
  RCC->APB1ENR1 |= (1 << 17);
  RCC->APB1RSTR1 = (1 << 17);
  RCC->APB1RSTR1 = 0;
}

#define UART_STATE_BITMAP_RX_CARRIER (1 << 0)
#define UART_STATE_BITMAP_TX_CARRIER (1 << 1)
#define UART_STATE_BITMAP_BREAK      (1 << 2)
#define UART_STATE_BITMAP_RINGSIGNAL (1 << 3)
#define UART_STATE_BITMAP_FRAMING    (1 << 4)
#define UART_STATE_BITMAP_PARITY     (1 << 5)
#define UART_STATE_BITMAP_OVERRUN    (1 << 6)

static int
handle_intr (struct USART *USARTx, struct rb *rb2a, struct usart_stat *stat)
{
  int tx_ready = 0;
  uint32_t r = USARTx->ISR;
  int notify_bits = 0;
  int smartcard_mode = ((USARTx->CR3 & USART_CR3_SCEN) != 0);

  if (smartcard_mode)
    {
      if ((r & USART_ISR_TC))
	{
	  tx_ready = 1;
	  USARTx->CR1 &= ~USART_CR1_TCIE;
	}
    }
  else
    {
      if ((r & USART_ISR_TXE))
	{
	  tx_ready = 1;
	  USARTx->CR1 &= ~USART_CR1_TXEIE;
	}
    }

  if ((r & USART_ISR_RXNE))
    {
      uint32_t data = USARTx->RDR;

      /* RDR register should be accessed even if data is not used.
       * Its read-access has side effect of clearing error flags.
       */
      asm volatile ("" : : "r" (data) : "memory");

      if ((r & USART_ISR_NE))
	stat->err_rx_noise++;
      else if ((r & USART_ISR_FE))
	{
	  /* NOTE: Noway to distinguish framing error and break  */

	  stat->rx_break++;
	  notify_bits |= UART_STATE_BITMAP_BREAK;
	}
      else if ((r & USART_ISR_PE))
	{
	  stat->err_rx_parity++;
	  notify_bits |= UART_STATE_BITMAP_PARITY;
	}
      else
	{
	  if ((r & USART_ISR_ORE))
	    {
	      stat->err_rx_overrun++;
	      notify_bits |= UART_STATE_BITMAP_OVERRUN;
	    }

	  /* XXX: if CS is 7-bit, mask it, or else parity bit in upper layer */
	  if (rb_ll_put (rb2a, (data & 0xff)) < 0)
	    stat->err_rx_overflow++;
	  else
	    stat->rx++;
	}
    }
  else if ((r & USART_ISR_ORE))
    {				/* Clear ORE */
      uint32_t data = USARTx->RDR;
      asm volatile ("" : : "r" (data) : "memory");
      stat->err_rx_overrun++;
      notify_bits |= UART_STATE_BITMAP_OVERRUN;
    }

  if (notify_bits)
    {
      if ((*ss_notify_callback) (stat->dev_no, notify_bits))
	stat->err_notify_overflow++;
    }

  return tx_ready;
}

static int
handle_tx (struct USART *USARTx, struct rb *rb2h, struct usart_stat *stat)
{
  int tx_ready = 1;
  int c = rb_ll_get (rb2h);

  if (c >= 0)
    {
      uint32_t r;
      int smartcard_mode = ((USARTx->CR3 & USART_CR3_SCEN) != 0);

      USARTx->TDR = (c & 0xff);
      stat->tx++;
      r = USARTx->ISR;
      if (smartcard_mode)
	{
	  if ((r & USART_ISR_TC) == 0)
	    {
	      tx_ready = 0;
	      USARTx->CR1 |= USART_CR1_TCIE;
	    }
	}
      else
	{
	  if ((r & USART_ISR_TXE) == 0)
	    {
	      tx_ready = 0;
	      USARTx->CR1 |= USART_CR1_TXEIE;
	    }
	}
    }

  return tx_ready;
}

int
usart_send_break (uint8_t dev_no)
{
  struct USART *USARTx = get_usart_dev (dev_no);
  if (USARTx == NULL)
    return -1;

  USARTx->RQR |= 0x02;
  return 0;
}

int
usart_block_sendrecv (uint8_t dev_no, const char *s_buf, uint16_t s_buflen,
		      char *r_buf, uint16_t r_buflen,
		      uint32_t *timeout_block_p, uint32_t timeout_char)
{
  /* TBD */
  (void)dev_no;
  (void)s_buf;
  (void)s_buflen;
  (void)r_buf;
  (void)r_buflen;
  (void)timeout_block_p;
  (void)timeout_char;
  return 0;
}
