/*
 * usb-stm32f103.c - USB driver for STM32F103
 *
 * Copyright (C) 2016, 2017, 2018  Flying Stone Technology
 * Author: NIIBE Yutaka <gniibe@fsij.org>
 *
 * This file is a part of Chopstx, a thread library for embedded.
 *
 * Chopstx is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Chopstx is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As additional permission under GNU GPL version 3 section 7, you may
 * distribute non-source form of the Program without the copy of the
 * GNU GPL normally required by section 4, provided you inform the
 * recipients of GNU GPL by a written offer.
 *
 */

#include <stdint.h>
#include <stdlib.h>

#include "stm32f103.h"
#include "usb_lld.h"
#include "usb_lld_driver.h"

#define REG_BASE  (0x40005C00UL) /* USB Peripheral Registers base address */
#define PMA_ADDR  (0x40006000UL) /* USB Packet Memory Area base address   */

static void
epbuf_set_tx_addr (uint8_t ep_num, uint16_t addr)
{
  uint16_t *reg_p = (uint16_t *)(PMA_ADDR + (ep_num*8+0)*2);

  *reg_p = addr;
}

static uint16_t
epbuf_get_tx_addr (uint8_t ep_num)
{
  uint16_t *reg_p = (uint16_t *)(PMA_ADDR + (ep_num*8+0)*2);

  return *reg_p;
}


static void
epbuf_set_tx_count (uint8_t ep_num, uint16_t size)
{
  uint16_t *reg_p = (uint16_t *)(PMA_ADDR + (ep_num*8+2)*2);

  *reg_p = size;
}

static uint16_t
epbuf_get_tx_count (uint8_t ep_num)
{
  uint16_t *reg_p = (uint16_t *)(PMA_ADDR + (ep_num*8+2)*2);

  return *reg_p  & 0x03ff;
}


static void
epbuf_set_rx_addr (uint8_t ep_num, uint16_t addr)
{
  uint16_t *reg_p = (uint16_t *)(PMA_ADDR + (ep_num*8+4)*2);

  *reg_p = addr;
}

static uint16_t
epbuf_get_rx_addr (uint8_t ep_num)
{
  uint16_t *reg_p = (uint16_t *)(PMA_ADDR + (ep_num*8+4)*2);

  return *reg_p;
}


static void
epbuf_set_rx_buf_size (uint8_t ep_num, uint16_t size)
{				/* Assume size is even */
  uint16_t *reg_p = (uint16_t *)(PMA_ADDR + (ep_num*8+6)*2);
  uint16_t value;

  if (size <= 62)
    value = (size & 0x3e) << 9;
  else
    value = 0x8000 | (((size >> 5) - 1) << 10);

  *reg_p = value;
}

static uint16_t
epbuf_get_rx_count (uint8_t ep_num)
{
  uint16_t *reg_p = (uint16_t *)(PMA_ADDR + (ep_num*8+6)*2);

  return *reg_p & 0x03ff;
}


static void
usb_lld_shutdown_chip_specific (void)
{
  RCC->APB1ENR &= ~RCC_APB1ENR_USBEN;
  RCC->APB1RSTR = RCC_APB1RSTR_USBRST;
}

static void
wait (int count)
{
  int i;

  for (i = 0; i < count; i++)
    asm volatile ("" : : "r" (i) : "memory");
}

static void
usb_lld_init_chip_specific (void)
{
  if ((RCC->APB1ENR & RCC_APB1ENR_USBEN)
      && (RCC->APB1RSTR & RCC_APB1RSTR_USBRST) == 0)
    /* Make sure the device is disconnected, even after core reset.  */
    {
      usb_lld_shutdown_chip_specific ();
      /* Disconnect requires SE0 (>= 2.5uS).  */
      wait (5*MHZ);
    }

  if ((RCC->APB1ENR & RCC_APB1ENR_USBEN) == 0)
    {
      RCC->APB1ENR |= RCC_APB1ENR_USBEN;
      RCC->APB1RSTR = RCC_APB1RSTR_USBRST;
      RCC->APB1RSTR = 0;
    }
}

#include "usb-st-common.c"

static int
handle_setup0 (struct usb_dev *dev)
{
  const uint16_t *pw;
  uint16_t w;
  uint8_t req_no;
  HANDLER handler;

  pw = (uint16_t *)(PMA_ADDR + (epbuf_get_rx_addr (ENDP0) * 2));
  w = *pw++;

  dev->dev_req.type = (w & 0xff);
  dev->dev_req.request = req_no = (w >> 8);
  pw++;
  dev->dev_req.value = *pw++;
  pw++;
  dev->dev_req.index = *pw++;
  pw++;
  dev->dev_req.len = *pw;

  dev->ctrl_data.addr = NULL;
  dev->ctrl_data.len = 0;
  dev->ctrl_data.require_zlp = 0;

  if ((dev->dev_req.type & REQUEST_TYPE) == STANDARD_REQUEST)
    {
      int r;

      switch (req_no)
	{
	case 0: handler = std_get_status;  break;
	case 1: handler = std_clear_feature;  break;
	case 3: handler = std_set_feature;  break;
	case 5: handler = std_set_address;  break;
	case 6: handler = std_get_descriptor;  break;
	case 8: handler = std_get_configuration;  break;
	case 9: handler = std_set_configuration;  break;
	case 10: handler = std_get_interface;  break;
	case 11: handler = std_set_interface;  break;
	default: handler = std_none;  break;
	}

      if ((r = (*handler) (dev)) < 0)
	{
	  usb_lld_ctrl_error (dev);
	  return USB_EVENT_OK;
	}
      else
	return r;
    }
  else
    return USB_EVENT_CTRL_REQUEST;
}

void
usb_lld_to_pmabuf (const void *src, uint16_t addr, size_t n)
{
  const uint8_t *s = (const uint8_t *)src;
  uint16_t *p;
  uint16_t w;

  if (n == 0)
    return;

  if ((addr & 1))
    {
      p = (uint16_t *)(PMA_ADDR + (addr - 1) * 2);
      w = *p;
      w = (w & 0xff) | (*s++) << 8;
      *p = w;
      p += 2;
      n--;
    }
  else
    p = (uint16_t *)(PMA_ADDR + addr * 2);

  while (n >= 2)
    {
      w = *s++;
      w |= (*s++) << 8;
      *p = w;
      p += 2;
      n -= 2;
    }

  if (n > 0)
    {
      w = *s;
      *p = w;
    }
}

void
usb_lld_from_pmabuf (void *dst, uint16_t addr, size_t n)
{
  uint8_t *d = (uint8_t *)dst;
  uint16_t *p;
  uint16_t w;

  if (n == 0)
    return;

  if ((addr & 1))
    {
      p = (uint16_t *)(PMA_ADDR + (addr - 1) * 2);
      w = *p;
      *d++ = (w >> 8);
      p += 2;
      n--;
    }
  else
    p = (uint16_t *)(PMA_ADDR + addr * 2);

  while (n >= 2)
    {
      w = *p;
      *d++ = (w & 0xff);
      *d++ = (w >> 8);
      p += 2;
      n -= 2;
    }

  if (n > 0)
    {
      w = *p;
      *d = (w & 0xff);
    }
}
