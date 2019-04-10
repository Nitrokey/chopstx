/*
 * clk_gpio_init-stm32l.c - Clock and GPIO initialization for STM32L.
 *
 * Copyright (C) 2019  Flying Stone Technology
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

#include <mcu/stm32l.h>

static void __attribute__((used))
clock_init (void)
{
}

static struct GPIO *const GPIO_LED = (struct GPIO *)GPIO_LED_BASE;
#ifdef GPIO_USB_BASE
static struct GPIO *const GPIO_USB = (struct GPIO *)GPIO_USB_BASE;
#endif
#ifdef GPIO_OTHER_BASE
static struct GPIO *const GPIO_OTHER = (struct GPIO *)GPIO_OTHER_BASE;
#endif

static void __attribute__((used))
gpio_init (void)
{
  /* Enable GPIO clock. */

  /* LED is mandatory.  We configure it always.  */
  GPIO_LED->ODR = VAL_GPIO_LED_ODR;
}
