/*
 * bb.c - Bounded Buffer
 *
 * Copyright (C) 2017  Flying Stone Technology
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
#include <bb.h>

void
bb_init (struct bb *bb, uint32_t items)
{
  bb->items = items;
  chopstx_cond_init (&bb->cond);
  chopstx_mutex_init (&bb->mutex);
}

void
bb_get (struct bb *bb)
{
  chopstx_mutex_lock (&bb->mutex);
  while (bb->items == 0)
    chopstx_cond_wait (&bb->cond, &bb->mutex);
  bb->items--;
  chopstx_mutex_unlock (&bb->mutex);
}

void
bb_put (struct bb *bb)
{
  chopstx_mutex_lock (&bb->mutex);
  bb->items++;
  chopstx_cond_signal (&bb->cond);
  chopstx_mutex_unlock (&bb->mutex);
}

static int
bb_check (void *arg)
{
  struct bb *bb = arg;

  return bb->items != 0;
}

void
bb_prepare_poll (struct bb *bb, chopstx_poll_cond_t *p)
{
  poll_desc->type = CHOPSTX_POLL_COND;
  poll_desc->ready = 0;
  poll_desc->cond = &bb->cond;
  poll_desc->mutex = &bb->mutex;
  poll_desc->check = bb_check;
  poll_desc->arg = ev;
}
