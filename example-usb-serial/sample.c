#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <chopstx.h>

#include "usb_lld.h"
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


#define PRIO_BLK 2
#define PRIO_CDC 3

#define STACK_MAIN
#define STACK_PROCESS_1
#define STACK_PROCESS_2
#include "stack-def.h"
#define STACK_ADDR_BLK ((uint32_t)process1_base)
#define STACK_SIZE_BLK (sizeof process1_base)

#define STACK_ADDR_CDC ((uint32_t)process2_base)
#define STACK_SIZE_CDC (sizeof process2_base)


static void *
cdc_loop (void *arg)
{
  struct cdc **cdc_array;
  struct cdc *cdc_another;
  struct cdc *cdc;

  cdc_array = arg;
  cdc = cdc_array[0];
  cdc_another = cdc_array[1];

  while (1)
    {
      char s[BUFSIZE];

      cdc_wait_connection (cdc);

      chopstx_usec_wait (50*1000);

      /* Send ZLP at the beginning.  */
      cdc_send (cdc, s, 0);

      while (1)
	{
	  int size;
	  uint32_t usec = 3000000;	/* 3.0 seconds */

	  size = cdc_recv (cdc_another, s, &usec);
	  if (size < 0)
	    break;

	  if (size)
	    {
	      if (cdc_send (cdc, s, size) < 0)
		break;
	    }
	  else
	    {
	      if (cdc_send (cdc, "HELLO!\r\n", 8) < 0)
		break;
	    }
	}
    }

  return NULL;
}

int
main (int argc, const char *argv[])
{
  struct cdc *cdc_array[2];
  struct cdc *cdc_array_dash[2];

  (void)argc;
  (void)argv;

  chopstx_create (PRIO_BLK, STACK_ADDR_BLK, STACK_SIZE_BLK, blk, NULL);

  chopstx_usec_wait (200*1000);

  cdc_init ();
  cdc_wait_configured ();

  cdc_array[0] = cdc_array_dash[1] = cdc_open (0);
  cdc_array[1] = cdc_array_dash[0] = cdc_open (1);

  chopstx_create (PRIO_CDC, STACK_ADDR_CDC, STACK_SIZE_CDC,
		  cdc_loop, cdc_array);
  cdc_loop (cdc_array_dash);

  return 0;
}
