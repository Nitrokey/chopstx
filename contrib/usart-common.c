static void *usart_main (void *arg);

/*
 * Ring buffer
 */
#define MAX_RB_BUF 1024

struct rb {
  uint8_t *buf;
  chopstx_mutex_t m;
  chopstx_cond_t data_available;
  chopstx_cond_t space_available;
  uint32_t head  :10;
  uint32_t tail  :10;
  uint32_t size  :10;
  uint32_t full  : 1;
  uint32_t empty : 1;
};
/* full && empty -> data is consumed fully */

/*
 * Note: size = 1024 can still work, regardless of the limit of 10-bit.
 */
static void
rb_init (struct rb *rb, uint8_t *p, uint16_t size)
{
  rb->buf = p;
  rb->size = size;
  chopstx_mutex_init (&rb->m);
  chopstx_cond_init (&rb->data_available);
  chopstx_cond_init (&rb->space_available);
  rb->head = rb->tail = 0;
  rb->full = 0;
  rb->empty = 1;
}

static void
rb_add (struct rb *rb, uint8_t v)
{
  rb->buf[rb->tail++] = v;
  if (rb->tail == rb->size)
    rb->tail = 0;
  if (rb->tail == rb->head)
    rb->full = 1;
  else
    rb->full = 0;
  rb->empty = 0;
}

static uint8_t
rb_del (struct rb *rb)
{
  uint32_t v = rb->buf[rb->head++];

  if (rb->head == rb->size)
    rb->head = 0;
  if (rb->head == rb->tail)
    rb->empty = 1;
  rb->full = 0;

  return v;
}

/*
 * Application: consumer
 * Hardware:    generator
 */
static int
rb_ll_put (struct rb *rb, uint8_t v)
{
  int r;

  chopstx_mutex_lock (&rb->m);
  if (rb->full && !rb->empty)
    r = -1;
  else
    {
      r = 0;
      rb_add (rb, v);
      chopstx_cond_signal (&rb->data_available);
    }
  chopstx_mutex_unlock (&rb->m);
  return r;
}

/*
 * Application: generator
 * Hardware:    consumer
 */
static int
rb_ll_get (struct rb *rb)
{
  int r;

  chopstx_mutex_lock (&rb->m);
  if (rb->empty)
    {
      if (!rb->full)
	rb->full = 1;
      r = -1;
    }
  else
    r = rb_del (rb);
  chopstx_cond_signal (&rb->space_available);
  chopstx_mutex_unlock (&rb->m);
  return r;
}

static void
rb_ll_flush (struct rb *rb)
{
  chopstx_mutex_lock (&rb->m);
  while (!rb->empty)
    rb_del (rb);
  chopstx_cond_signal (&rb->space_available);
  chopstx_mutex_unlock (&rb->m);
}

/*
 * Application: consumer
 * Hardware:    generator
 */
static int
rb_read (struct rb *rb, uint8_t *buf, uint16_t buflen)
{
  int i = 0;

  chopstx_mutex_lock (&rb->m);
  while (rb->empty)
    chopstx_cond_wait (&rb->data_available, &rb->m);

  while (i < buflen)
    {
      buf[i++] = rb_del (rb);
      if (rb->empty)
	break;
    }
  chopstx_cond_signal (&rb->space_available);
  chopstx_mutex_unlock (&rb->m);

  return i;
}

/*
 * Application: generator
 * Hardware:    consumer
 */
static void
rb_write (struct rb *rb, uint8_t *buf, uint16_t buflen)
{
  int i = 0;

  chopstx_mutex_lock (&rb->m);

  do
    {
      while (rb->full && !rb->empty)
	chopstx_cond_wait (&rb->space_available, &rb->m);

      while (i < buflen)
	{
	  rb_add (rb, buf[i++]);
	  if (rb->full)
	    {
	      chopstx_cond_signal (&rb->data_available);
	      break;
	    }
	}
    }
  while (i < buflen);

  if (i)
    chopstx_cond_signal (&rb->data_available);
  chopstx_mutex_unlock (&rb->m);
}

static int
rb_empty_check (void *arg)
{
  struct rb *rb = arg;
  return rb->empty == 0;
}

/* Can be used two ways:
 *
 *   When the ring buffer is rb_a2h:
 *     Hardware-side polling if data is available from application.
 *
 *   When the ring buffer is rb_h2a:
 *     Application-side polling if data is available from hardware.
 */
static void
rb_get_prepare_poll (struct rb *rb, chopstx_poll_cond_t *poll_desc)
{
  poll_desc->type  = CHOPSTX_POLL_COND;
  poll_desc->ready = 0;
  poll_desc->cond  = &rb->data_available;
  poll_desc->mutex = &rb->m;
  poll_desc->check = rb_empty_check;
  poll_desc->arg   = rb;
}

const struct usart_stat *
usart_stat (uint8_t dev_no)
{
  if (dev_no < USART_DEVNO_START || dev_no > USART_DEVNO_END)
    return NULL;
  return usart_array[dev_no - USART_DEVNO_START].stat;
}

static struct USART *
get_usart_dev (uint8_t dev_no)
{
  if (dev_no < USART_DEVNO_START || dev_no > USART_DEVNO_END)
    return NULL;
  return usart_array[dev_no - USART_DEVNO_START].USART;
}

static struct rb *
get_usart_rb_h2a (uint8_t dev_no)
{
  if (dev_no < USART_DEVNO_START || dev_no > USART_DEVNO_END)
    return NULL;
  return usart_array[dev_no - USART_DEVNO_START].rb_h2a;
}

static struct rb *
get_usart_rb_a2h (uint8_t dev_no)
{
  if (dev_no < USART_DEVNO_START || dev_no > USART_DEVNO_END)
    return NULL;
  return usart_array[dev_no - USART_DEVNO_START].rb_a2h;
}

static struct chx_intr *
get_usart_intr (uint8_t dev_no)
{
  if (dev_no < USART_DEVNO_START || dev_no > USART_DEVNO_END)
    return NULL;
  return usart_array[dev_no - USART_DEVNO_START].intr;
}

void
usart_init (uint16_t prio, uintptr_t stack_addr, size_t stack_size,
	    int (*cb) (uint8_t dev_no, uint16_t notify_bits))
{
  usart_init0 (cb);
  chopstx_create (prio, stack_addr, stack_size, usart_main, NULL);
}

struct brr_setting {
  uint8_t baud_spec;
  uint16_t brr_value;
};
#define NUM_BAUD (int)(sizeof (brr_table) / sizeof (struct brr_setting))

static int (*ss_notify_callback) (uint8_t dev_no, uint16_t notify_bits);

static struct chx_poll_head *usart_poll[NUM_USART*2];

static void *
usart_main (void *arg)
{
  int i;

  (void)arg;

  for (i = 0; i < NUM_USART; i++)
    {
      *usart_array[i].tx_ready = 1;
      rb_init (usart_array[i].rb_a2h, usart_array[i].buf_a2h, BUF_A2H_SIZE);
      rb_init (usart_array[i].rb_h2a, usart_array[i].buf_h2a, BUF_H2A_SIZE);
      rb_get_prepare_poll (usart_array[i].rb_a2h, usart_array[i].app_write_event);
    }

  while (1)
    {
      int n = 0;

      for (i = 0; i < NUM_USART; i++)
	{
	  usart_poll[n++] = (struct chx_poll_head *)usart_array[i].intr;
	  if (*usart_array[i].tx_ready)
	    usart_poll[n++] = (struct chx_poll_head *)usart_array[i].app_write_event;
	  else
	    usart_array[i].app_write_event->ready = 0;
	}

      chopstx_poll (NULL, n, usart_poll);

      for (i = 0; i < NUM_USART; i++)
	{
	  int tx_done = 0;

	  if (usart_array[i].intr->ready)
	    {
	      tx_done = handle_intr (usart_array[i].USART,
				     usart_array[i].rb_h2a, usart_array[i].stat);
	      *usart_array[i].tx_ready |= tx_done;
	      chopstx_intr_done (usart_array[i].intr);
	    }

	  if (tx_done || (*usart_array[i].tx_ready
			  && usart_array[i].app_write_event->ready))
	    *usart_array[i].tx_ready = handle_tx (usart_array[i].USART,
						  usart_array[i].rb_a2h, usart_array[i].stat);
	}
    }

  return NULL;
}

int
usart_read (uint8_t dev_no, char *buf, uint16_t buflen)
{
  struct rb *rb = get_usart_rb_h2a (dev_no);

  if (rb == NULL)
    return -1;

  if (buf == NULL && buflen == 0)
    {
      rb_ll_flush (rb);
      return 0;
    }
  else
    return rb_read (rb, (uint8_t *)buf, buflen);
}

void
usart_read_prepare_poll (uint8_t dev_no, chopstx_poll_cond_t *poll_desc)
{
  struct rb *rb = get_usart_rb_h2a (dev_no);

  if (rb == NULL)
    return;

  rb_get_prepare_poll (rb, poll_desc);
}

int
usart_read_ext (uint8_t dev_no, char *buf, uint16_t buflen, uint32_t *timeout_p)
{
  chopstx_poll_cond_t poll_desc;
  struct chx_poll_head *ph[] = { (struct chx_poll_head *)&poll_desc };
  int r;
  struct rb *rb = get_usart_rb_h2a (dev_no);

  if (rb == NULL)
    return -1;

  rb_get_prepare_poll (rb, &poll_desc);
  r = chopstx_poll (timeout_p, 1, ph);
  if (r == 0)
    return 0;
  else
    return rb_read (rb, (uint8_t *)buf, buflen);
}

static void
usart_wait_write_completion (struct rb *rb)
{
  chopstx_mutex_lock (&rb->m);
  while (!(rb->empty && rb->full))
    chopstx_cond_wait (&rb->space_available, &rb->m);
  chopstx_mutex_unlock (&rb->m);
}

int
usart_write (uint8_t dev_no, char *buf, uint16_t buflen)
{
  struct rb *rb = get_usart_rb_a2h (dev_no);

  if (rb == NULL)
    return -1;

  if (buf == NULL && buflen == 0)
    rb_ll_flush (rb);
  else
    {
      struct USART *USARTx = get_usart_dev (dev_no);
      int smartcard_mode = ((USARTx->CR3 & USART_CR3_SCEN) != 0);

      if (smartcard_mode)
	usart_config_recv_enable (USARTx, 0);

      rb_write (rb, (uint8_t *)buf, buflen);

      if (smartcard_mode)
	{
	  usart_wait_write_completion (rb);
	  usart_config_recv_enable (USARTx, 1);
	}
    }

  return 0;
}
