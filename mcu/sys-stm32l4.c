/*
 * sys-stm32l432.c - system routines for STM32L432.
 *
 * Copyright (C) 2019  Flying Stone Technology
 * Author: NIIBE Yutaka <gniibe@fsij.org>
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.  This file is offered as-is,
 * without any warranty.
 *
 * We put some system routines (which is useful for any program) here.
 */

#include <stdint.h>
#include <stdlib.h>
#include <mcu/cortex-m.h>
#include "board.h"

#include "mcu/clk_gpio_init-stm32l.c"

void
set_led (int on)
{
#if defined(GPIO_LED_CLEAR_TO_EMIT)
  if (on)
    GPIO_LED->BRR = (1 << GPIO_LED_CLEAR_TO_EMIT);
  else
    GPIO_LED->BSRR = (1 << GPIO_LED_CLEAR_TO_EMIT);
#else
  if (on)
    GPIO_LED->BSRR = (1 << GPIO_LED_SET_TO_EMIT);
  else
    GPIO_LED->BRR = (1 << GPIO_LED_SET_TO_EMIT);
#endif
}

static void wait (int count)
{
  int i;

  for (i = 0; i < count; i++)
    asm volatile ("" : : "r" (i) : "memory");
}


void
usb_lld_sys_shutdown (void)
{
  RCC->APB1ENR1 &= ~(RCC_PHR_USB | RCC_PHR_CRS);
  RCC->APB1RSTR1 |= (RCC_PHR_USB | RCC_PHR_CRS);
}

void
usb_lld_sys_init (void)
{
  /* XXX: should configure CRS (clock recovery system) and HSI48 clock */

  if ((RCC->APB1ENR1 & RCC_PHR_USB) && (RCC->APB1RSTR1 & RCC_PHR_USB) == 0)
    /* Make sure the device is disconnected, even after core reset.  */
    {
      usb_lld_sys_shutdown ();
      /* Disconnect requires SE0 (>= 2.5uS).  */
      wait (5*MHZ);
    }

  RCC->APB1ENR1 |= (RCC_PHR_USB | RCC_PHR_CRS);
  RCC->APB1RSTR1 = (RCC_PHR_USB | RCC_PHR_CRS);
  RCC->APB1RSTR1 = 0;
}

/* Not yet implemented, API should be reconsidered */

void
flash_unlock (void)
{
}


#define intr_disable()  asm volatile ("cpsid   i" : : : "memory")
#define intr_enable()  asm volatile ("cpsie   i" : : : "memory")


int
flash_wait_for_last_operation (uint32_t timeout)
{
  (void)timeout;
  return 0;
}

int
flash_program_halfword (uintptr_t addr, uint16_t data)
{
  (void)addr;
  (void)data;
  return 0;
}

int
flash_erase_page (uintptr_t addr)
{
  (void)addr;
  return 0;
}

int
flash_check_blank (const uint8_t *p_start, size_t size)
{
  (void)p_start;
  (void)size;
  return 1;
}


int
flash_write (uintptr_t dst_addr, const uint8_t *src, size_t len)
{
  (void)dst_addr;
  (void)src;
  (void)len;
  return 1;
}

int
flash_protect (void)
{
  return 0;
}

void __attribute__((naked))
flash_erase_all_and_exec (void (*entry)(void))
{
  (void)entry;
}

void
nvic_system_reset (void)
{
  SCB->AIRCR = (0x05FA0000 | (SCB->AIRCR & 0x70) | SCB_AIRCR_SYSRESETREQ);
  asm volatile ("dsb");
  for (;;);
}

const uint8_t sys_version[8] __attribute__((section(".sys.version"))) = {
  3*2+2,	     /* bLength */
  0x03,		     /* bDescriptorType = USB_STRING_DESCRIPTOR_TYPE */
  /* sys version: "3.0" */
  '3', 0, '.', 0, '0', 0,
};

#if defined(USE_SYS3) || defined(USE_SYS_BOARD_ID)
const uint32_t __attribute__((section(".sys.board_id")))
sys_board_id = BOARD_ID;

const uint8_t __attribute__((section(".sys.board_name")))
sys_board_name[] = BOARD_NAME;
#endif
