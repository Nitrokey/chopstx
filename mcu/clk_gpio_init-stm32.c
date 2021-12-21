/*
 * clk_gpio_init-stm32.c - Clock and GPIO initialization for STM32.
 *
 * Copyright (C) 2015, 2018  Flying Stone Technology
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
//#include "chip_config.h"
#if defined(MCU_STM32F0)
#include <mcu/stm32.h>
#else
#include <mcu/stm32f103.h>
#include "chip_config.h"
#endif

#if defined(MCU_STM32F0)
#define STM32_PPRE1		STM32_PPRE1_DIV1
#define STM32_PLLSRC		STM32_PLLSRC_HSI
#define STM32_FLASHBITS		0x00000011
#define STM32_PLLCLKIN		(STM32_HSICLK / 2)
#else
#define STM32_PPRE1		STM32_PPRE1_DIV2
#define STM32_PLLSRC		STM32_PLLSRC_HSE
#define STM32_FLASHBITS		0x00000012
#define STM32_PLLCLKIN		(STM32_HSECLK / 1)
#endif

#define STM32_SW		STM32_SW_PLL
#define STM32_HPRE		STM32_HPRE_DIV1
#define STM32_PPRE2		STM32_PPRE2_DIV1
#ifndef STM32_ADCPRE
#define STM32_ADCPRE		STM32_ADCPRE_DIV6
#endif
#define STM32_MCOSEL		STM32_MCO_NOCLOCK
#ifndef STM32_USBPRE
#define STM32_USBPRE		STM32_USBPRE_DIV1P5
#endif

#define STM32_PLLMUL		((STM32_PLLMUL_VALUE - 2) << 18) //TODO: Replacing the 8 with STM32_PLLMUL_VALUE as it was originally creates an overflow! why?
#define STM32_PLLCLKOUT		(STM32_PLLCLKIN * STM32_PLLMUL_VALUE)
#define STM32_SYSCLK		STM32_PLLCLKOUT
#define STM32_HCLK		(STM32_SYSCLK / 1)


static void __attribute__((used))
clock_init (void)
{
  /* HSI setup */
  RCC->CR |= RCC_CR_HSION;
  while (!(RCC->CR & RCC_CR_HSIRDY))
    ;
  /* Reset HSEON, HSEBYP, CSSON, and PLLON, not touching RCC_CR_HSITRIM */
  RCC->CR &= (RCC_CR_HSITRIM | RCC_CR_HSION);
  RCC->CFGR = 0;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI)
    ;

#if !defined(MCU_STM32F0)
  /* HSE setup */
  RCC->CR |= RCC_CR_HSEON;
  while (!(RCC->CR & RCC_CR_HSERDY))
    ;
#endif

  /* PLL setup */
  RCC->CFGR |= STM32_PLLMUL | STM32_PLLXTPRE | STM32_PLLSRC;
  RCC->CR   |= RCC_CR_PLLON;
  while (!(RCC->CR & RCC_CR_PLLRDY))
    ;

  /* Clock settings */
  RCC->CFGR = STM32_MCOSEL | STM32_USBPRE | STM32_PLLMUL | STM32_PLLXTPRE
    | STM32_PLLSRC | STM32_ADCPRE | STM32_PPRE2 | STM32_PPRE1 | STM32_HPRE;

  /*
   * We don't touch RCC->CR2, RCC->CFGR2, RCC->CFGR3, and RCC->CIR.
   */

  /* Flash setup */
  FLASH->ACR = STM32_FLASHBITS;

  /* Switching on the configured clock source. */
  RCC->CFGR |= STM32_SW;
  while ((RCC->CFGR & RCC_CFGR_SWS) != (STM32_SW << 2))
    ;

#if defined(MCU_STM32F0)
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
  RCC->APB2RSTR = RCC_APB2RSTR_SYSCFGRST;
  RCC->APB2RSTR = 0;

# if defined(STM32F0_USE_VECTOR_ON_RAM)
  /* Use vectors on RAM */
  SYSCFG->CFGR1 = (SYSCFG->CFGR1 & ~SYSCFG_CFGR1_MEM_MODE) | 3;
# endif
#endif
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
#if defined(MCU_STM32F0)
  RCC->AHBENR |= RCC_ENR_IOP_EN;
  RCC->AHBRSTR = RCC_RSTR_IOP_RST;
  RCC->AHBRSTR = 0;
#else
  RCC->APB2ENR |= RCC_ENR_IOP_EN;
  RCC->APB2RSTR = RCC_RSTR_IOP_RST;
  RCC->APB2RSTR = 0;
#endif

#if defined(MCU_STM32F0)
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
#else
#ifdef AFIO_MAPR_SOMETHING
  AFIO->MAPR |= AFIO_MAPR_SOMETHING;
#endif

  /* LED is mandatory.  We configure it always.  */
  GPIO_LED->ODR = VAL_GPIO_LED_ODR;
  GPIO_LED->CRH = VAL_GPIO_LED_CRH;
  GPIO_LED->CRL = VAL_GPIO_LED_CRL;

  /* If there is USB enabler pin and it's independent, we configure it.  */
#if defined(GPIO_USB_BASE) && GPIO_USB_BASE != GPIO_LED_BASE
  GPIO_USB->ODR = VAL_GPIO_USB_ODR;
  GPIO_USB->CRH = VAL_GPIO_USB_CRH;
  GPIO_USB->CRL = VAL_GPIO_USB_CRL;
#endif

#ifdef GPIO_OTHER_BASE
  GPIO_OTHER->ODR = VAL_GPIO_OTHER_ODR;
  GPIO_OTHER->CRH = VAL_GPIO_OTHER_CRH;
  GPIO_OTHER->CRL = VAL_GPIO_OTHER_CRL;
#endif
#endif
}
