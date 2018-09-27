/*
 * ackbtn-stm32f103.c - Acknowledge button support for STM32F103
 *
 * Copyright (C) 2018  g10 Code GmbH
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
#include <string.h>
#include <chopstx.h>
#include <mcu/stm32f103.h>

#include "board.h"
#include "sys.h"

static uint32_t pin_config;

void
ackbtn_init (chopstx_intr_t *intr)
{
  uint8_t irq_num;
  uint32_t afio_exticr_index;
  uint32_t afio_exticr_extiX_pY;
  int rising_edge;

  switch (SYS_BOARD_ID)
    {
    case BOARD_ID_FST_01SZ:
    default:
      /* PA3 is connected to a hall sensor DRV5032FA */
      afio_exticr_index = 0;
      afio_exticr_extiX_pY = AFIO_EXTICR1_EXTI3_PA;
      irq_num = EXTI3_IRQ;
      pin_config = 0x0008; /* EXTI_PR_PR3 == EXTI_IMR_MR3 == EXTI_RTSR_TR3 */
      rising_edge = 1;
      break;
    }

  chopstx_claim_irq (intr, irq_num);

  /* Configure EXTI line */
  if (afio_exticr_extiX_pY)
    AFIO->EXTICR[afio_exticr_index] |= afio_exticr_extiX_pY;

  /* Interrupt is masked, now */
  EXTI->IMR &= ~pin_config;

  /* Configure which edge is detected */
  if (rising_edge)
    EXTI->RTSR |= pin_config;
  else
    EXTI->FTSR |= pin_config;
}

void
ackbtn_enable (void)
{
  EXTI->PR |= pin_config;	/* Clear pending interrupt */
  EXTI->IMR |= pin_config;	/* Enable interrupt clearing the mask */
}

void
ackbtn_disable (void)
{
  EXTI->IMR &= ~pin_config;	/* Disable interrupt having the mask */
  EXTI->PR |= pin_config;	/* Clear pending interrupt */
}
