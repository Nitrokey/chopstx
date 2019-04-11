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

#define STM32_FLASHBITS		0x00000704

void
clock_init (void)
{
  /* MSI: 4MHz (keep the default value) */

  /* PLL input: MSI at 4MHz, VCO: 160 MHz, output 80MHz */
  RCC->PLLCFGR = ((1 << 24) | (40 << 8)) | 0x01;

  /* Enable PLL */
  RCC->CR |= (1 << 24);
  while (!(RCC->CR & (1 << 25)))
    ;

  /* Flash setup: four wait states at 80MHz */
  FLASH->ACR = STM32_FLASHBITS;
  while ((FLASH->ACR & 0x07) != (STM32_FLASHBITS & 0x07))
    ;

  /* Configure bus clocks: AHB: 80MHz, APB1: 40MHz, APB2: 80MHz */
  RCC->CFGR = (0x04 << 8);

  /* Switch SYSCLOCK using PLL */
  RCC->CFGR |= 0x03;
  while ((RCC->CFGR & 0x0C) != 0x0C)
    ;
}

static struct GPIO *const GPIO_LED = (struct GPIO *)GPIO_LED_BASE;
#ifdef GPIO_USB_BASE
static struct GPIO *const GPIO_USB = (struct GPIO *)GPIO_USB_BASE;
#endif
#ifdef GPIO_OTHER_BASE
static struct GPIO *const GPIO_OTHER = (struct GPIO *)GPIO_OTHER_BASE;
#endif

void
gpio_init (void)
{
  /* Enable GPIO clock. */
  RCC->AHB2ENR |= RCC_PHR_GPIO;
  RCC->AHB2RSTR = RCC_PHR_GPIO;
  RCC->AHB2RSTR = 0;

  /* Delay (more than two clocks) is needed.  */
  while (RCC->AHB2RSTR != 0)
    ;

  /* LED is mandatory.  We configure it always.  */
  GPIO_LED->OSPEEDR = VAL_GPIO_LED_OSPEEDR;
  GPIO_LED->OTYPER  = VAL_GPIO_LED_OTYPER;
  GPIO_LED->MODER   = VAL_GPIO_LED_MODER;
  GPIO_LED->PUPDR   = VAL_GPIO_LED_PUPDR;

#ifdef GPIO_OTHER_BASE
  GPIO_OTHER->OSPEEDR = VAL_GPIO_OTHER_OSPEEDR;
  GPIO_OTHER->OTYPER  = VAL_GPIO_OTHER_OTYPER;
  GPIO_OTHER->MODER   = VAL_GPIO_OTHER_MODER;
  GPIO_OTHER->PUPDR   = VAL_GPIO_OTHER_PUPDR;
#endif
}
