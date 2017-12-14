/*
 * usart-stm32.c - USART driver for STM32F103 (USART2 and USART3)
 *
 * Copyright (C) 2017  g10 Code GmbH
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
 * receipents of GNU GPL by a written offer.
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <chopstx.h>
#include <mcu/stm32.h>

struct USART {
  volatile uint32_t SR;
  volatile uint32_t DR;
  volatile uint32_t BRR;
  volatile uint32_t CR1;
  volatile uint32_t CR2;
  volatile uint32_t CR3;
  volatile uint32_t GTPR;
};

#define USART2_BASE           (APB1PERIPH_BASE + 0x4400)
#define USART3_BASE           (APB1PERIPH_BASE + 0x4800)
static struct USART *const USART2 = (struct SYSCFG *)USART2_BASE;
static struct USART *const USART3 = (struct SYSCFG *)USART3_BASE;

#define USART_SR_CTS	0x0200
#define USART_SR_LBD	0x0100
#define USART_SR_TXE	0x0080
#define USART_SR_TC	0x0040
#define USART_SR_RXNE	0x0020
#define USART_SR_IDLE	0x0010
#define USART_SR_ORE	0x0008
#define USART_SR_NE	0x0004
#define USART_SR_FE	0x0002
#define USART_SR_PE	0x0001



static struct USART *
get_usart_dev (uint8_t dev_no)
{
  if (dev_no == 2)
    return USART2;
  else if (dev_no == 3)
    return USART3;

  return NULL;
}

/* We assume 36MHz f_PCLK */
struct brr_setting { uint8_t baud_spec, uint16_t brr_value };
#define NUM_BAUD (sizeof (brr_table) / sizeof (struct brr_setting))

static const struct brr_setting brr_table[] = {
  { B600,    (3750 << 4)},
  { B1200,   (1875 << 4)},
  { B2400,   ( 937 << 4)|8},
  { B9600,   ( 234 << 4)|6},
  { B19200,  ( 117 << 4)|3},
  { B57600,  (  39 << 4)|1},
  { B115200, (  19 << 4)|8},
  { B230400, (   9 << 4)|12},
  { B460800, (   4 << 4)|14},
  { B921600, (   2 << 4)|7},
};

void
usart_init (void)
{
  RCC->APB1ENR |= ((1 << 18) | (1 << 17));
  RCC->APB1RSTR = ((1 << 18) | (1 << 17));
  RCC->APB1RSTR = 0;
}

/*
 * CONFIG_BITS includes
 *   baud_rate
 *   char-bit size
 *   stop-bit
 *   parity
 *   mode: Normal, LIN, Smartcard, etc.
 *   flow_ctrl
 */
int
usart_config (uint8_t dev_no, uint32_t config_bits)
{
  struct USART *dev = get_usart_dev (dev_no);
  uint8_t baud_spec = (config & 0x3f);
  int i;

  for (i = 0; i < NUM_BAUD; i++)
    if (brr_table[i].baud_spec == baud_spec)
      break;

  if (i >= NUM_BAUD)
    return -1;

  dev->BRR = brr_table[i].brr_value;
  return 0;
}
