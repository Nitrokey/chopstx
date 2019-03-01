#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <chopstx.h>
#include <contrib/usart.h>

#include <usb_lld.h>
#include "cdc.h"

/* For set_led */
#include "board.h"
#include "sys.h"

static void *
blk (void *arg)
{
  (void)arg;

  while (1)
    {
      set_led (0);
      chopstx_usec_wait (200*1000);
      set_led (1);
      chopstx_usec_wait (200*1000);
    }

  return NULL;
}

#define PRIO_USART     4
#define PRIO_CDC2USART 3
#define PRIO_USART2CDC 3
#define PRIO_CDC       2

#define STACK_MAIN
#define STACK_PROCESS_1
#define STACK_PROCESS_2
#define STACK_PROCESS_3
#define STACK_PROCESS_4
#define STACK_PROCESS_5
#define STACK_PROCESS_6
#include "stack-def.h"
#define STACK_ADDR_CDC ((uintptr_t)process1_base)
#define STACK_SIZE_CDC (sizeof process1_base)

#define STACK_ADDR_USART ((uint32_t)process2_base)
#define STACK_SIZE_USART (sizeof process2_base)

#define STACK_ADDR_CDC2USART0 ((uint32_t)process3_base)
#define STACK_SIZE_CDC2USART0 (sizeof process3_base)

#define STACK_ADDR_USART2CDC0 ((uint32_t)process4_base)
#define STACK_SIZE_USART2CDC0 (sizeof process4_base)

#define STACK_ADDR_CDC2USART1 ((uint32_t)process5_base)
#define STACK_SIZE_CDC2USART1 (sizeof process5_base)

#define STACK_ADDR_USART2CDC1 ((uint32_t)process6_base)
#define STACK_SIZE_USART2CDC1 (sizeof process6_base)


struct cdc_usart {
  uint8_t dev_no;
  struct cdc *cdc;
};

static void *
usart_to_cdc_loop (void *arg)
{
  struct cdc_usart *cdc_usart = arg;

  while (1)
    {
      char s[BUFSIZE];

      cdc_wait_connection (cdc_usart->cdc);

      /* Flush USART buffers */
      usart_read (cdc_usart->dev_no, NULL, 0);
      usart_write (cdc_usart->dev_no, NULL, 0);

      chopstx_usec_wait (100*1000);

      while (1)
	{
	  int size = usart_read (cdc_usart->dev_no, s, BUFSIZE);

	  if (size > 0)
	    {
	      if (cdc_send (cdc_usart->cdc, s, size) < 0)
		break;
	    }
	}
    }

  return NULL;
}

static void *
cdc_to_usart_loop (void *arg)
{
  struct cdc_usart *cdc_usart = arg;

  while (1)
    {
      char s[BUFSIZE];

      cdc_wait_connection (cdc_usart->cdc);

      /* Flush USART buffers */
      usart_read (cdc_usart->dev_no, NULL, 0);
      usart_write (cdc_usart->dev_no, NULL, 0);

      chopstx_usec_wait (50*1000);

      /* Send ZLP at the beginning.  */
      cdc_send (cdc_usart->cdc, s, 0);

      while (1)
	{
	  int size;
	  uint32_t usec = 3000000;	/* 3.0 seconds */

	  size = cdc_recv (cdc_usart->cdc, s, &usec);
	  if (size < 0)
	    break;

	  if (size)
	    usart_write (cdc_usart->dev_no, s, size);
	}
    }

  return NULL;
}

static struct cdc_usart cdc_usart0;
static struct cdc_usart cdc_usart1;

static int
ss_notify (uint8_t dev_no, uint16_t state_bits)
{
  struct cdc *s;

  if (dev_no == cdc_usart0.dev_no)
    s = cdc_usart0.cdc;
  else if (dev_no == cdc_usart1.dev_no)
    s = cdc_usart1.cdc;
  else
    return -1;

  return cdc_ss_notify (s, state_bits);
}

static void
send_break (uint8_t dev_no, uint16_t duration)
{
  (void)duration; 		/* Not supported by USART.  */
  usart_send_break (dev_no);
}

static void
setup_usart_config (uint8_t dev_no, uint32_t bitrate, uint8_t format,
		    uint8_t paritytype, uint8_t databits)
{
  /* Check supported config(s) */
  uint32_t config_bits;

  if (bitrate == 9600)
    config_bits = B9600;
  else if (bitrate == 19200)
    config_bits = B19200;
  else if (bitrate == 57600)
    config_bits = B57600;
  else if (bitrate == 115200)
    config_bits = B115200;
  else
    {
      bitrate = 115200;
      config_bits = B115200;
    }

  if (format == 0)
    config_bits |= STOP1B;
  else if (format == 1)
    config_bits |= STOP1B5;
  else if (format == 2)
    config_bits |= STOP2B;
  else
    {
      format = 0;
      config_bits |= STOP1B;
    }

  if (paritytype == 0)
    config_bits |= 0;
  else if (paritytype == 1)
    config_bits |= (PARENB | PARODD);
  else if (paritytype == 2)
    config_bits |= PARENB;
  else
    {
      paritytype = 0;
      config_bits |= 0;
    }

  if (databits == 7)
    config_bits |= CS7;
  else if (databits == 7)
    config_bits |= CS8;
  else
    {
      databits = 8;
      config_bits |= CS8;
    }

  if (databits == 7 && paritytype == 0)
    {
      databits = 8;
      config_bits &= ~MASK_CS;
      config_bits |= CS8;
    }

  usart_config (dev_no, config_bits);
}


int
main (int argc, const char *argv[])
{
  (void)argc;
  (void)argv;

  chopstx_usec_wait (200*1000);

  cdc_init (PRIO_CDC, STACK_ADDR_CDC, STACK_SIZE_CDC,
	    send_break, setup_usart_config);
  cdc_wait_configured ();

  usart_init (PRIO_USART, STACK_ADDR_USART, STACK_SIZE_USART, ss_notify);

  usart_config (2, B115200 | CS8 | STOP1B);
  usart_config (3, B115200 | CS8 | STOP1B);

  cdc_usart0.cdc = cdc_open (0);
  cdc_usart0.dev_no = 2;
  cdc_usart1.cdc = cdc_open (1);
  cdc_usart1.dev_no = 3;

  chopstx_create (PRIO_USART2CDC, STACK_ADDR_USART2CDC0,
		  STACK_SIZE_USART2CDC0, usart_to_cdc_loop, &cdc_usart0);
  chopstx_create (PRIO_USART2CDC, STACK_ADDR_USART2CDC1,
		  STACK_SIZE_USART2CDC1, usart_to_cdc_loop, &cdc_usart1);
  chopstx_create (PRIO_CDC2USART, STACK_ADDR_CDC2USART0,
		  STACK_SIZE_CDC2USART0, cdc_to_usart_loop, &cdc_usart0);
  chopstx_create (PRIO_CDC2USART, STACK_ADDR_CDC2USART1,
		  STACK_SIZE_CDC2USART1, cdc_to_usart_loop, &cdc_usart1);

  blk (NULL);
  return 0;
}
