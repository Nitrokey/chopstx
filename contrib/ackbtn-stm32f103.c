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
 * recipients of GNU GPL by a written offer.
 *
 */

#include <stdint.h>
#include <string.h>
#include <chopstx.h>
#include <mcu/stm32f103.h>

#include "board.h"
#include "sys.h"

/*
 * All EXTI registers (EXTI_IMR, EXTI_EMR, EXTI_PR , EXTI_RTSR, and
 * EXTI_FTSR) have same structure, where each bit of X is used for
 * line X, from 0 up to 19.
 *
 * We use 31-bit of PIN_CONFIG to represent if it's for rising edge or
 * falling edge.
 */
static uint32_t pin_config;
#define PINCFG_EDGE        0x80000000
#define PINCFG_EDGE_RISING PINCFG_EDGE

void
ackbtn_init (chopstx_intr_t *intr)
{
  uint8_t irq_num;
  uint32_t afio_exticr_index;
  uint32_t afio_exticr_extiX_pY;

  switch (SYS_BOARD_ID)
    {
    case BOARD_ID_FST_01:
    case BOARD_ID_FST_01G:
    case BOARD_ID_GNUKEY_DS:
      /* PA2 can be connected to a hall sensor or a switch */
      afio_exticr_index = 0;
      afio_exticr_extiX_pY = AFIO_EXTICR1_EXTI2_PA;
      irq_num = EXTI2_IRQ;
      pin_config = 0x0004; /* EXTI_PR_PR2 == EXTI_IMR_MR2 == EXTI_RTSR_TR2 */
      pin_config |= PINCFG_EDGE_RISING;
      break;

    case BOARD_ID_FST_01SZ:
    default:
      /* PA3 is connected to a hall sensor DRV5032FA */
      afio_exticr_index = 0;
      afio_exticr_extiX_pY = AFIO_EXTICR1_EXTI3_PA;
      irq_num = EXTI3_IRQ;
      pin_config = 0x0008; /* EXTI_PR_PR3 == EXTI_IMR_MR3 == EXTI_RTSR_TR3 */
      pin_config |= PINCFG_EDGE_RISING;
      break;
    }

  /* Configure EXTI line */
  if (afio_exticr_extiX_pY)
    AFIO->EXTICR[afio_exticr_index] |= afio_exticr_extiX_pY;

  /* Interrupt is masked, now */
  EXTI->IMR &= ~(pin_config & ~PINCFG_EDGE);

  chopstx_claim_irq (intr, irq_num);
}

void
ackbtn_enable (void)
{
  /* Clear pending interrupt */
  EXTI->PR |= (pin_config & ~PINCFG_EDGE);
  /* Enable interrupt, clearing the mask */
  EXTI->IMR |= (pin_config & ~PINCFG_EDGE);

  /* Configure which edge is detected */
  if ((pin_config & PINCFG_EDGE))
    EXTI->RTSR |= (pin_config & ~PINCFG_EDGE);
  else
    EXTI->FTSR |= (pin_config & ~PINCFG_EDGE);
}

void
ackbtn_disable (void)
{
  /* Disable interrupt having the mask */
  EXTI->IMR &= ~(pin_config & ~PINCFG_EDGE);
  /* Clear pending interrupt */
  EXTI->PR |= (pin_config & ~PINCFG_EDGE);

  /* Disable edge detection */
  EXTI->RTSR &= ~(pin_config & ~PINCFG_EDGE);
  EXTI->FTSR &= ~(pin_config & ~PINCFG_EDGE);
}
