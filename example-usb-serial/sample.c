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

#define STACK_MAIN
#define STACK_PROCESS_2
#define STACK_PROCESS_3
#define STACK_PROCESS_4
#define STACK_PROCESS_5
#define STACK_PROCESS_6
#include "stack-def.h"
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


int
main (int argc, const char *argv[])
{
  struct cdc_usart cdc_usart0;
  struct cdc_usart cdc_usart1;

  (void)argc;
  (void)argv;

  chopstx_usec_wait (200*1000);

  cdc_init ();
  cdc_wait_configured ();

  usart_init (PRIO_USART, STACK_ADDR_USART, STACK_SIZE_USART);

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
