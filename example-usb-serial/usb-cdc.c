#include <stdint.h>
#include <stdlib.h>
#include <chopstx.h>
#include <string.h>
#include <usb_lld.h>
#include "cdc.h"

static chopstx_intr_t usb_intr;

struct line_coding
{
  uint32_t bitrate;
  uint8_t format;
  uint8_t paritytype;
  uint8_t databits;
} __attribute__((packed));

static const struct line_coding lc_default = {
  115200, /* baud rate: 115200    */
  0x00,   /* stop bits: 1         */
  0x00,   /* parity:    none      */
  0x08    /* bits:      8         */
};

static uint8_t device_state;    /* USB device status */

struct cdc {
  uint8_t dev_no;

  uint8_t endp1;
  uint8_t endp2;
  uint8_t endp3;

  chopstx_mutex_t mtx;
  chopstx_cond_t cnd_rx;
  chopstx_cond_t cnd_tx;
  uint8_t input[BUFSIZE];
  uint32_t input_len        : 7;
  uint32_t flag_connected   : 1;
  uint32_t flag_output_ready: 1;
  uint32_t flag_input_avail : 1;
  uint32_t flag_notify_busy : 1;
  uint32_t                  :21;
  struct line_coding line_coding;
};

#define MAX_CDC 2
static struct cdc cdc_table[MAX_CDC];


/*
 * Locate CDC structure from interface number or endpoint number.
 */
static struct cdc *
cdc_get (int interface, uint8_t ep_num)
{
  struct cdc *s;

  if (interface >= 0)
    {
      if (interface == 0 || interface == 1)
	s = &cdc_table[0];
      else
	s = &cdc_table[1];
    }
  else
    {
      if (ep_num == ENDP1 || ep_num == ENDP2 || ep_num == ENDP3)
	s = &cdc_table[0];
      else
	s = &cdc_table[1];
    }

  return s;
}


#define ENDP0_RXADDR        (0x40)
#define ENDP0_TXADDR        (0x80)
#define ENDP1_TXADDR        (0xc0)
#define ENDP2_TXADDR        (0x100)
#define ENDP3_RXADDR        (0x10A)
#define ENDP4_TXADDR        (0x14A)
#define ENDP5_TXADDR        (0x18A)
#define ENDP6_RXADDR        (0x194)
/* 0x1d4 = 468, 44-byte available */

#define USB_CDC_REQ_SET_LINE_CODING             0x20
#define USB_CDC_REQ_GET_LINE_CODING             0x21
#define USB_CDC_REQ_SET_CONTROL_LINE_STATE      0x22
#define USB_CDC_REQ_SEND_BREAK                  0x23

#define USB_CDC_NOTIFY_SERIAL_STATE		0x20

/* USB Device Descriptor */
static const uint8_t vcom_device_desc[18] = {
  18,   /* bLength */
  DEVICE_DESCRIPTOR,		/* bDescriptorType */
  0x10, 0x01,			/* bcdUSB = 1.1 */
  0x02,				/* bDeviceClass (CDC).              */
  0x00,				/* bDeviceSubClass.                 */
  0x00,				/* bDeviceProtocol.                 */
  0x40,				/* bMaxPacketSize.                  */
  0xFF, 0xFF, /* idVendor  */
  0x01, 0x00, /* idProduct */
  0x00, 0x01, /* bcdDevice  */
  1,				/* iManufacturer.                   */
  2,				/* iProduct.                        */
  3,				/* iSerialNumber.                   */
  1				/* bNumConfigurations.              */
};

#define VCOM_FEATURE_BUS_POWERED	0x80

/* Configuration Descriptor tree for a CDC.*/
static const uint8_t vcom_config_desc[] = {
  9,
  CONFIG_DESCRIPTOR,		/* bDescriptorType: Configuration */
  /* Configuration Descriptor.*/
  58*2+9, 0x00,			/* wTotalLength.                    */
  2*2,				/* bNumInterfaces.                  */
  1,				/* bConfigurationValue.             */
  0,				/* iConfiguration.                  */
  VCOM_FEATURE_BUS_POWERED,	/* bmAttributes.                    */
  50,				/* bMaxPower (100mA).               */
  /* Interface Descriptor.*/
  9,
  INTERFACE_DESCRIPTOR,
  0x00,		   /* bInterfaceNumber.                */
  0x00,		   /* bAlternateSetting.               */
  0x01,		   /* bNumEndpoints.                   */
  0x02,		   /* bInterfaceClass (Communications Interface Class,
		      CDC section 4.2).  */
  0x02,		   /* bInterfaceSubClass (Abstract Control Model, CDC
		      section 4.3).  */
  0x01,		   /* bInterfaceProtocol (AT commands, CDC section
		      4.4).  */
  0,	           /* iInterface.                      */
  /* Header Functional Descriptor (CDC section 5.2.3).*/
  5,	      /* bLength.                         */
  0x24,	      /* bDescriptorType (CS_INTERFACE).  */
  0x00,	      /* bDescriptorSubtype (Header Functional Descriptor). */
  0x10, 0x01, /* bcdCDC.                          */
  /* Call Management Functional Descriptor. */
  5,            /* bFunctionLength.                 */
  0x24,         /* bDescriptorType (CS_INTERFACE).  */
  0x01,         /* bDescriptorSubtype (Call Management Functional
		   Descriptor). */
  0x03,         /* bmCapabilities (D0+D1).          */
  0x01,         /* bDataInterface.                  */
  /* ACM Functional Descriptor.*/
  4,            /* bFunctionLength.                 */
  0x24,         /* bDescriptorType (CS_INTERFACE).  */
  0x02,         /* bDescriptorSubtype (Abstract Control Management
		   Descriptor).  */
  0x02,         /* bmCapabilities.                  */
  /* Union Functional Descriptor.*/
  5,            /* bFunctionLength.                 */
  0x24,         /* bDescriptorType (CS_INTERFACE).  */
  0x06,         /* bDescriptorSubtype (Union Functional
		   Descriptor).  */
  0x00,         /* bMasterInterface (Communication Class
		   Interface).  */
  0x01,         /* bSlaveInterface0 (Data Class Interface).  */
  /* Endpoint 2 Descriptor.*/
  7,
  ENDPOINT_DESCRIPTOR,
  ENDP2|0x80,    /* bEndpointAddress.    */
  0x03,          /* bmAttributes (Interrupt).        */
  0x0A, 0x00,	 /* wMaxPacketSize.                  */
  0xFF,		 /* bInterval.                       */
  /* Interface Descriptor.*/
  9,
  INTERFACE_DESCRIPTOR, /* bDescriptorType: */
  0x01,          /* bInterfaceNumber.                */
  0x00,          /* bAlternateSetting.               */
  0x02,          /* bNumEndpoints.                   */
  0x0A,          /* bInterfaceClass (Data Class Interface, CDC section 4.5). */
  0x00,          /* bInterfaceSubClass (CDC section 4.6). */
  0x00,          /* bInterfaceProtocol (CDC section 4.7). */
  0x00,		 /* iInterface.                      */
  /* Endpoint 3 Descriptor.*/
  7,
  ENDPOINT_DESCRIPTOR,		/* bDescriptorType: Endpoint */
  ENDP3,    /* bEndpointAddress. */
  0x02,				/* bmAttributes (Bulk).             */
  0x40, 0x00,			/* wMaxPacketSize.                  */
  0x00,				/* bInterval.                       */
  /* Endpoint 1 Descriptor.*/
  7,
  ENDPOINT_DESCRIPTOR,		/* bDescriptorType: Endpoint */
  ENDP1|0x80,			/* bEndpointAddress. */
  0x02,				/* bmAttributes (Bulk).             */
  0x40, 0x00,			/* wMaxPacketSize.                  */
  0x00,				/* bInterval.                       */

  /******************************************************************/

  /* Interface Descriptor (second). */
  9,
  INTERFACE_DESCRIPTOR,
  0x02,		   /* bInterfaceNumber.                */
  0x00,		   /* bAlternateSetting.               */
  0x01,		   /* bNumEndpoints.                   */
  0x02,		   /* bInterfaceClass (Communications Interface Class,
		      CDC section 4.2).  */
  0x02,		   /* bInterfaceSubClass (Abstract Control Model, CDC
		      section 4.3).  */
  0x01,		   /* bInterfaceProtocol (AT commands, CDC section
		      4.4).  */
  0,	           /* iInterface.                      */
  /* Header Functional Descriptor (CDC section 5.2.3).*/
  5,	      /* bLength.                         */
  0x24,	      /* bDescriptorType (CS_INTERFACE).  */
  0x00,	      /* bDescriptorSubtype (Header Functional Descriptor). */
  0x10, 0x01, /* bcdCDC.                          */
  /* Call Management Functional Descriptor. */
  5,            /* bFunctionLength.                 */
  0x24,         /* bDescriptorType (CS_INTERFACE).  */
  0x01,         /* bDescriptorSubtype (Call Management Functional
		   Descriptor). */
  0x03,         /* bmCapabilities (D0+D1).          */
  0x03,         /* bDataInterface.                  */
  /* ACM Functional Descriptor.*/
  4,            /* bFunctionLength.                 */
  0x24,         /* bDescriptorType (CS_INTERFACE).  */
  0x02,         /* bDescriptorSubtype (Abstract Control Management
		   Descriptor).  */
  0x02,         /* bmCapabilities.                  */
  /* Union Functional Descriptor.*/
  5,            /* bFunctionLength.                 */
  0x24,         /* bDescriptorType (CS_INTERFACE).  */
  0x06,         /* bDescriptorSubtype (Union Functional
		   Descriptor).  */
  0x02,         /* bMasterInterface (Communication Class
		   Interface).  */
  0x03,         /* bSlaveInterface0 (Data Class Interface).  */
  /* Endpoint 2 Descriptor.*/
  7,
  ENDPOINT_DESCRIPTOR,
  ENDP5|0x80,    /* bEndpointAddress.    */
  0x03,          /* bmAttributes (Interrupt).        */
  0x0A, 0x00,	 /* wMaxPacketSize.                  */
  0xFF,		 /* bInterval.                       */
  /* Interface Descriptor.*/
  9,
  INTERFACE_DESCRIPTOR, /* bDescriptorType: */
  0x03,          /* bInterfaceNumber.                */
  0x00,          /* bAlternateSetting.               */
  0x02,          /* bNumEndpoints.                   */
  0x0A,          /* bInterfaceClass (Data Class Interface, CDC section 4.5). */
  0x00,          /* bInterfaceSubClass (CDC section 4.6). */
  0x00,          /* bInterfaceProtocol (CDC section 4.7). */
  0x00,		 /* iInterface.                      */
  /* Endpoint 3 Descriptor.*/
  7,
  ENDPOINT_DESCRIPTOR,		/* bDescriptorType: Endpoint */
  ENDP6,    /* bEndpointAddress. */
  0x02,				/* bmAttributes (Bulk).             */
  0x40, 0x00,			/* wMaxPacketSize.                  */
  0x00,				/* bInterval.                       */
  /* Endpoint 1 Descriptor.*/
  7,
  ENDPOINT_DESCRIPTOR,		/* bDescriptorType: Endpoint */
  ENDP4|0x80,			/* bEndpointAddress. */
  0x02,				/* bmAttributes (Bulk).             */
  0x40, 0x00,			/* wMaxPacketSize.                  */
  0x00				/* bInterval.                       */
};


/*
 * U.S. English language identifier.
 */
static const uint8_t vcom_string0[4] = {
  4,				/* bLength */
  STRING_DESCRIPTOR,
  0x09, 0x04			/* LangID = 0x0409: US-English */
};

static const uint8_t vcom_string1[] = {
  23*2+2,			/* bLength */
  STRING_DESCRIPTOR,		/* bDescriptorType */
  /* Manufacturer: "Flying Stone Technology" */
  'F', 0, 'l', 0, 'y', 0, 'i', 0, 'n', 0, 'g', 0, ' ', 0, 'S', 0,
  't', 0, 'o', 0, 'n', 0, 'e', 0, ' ', 0, 'T', 0, 'e', 0, 'c', 0,
  'h', 0, 'n', 0, 'o', 0, 'l', 0, 'o', 0, 'g', 0, 'y', 0, 
};

static const uint8_t vcom_string2[] = {
  14*2+2,			/* bLength */
  STRING_DESCRIPTOR,		/* bDescriptorType */
  /* Product name: "Chopstx Serial" */
  'C', 0, 'h', 0, 'o', 0, 'p', 0, 's', 0, 't', 0, 'x', 0, ' ', 0,
  'S', 0, 'e', 0, 'r', 0, 'i', 0, 'a', 0, 'l', 0,
};

/*
 * Serial Number string.
 */
static const uint8_t vcom_string3[28] = {
  28,				    /* bLength */
  STRING_DESCRIPTOR,		    /* bDescriptorType */
  '0', 0,  '.', 0,  '0', 0, '0', 0, /* Version number */
};


#define NUM_INTERFACES 4


static void
usb_device_reset (struct usb_dev *dev)
{
  int i;

  usb_lld_reset (dev, VCOM_FEATURE_BUS_POWERED);

  /* Initialize Endpoint 0 */
  usb_lld_setup_endpoint (ENDP0, EP_CONTROL, 0, ENDP0_RXADDR, ENDP0_TXADDR, 64);

  device_state = USB_DEVICE_STATE_ATTACHED;
  for (i = 0; i < MAX_CDC; i++)
    {
      struct cdc *s = &cdc_table[i];

      chopstx_mutex_lock (&s->mtx);
      s->input_len = 0;
      s->flag_connected = 0;
      s->flag_output_ready = 1;
      s->flag_input_avail = 0;
      memcpy (&s->line_coding, &lc_default, sizeof (struct line_coding));
      chopstx_mutex_unlock (&s->mtx);
    }
}


void (*send_break) (uint8_t dev_no, uint16_t duration);
static void (*setup_usart_config) (uint8_t dev_no, uint32_t bitrate,
				   uint8_t format, uint8_t paritytype,
				   uint8_t databits);

#define CDC_CTRL_DTR            0x0001

static void
usb_ctrl_write_finish (struct usb_dev *dev)
{
  struct device_req *arg = &dev->dev_req;
  uint8_t type_rcp = arg->type & (REQUEST_TYPE|RECIPIENT);

  if (type_rcp == (CLASS_REQUEST | INTERFACE_RECIPIENT)
      && USB_SETUP_SET (arg->type))
    {
      struct cdc *s = cdc_get (arg->index, 0);

      if (arg->request == USB_CDC_REQ_SET_LINE_CODING)
	(*setup_usart_config) (s->dev_no, s->line_coding.bitrate,
			       s->line_coding.format,
			       s->line_coding.paritytype,
			       s->line_coding.databits);
      else if (arg->request == USB_CDC_REQ_SET_CONTROL_LINE_STATE)
	{
	  /* Open/close the connection.  */
	  chopstx_mutex_lock (&s->mtx);
	  s->flag_connected = ((arg->value & CDC_CTRL_DTR) != 0);
	  chopstx_cond_broadcast (&s->cnd_rx);
	  chopstx_mutex_unlock (&s->mtx);
	}
      else if (arg->request == USB_CDC_REQ_SEND_BREAK)
	{
	  chopstx_mutex_lock (&s->mtx);
	  send_break (s->dev_no, arg->value);
	  chopstx_mutex_unlock (&s->mtx);
	}
    }
  /*
   * The transaction was already finished.  So, it is no use to call
   * usb_lld_ctrl_error when the condition does not match.
   */
}



static int
vcom_port_data_setup (struct usb_dev *dev)
{
  struct device_req *arg = &dev->dev_req;

  if (USB_SETUP_GET (arg->type))
    {
      struct cdc *s = cdc_get (arg->index, 0);

      if (arg->request == USB_CDC_REQ_GET_LINE_CODING)
	return usb_lld_ctrl_send (dev, &s->line_coding,
				  sizeof (struct line_coding));
    }
  else  /* USB_SETUP_SET (req) */
    {
      if (arg->request == USB_CDC_REQ_SET_LINE_CODING
	  && arg->len == sizeof (struct line_coding))
	{
	  struct cdc *s = cdc_get (arg->index, 0);

	  return usb_lld_ctrl_recv (dev, &s->line_coding,
				    sizeof (struct line_coding));
	}
      else if (arg->request == USB_CDC_REQ_SET_CONTROL_LINE_STATE)
	return usb_lld_ctrl_ack (dev);
      else if (arg->request == USB_CDC_REQ_SEND_BREAK)
	return usb_lld_ctrl_ack (dev);
    }

  return -1;
}

static int
usb_setup (struct usb_dev *dev)
{
  struct device_req *arg = &dev->dev_req;
  uint8_t type_rcp = arg->type & (REQUEST_TYPE|RECIPIENT);

  if (type_rcp == (CLASS_REQUEST | INTERFACE_RECIPIENT))
    return vcom_port_data_setup (dev);

  return -1;
}

static int
usb_get_descriptor (struct usb_dev *dev)
{
  struct device_req *arg = &dev->dev_req;
  uint8_t rcp = arg->type & RECIPIENT;
  uint8_t desc_type = (arg->value >> 8);
  uint8_t desc_index = (arg->value & 0xff);

  if (rcp != DEVICE_RECIPIENT)
    return -1;

  if (desc_type == DEVICE_DESCRIPTOR)
    return usb_lld_ctrl_send (dev,
			      vcom_device_desc, sizeof (vcom_device_desc));
  else if (desc_type == CONFIG_DESCRIPTOR)
    return usb_lld_ctrl_send (dev,
			      vcom_config_desc, sizeof (vcom_config_desc));
  else if (desc_type == STRING_DESCRIPTOR)
    {
      const uint8_t *str;
      int size;

      switch (desc_index)
	{
	case 0:
	  str = vcom_string0;
	  size = sizeof (vcom_string0);
	  break;
	case 1:
	  str = vcom_string1;
	  size = sizeof (vcom_string1);
	  break;
	case 2:
	  str = vcom_string2;
	  size = sizeof (vcom_string2);
	  break;
	case 3:
	  str = vcom_string3;
	  size = sizeof (vcom_string3);
	  break;
	default:
	  return -1;
	}

      return usb_lld_ctrl_send (dev, str, size);
    }

  return -1;
}

static void
vcom_setup_endpoints_for_interface (uint16_t interface, int stop)
{
  struct cdc *s = cdc_get (interface, 0);

  if (interface == 0)
    {
      if (!stop)
	usb_lld_setup_endpoint (s->endp2, EP_INTERRUPT, 0, 0, ENDP2_TXADDR, 0);
      else
	usb_lld_stall_tx (s->endp2);
    }
  else if (interface == 1)
    {
      if (!stop)
	{
	  usb_lld_setup_endpoint (s->endp1, EP_BULK, 0, 0, ENDP1_TXADDR, 0);
	  usb_lld_setup_endpoint (s->endp3, EP_BULK, 0, ENDP3_RXADDR, 0, 64);
	  /* Start with no data receiving (ENDP3 not enabled)*/
	}
      else
	{
	  usb_lld_stall_tx (s->endp1);
	  usb_lld_stall_rx (s->endp3);
	}
    }
  else if (interface == 2)
    {
      if (!stop)
	usb_lld_setup_endpoint (s->endp2, EP_INTERRUPT, 0, 0, ENDP5_TXADDR, 0);
      else
	usb_lld_stall_tx (s->endp2);
    }
  else if (interface == 3)
    {
      if (!stop)
	{
	  usb_lld_setup_endpoint (s->endp1, EP_BULK, 0, 0, ENDP4_TXADDR, 0);
	  usb_lld_setup_endpoint (s->endp3, EP_BULK, 0, ENDP6_RXADDR, 0, 64);
	  /* Start with no data receiving (ENDP6 not enabled)*/
	}
      else
	{
	  usb_lld_stall_tx (s->endp1);
	  usb_lld_stall_rx (s->endp3);
	}
    }
}

static int
usb_set_configuration (struct usb_dev *dev)
{
  int i;
  uint8_t current_conf;

  current_conf = usb_lld_current_configuration (dev);
  if (current_conf == 0)
    {
      if (dev->dev_req.value != 1)
	return -1;

      usb_lld_set_configuration (dev, 1);
      for (i = 0; i < NUM_INTERFACES; i++)
	vcom_setup_endpoints_for_interface (i, 0);
      device_state = USB_DEVICE_STATE_CONFIGURED;
      for (i = 0; i < MAX_CDC; i++)
	{
	  struct cdc *s = &cdc_table[i];

	  chopstx_mutex_lock (&s->mtx);
	  chopstx_cond_signal (&s->cnd_rx);
	  chopstx_mutex_unlock (&s->mtx);
	}
    }
  else if (current_conf != dev->dev_req.value)
    {
      if (dev->dev_req.value != 0)
	return -1;

      usb_lld_set_configuration (dev, 0);
      for (i = 0; i < NUM_INTERFACES; i++)
	vcom_setup_endpoints_for_interface (i, 1);
      device_state = USB_DEVICE_STATE_ADDRESSED;
      for (i = 0; i < MAX_CDC; i++)
	{
	  struct cdc *s = &cdc_table[i];

	  chopstx_mutex_lock (&s->mtx);
	  chopstx_cond_signal (&s->cnd_rx);
	  chopstx_mutex_unlock (&s->mtx);
	}
    }

  usb_lld_ctrl_ack (dev);
  return 0;
}


static int
usb_set_interface (struct usb_dev *dev)
{
  uint16_t interface = dev->dev_req.index;
  uint16_t alt = dev->dev_req.value;

  if (interface >= NUM_INTERFACES)
    return -1;

  if (alt != 0)
    return -1;
  else
    {
      vcom_setup_endpoints_for_interface (interface, 0);
      return usb_lld_ctrl_ack (dev);
    }
}

static int
usb_get_interface (struct usb_dev *dev)
{
  const uint8_t zero = 0;
  uint16_t interface = dev->dev_req.index;

  if (interface >= NUM_INTERFACES)
    return -1;

  /* We don't have alternate interface, so, always return 0.  */
  return usb_lld_ctrl_send (dev, &zero, 1);
}

static int
usb_get_status_interface (struct usb_dev *dev)
{
  const uint16_t status_info = 0;
  uint16_t interface = dev->dev_req.index;

  if (interface >= NUM_INTERFACES)
    return -1;

  return usb_lld_ctrl_send (dev, &status_info, 2);
}


static void
usb_tx_done (uint8_t ep_num, uint16_t len)
{
  struct cdc *s = cdc_get (-1, ep_num);

  (void)len;

  chopstx_mutex_lock (&s->mtx);
  if (ep_num == s->endp1)
    {
      if (s->flag_output_ready == 0)
	{
	  s->flag_output_ready = 1;
	  chopstx_cond_signal (&s->cnd_tx);
	}
    }
  else if (ep_num == s->endp2)
    {
      s->flag_notify_busy = 0;
    }
  chopstx_mutex_unlock (&s->mtx);
}


static void
usb_rx_ready (uint8_t ep_num, uint16_t len)
{
  struct cdc *s = cdc_get (-1, ep_num);

  if (ep_num == s->endp3)
    {
      usb_lld_rxcpy (s->input, ep_num, 0, len);
      s->flag_input_avail = 1;
      s->input_len = len;
      chopstx_cond_signal (&s->cnd_rx);
    }
}

static void *cdc_main (void *arg);


void
cdc_init (uint16_t prio, uintptr_t stack_addr, size_t stack_size,
	  void (*sendbrk_callback) (uint8_t dev_no, uint16_t duration),
	  void (*config_callback) (uint8_t dev_no,
				   uint32_t bitrate, uint8_t format,
				   uint8_t paritytype, uint8_t databits))
{
  int i;

  send_break = sendbrk_callback;
  setup_usart_config = config_callback;
  for (i = 0; i < MAX_CDC; i++)
    {
      struct cdc *s = &cdc_table[i];

      chopstx_mutex_init (&s->mtx);
      chopstx_cond_init (&s->cnd_tx);
      chopstx_cond_init (&s->cnd_rx);
      s->input_len = 0;
      s->flag_connected = 0;
      s->flag_output_ready = 1;
      s->flag_input_avail = 0;
      memcpy (&s->line_coding, &lc_default, sizeof (struct line_coding));

      if (i == 0)
	{
	  s->dev_no = 2;
	  s->endp1 = ENDP1;
	  s->endp2 = ENDP2;
	  s->endp3 = ENDP3;
	}
      else
	{
	  s->dev_no = 3;
	  s->endp1 = ENDP4;
	  s->endp2 = ENDP5;
	  s->endp3 = ENDP6;
	}
    }

  device_state = USB_DEVICE_STATE_UNCONNECTED;
  chopstx_create (prio, stack_addr, stack_size, cdc_main, NULL);
}

struct cdc *
cdc_open (uint8_t cdc_num)
{
  struct cdc *s;

  if (cdc_num >= MAX_CDC)
    return NULL;

  s = &cdc_table[cdc_num];
  return s;
}


static void *
cdc_main (void *arg)
{
  struct usb_dev dev;
  int e;

  (void)arg;

#if defined(OLDER_SYS_H)
  /*
   * Historically (before sys < 3.0), NVIC priority setting for USB
   * interrupt was done in usb_lld_sys_init for free standing
   * application.  Thus this compatibility code.
   *
   * We can't call usb_lld_init after chopstx_claim_irq, as
   * usb_lld_init does its own setting for NVIC, which is incompatible
   * to Chopstx's interrupt handling.  Calling chopstx_claim_irq after
   * usb_lld_init overrides that for Chopstx.
   *
   */
  usb_lld_init (&dev, VCOM_FEATURE_BUS_POWERED);
  chopstx_claim_irq (&usb_intr, INTR_REQ_USB);
#else
  chopstx_claim_irq (&usb_intr, INTR_REQ_USB);
  usb_lld_init (&dev, VCOM_FEATURE_BUS_POWERED);
#endif

  while (1)
    {
      chopstx_intr_wait (&usb_intr);
      if (usb_intr.ready)
	{
	  uint8_t ep_num;
#if defined(OLDER_SYS_H)
	event_handle:
#endif
	  /*
	   * When interrupt is detected, call usb_lld_event_handler.
	   * The event may be one of following:
	   *    (1) Transfer to endpoint (bulk or interrupt)
	   *        In this case EP_NUM is encoded in the variable E.
	   *    (2) "NONE" event: some trasfer was done, but all was
	   *        done by lower layer, no other work is needed in
	   *        upper layer.
	   *    (3) Device events: Reset or Suspend
	   *    (4) Device requests to the endpoint zero.
	   *        
	   */
	  e = usb_lld_event_handler (&dev);
	  chopstx_intr_done (&usb_intr);
	  ep_num = USB_EVENT_ENDP (e);

	  if (ep_num != 0)
	    {
	      if (USB_EVENT_TXRX (e))
		usb_tx_done (ep_num, USB_EVENT_LEN (e));
	      else
		usb_rx_ready (ep_num, USB_EVENT_LEN (e));
	    }
	  else
	    switch (USB_EVENT_ID (e))
	      {
	      case USB_EVENT_DEVICE_RESET:
		usb_device_reset (&dev);
		continue;

	      case USB_EVENT_DEVICE_ADDRESSED:
		/* The addres is assigned to the device.  We don't
		 * need to do anything for this actually, but in this
		 * application, we maintain the USB status of the
		 * device.  Usually, just "continue" as EVENT_OK is
		 * OK.
		 */
		device_state = USB_DEVICE_STATE_ADDRESSED;
		{
		  struct cdc *s = &cdc_table[0];
		  chopstx_mutex_lock (&s->mtx);
		  chopstx_cond_signal (&s->cnd_rx);
		  chopstx_mutex_unlock (&s->mtx);
		}
		continue;

	      case USB_EVENT_GET_DESCRIPTOR:
		if (usb_get_descriptor (&dev) < 0)
		  usb_lld_ctrl_error (&dev);
		continue;

	      case USB_EVENT_SET_CONFIGURATION:
		if (usb_set_configuration (&dev) < 0)
		  usb_lld_ctrl_error (&dev);
		continue;

	      case USB_EVENT_SET_INTERFACE:
		if (usb_set_interface (&dev) < 0)
		  usb_lld_ctrl_error (&dev);
		continue;

	      case USB_EVENT_CTRL_REQUEST:
		/* Device specific device request.  */
		if (usb_setup (&dev) < 0)
		  usb_lld_ctrl_error (&dev);
		continue;

	      case USB_EVENT_GET_STATUS_INTERFACE:
		if (usb_get_status_interface (&dev) < 0)
		  usb_lld_ctrl_error (&dev);
		continue;

	      case USB_EVENT_GET_INTERFACE:
		if (usb_get_interface (&dev) < 0)
		  usb_lld_ctrl_error (&dev);
		continue;

	      case USB_EVENT_SET_FEATURE_DEVICE:
	      case USB_EVENT_SET_FEATURE_ENDPOINT:
	      case USB_EVENT_CLEAR_FEATURE_DEVICE:
	      case USB_EVENT_CLEAR_FEATURE_ENDPOINT:
		usb_lld_ctrl_ack (&dev);
		continue;

	      case USB_EVENT_CTRL_WRITE_FINISH:
		/* Control WRITE transfer finished.  */
		usb_ctrl_write_finish (&dev);
		continue;

	      case USB_EVENT_OK:
	      case USB_EVENT_DEVICE_SUSPEND:
	      default:
		continue;
	      }
	}
    }

  return NULL;
}


void
cdc_wait_configured (void)
{
  struct cdc *s = &cdc_table[0];

  chopstx_mutex_lock (&s->mtx);
  while (device_state != USB_DEVICE_STATE_CONFIGURED)
    chopstx_cond_wait (&s->cnd_rx, &s->mtx);
  chopstx_mutex_unlock (&s->mtx);
}


void
cdc_wait_connection (struct cdc *s)
{
  chopstx_mutex_lock (&s->mtx);
  while (s->flag_connected == 0)
    chopstx_cond_wait (&s->cnd_rx, &s->mtx);
  s->flag_output_ready = 1;
  s->flag_input_avail = 0;
  s->input_len = 0;
  usb_lld_rx_enable (s->endp3);	/* Accept input for line */
  chopstx_mutex_unlock (&s->mtx);
}

static int
check_tx (struct cdc *s)
{
  if (s->flag_output_ready)
    /* TX done */
    return 1;
  if (s->flag_connected == 0)
    /* Disconnected */
    return -1;
  return 0;
}

int
cdc_send (struct cdc *s, const char *buf, int len)
{
  int r;
  const char *p;
  int count;

  p = buf;
  count = len >= 64 ? 64 : len;

  while (1)
    {
      chopstx_mutex_lock (&s->mtx);
      while ((r = check_tx (s)) == 0)
	chopstx_cond_wait (&s->cnd_tx, &s->mtx);
      if (r > 0)
	{
	  usb_lld_txcpy (p, s->endp1, 0, count);
	  usb_lld_tx_enable (s->endp1, count);
	  s->flag_output_ready = 0;
	}
      chopstx_mutex_unlock (&s->mtx);

      len -= count;
      p += count;
      if (len == 0 && count != 64)
	/*
	 * The size of the last packet should be != 0
	 * If 64, send ZLP (zelo length packet)
	 */
	break;
      count = len >= 64 ? 64 : len;
    }

  return r;
}


static int
check_rx (void *arg)
{
  struct cdc *s = arg;

  if (s->flag_input_avail)
    /* RX */
    return 1;
  if (s->flag_connected == 0)
    /* Disconnected */
    return 1;
  return 0;
}

/*
 * Returns -1 on connection close
 *          0 on timeout.
 *          >0 length of the input
 *
 */
int
cdc_recv (struct cdc *s, char *buf, uint32_t *timeout)
{
  int r;
  chopstx_poll_cond_t poll_desc;

  poll_desc.type = CHOPSTX_POLL_COND;
  poll_desc.ready = 0;
  poll_desc.cond = &s->cnd_rx;
  poll_desc.mutex = &s->mtx;
  poll_desc.check = check_rx;
  poll_desc.arg = s;

  while (1)
    {
      struct chx_poll_head *pd_array[1] = {
	(struct chx_poll_head *)&poll_desc
      };
      chopstx_poll (timeout, 1, pd_array);
      chopstx_mutex_lock (&s->mtx);
      r = check_rx (s);
      chopstx_mutex_unlock (&s->mtx);
      if (r || (timeout != NULL && *timeout == 0))
	break;
    }

  chopstx_mutex_lock (&s->mtx);
  if (s->flag_connected == 0)
    r = -1;
  else if (s->flag_input_avail)
    {
      r = s->input_len;
      memcpy (buf, s->input, r);
      s->flag_input_avail = 0;
      usb_lld_rx_enable (s->endp3);
      s->input_len = 0;
    }
  else
    r = 0;
  chopstx_mutex_unlock (&s->mtx);

  return r;
}


int
cdc_ss_notify (struct cdc *s, uint16_t state_bits)
{
  int busy;
  uint8_t notification[10];
  int interface;

  if (s == &cdc_table[0])
    interface = 0;
  else
    interface = 2;

  /* Little endian */
  notification[0] = REQUEST_DIR | CLASS_REQUEST | INTERFACE_RECIPIENT;
  notification[1] = USB_CDC_NOTIFY_SERIAL_STATE;
  notification[2] = notification[3] = 0;
  notification[4] = interface;
  notification[5] = 0;
  notification[6] = 2;
  notification[7] = 0;
  notification[8] = (state_bits & 0xff);
  notification[9] = (state_bits >> 8);

  chopstx_mutex_lock (&s->mtx);
  busy = s->flag_notify_busy;
  if (!busy)
    {
      usb_lld_write (s->endp2, notification, sizeof notification);
      s->flag_notify_busy = 1;
    }
  chopstx_mutex_unlock (&s->mtx);

  return busy;
}
