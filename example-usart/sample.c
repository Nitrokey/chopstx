#include <stdint.h>
#include <stdlib.h>
#include <chopstx.h>

#include "board.h"
#include "sys.h" /* for set_led */
#include <contrib/usart.h>

static chopstx_mutex_t mtx;
static chopstx_cond_t cnd0;
static chopstx_cond_t cnd1;

static uint8_t u, v;
static uint8_t m;		/* 0..100 */

static void
wait_for (uint32_t usec)
{
#if defined(BUSY_LOOP)
  uint32_t count = usec * 6;
  uint32_t i;

  for (i = 0; i < count; i++)
    asm volatile ("" : : "r" (i) : "memory");
#else
  chopstx_usec_wait (usec);
#endif
}

static void *
pwm (void *arg)
{
  (void)arg;

  chopstx_mutex_lock (&mtx);
  chopstx_cond_wait (&cnd0, &mtx);
  chopstx_mutex_unlock (&mtx);

  while (1)
    {
      set_led (u&v);
      wait_for (m);
      set_led (0);
      wait_for (100-m);
    }

  return NULL;
}

static void *
blk (void *arg)
{
  (void)arg;

  chopstx_mutex_lock (&mtx);
  chopstx_cond_wait (&cnd1, &mtx);
  chopstx_mutex_unlock (&mtx);

  while (1)
    {
      v = 0;
      wait_for (200*1000);
      v = 1;
      wait_for (200*1000);
    }

  return NULL;
}

#if defined(BUSY_LOOP)
#define PRIO_PWM (CHOPSTX_SCHED_RR|1)
#define PRIO_BLK (CHOPSTX_SCHED_RR|1)
#else
#define PRIO_PWM 3
#define PRIO_BLK 2
#endif

#define STACK_MAIN
#define STACK_PROCESS_1
#define STACK_PROCESS_2
#define STACK_PROCESS_3
#include "stack-def.h"


#define STACK_ADDR_PWM ((uint32_t)process1_base)
#define STACK_SIZE_PWM (sizeof process1_base)

#define STACK_ADDR_BLK ((uint32_t)process2_base)
#define STACK_SIZE_BLK (sizeof process2_base)

#define PRIO_USART     4
#define STACK_ADDR_USART ((uint32_t)process3_base)
#define STACK_SIZE_USART (sizeof process3_base)

static int
ss_notify (uint8_t dev_no, uint16_t state_bits)
{
  (void)dev_no;
  (void)state_bits;
  return 0;
}

int
main (int argc, const char *argv[])
{
  chopstx_poll_cond_t poll_desc;
  uint32_t timeout;
  struct chx_poll_head *ph[1];

  (void)argc;
  (void)argv;

  chopstx_mutex_init (&mtx);
  chopstx_cond_init (&cnd0);
  chopstx_cond_init (&cnd1);

  m = 10;

  chopstx_create (PRIO_PWM, STACK_ADDR_PWM, STACK_SIZE_PWM, pwm, NULL);
  chopstx_create (PRIO_BLK, STACK_ADDR_BLK, STACK_SIZE_BLK, blk, NULL);

  chopstx_usec_wait (200*1000);

  chopstx_mutex_lock (&mtx);
  chopstx_cond_signal (&cnd0);
  chopstx_cond_signal (&cnd1);
  chopstx_mutex_unlock (&mtx);

  usart_init (PRIO_USART, STACK_ADDR_USART, STACK_SIZE_USART, ss_notify);
  usart_config (2, B115200 | CS8 | STOP1B);

  usart_read_prepare_poll (2, &poll_desc);
  ph[0] = (struct chx_poll_head *)&poll_desc;

  timeout = 200*1000*6;
  while (1)
    {
      chopstx_poll (&timeout, 1, ph);
      if (timeout == 0)
	{
	  usart_write (2, "Hello\r\n", 7);
	  u ^= 1;
	  timeout = 200*1000*6;
	}
      else
	{
	  char buf[256];
	  int r;
	  r = usart_read (2, buf, 256);
	  if (r)
	    usart_write (2, buf, r);
	}
    }

  return 0;
}
