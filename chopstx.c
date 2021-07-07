/*
 * chopstx.c - Threads and only threads.
 *
 * Copyright (C) 2013, 2014, 2015, 2016, 2017, 2018, 2019
 *               Flying Stone Technology
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

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <chopstx.h>

/*
 * Thread priority: greater (as integer) has higher precedence.
 */
#if !defined(CHX_PRIO_MAIN_INIT)
#define CHX_PRIO_MAIN_INIT 1
#endif
#if !defined(CHX_FLAGS_MAIN)
#define CHX_FLAGS_MAIN 0
#endif

/* Constant for round robin scheduling.  */
#if !defined(PREEMPTION_USEC)
#define PREEMPTION_USEC 1000 /* 1ms */
#endif

#define MAX_PRIO (255+1)

#ifndef MHZ
#define MHZ 72
#endif

typedef void *(voidfunc) (void *);

void __attribute__((weak)) chx_fatal (uint32_t err_code);

/**
 * chx_fatal - Fatal error point.
 * @err_code: Error code
 *
 * When it detects a coding error, this function will be called to
 * stop further execution of code.  It never returns.
 */
void
chx_fatal (uint32_t err_code)
{
  (void)err_code;
  for (;;);
}

/* Include the definition of thread context structure.  */
#ifdef GNU_LINUX_EMULATION
#include "chopstx-gnu-linux.h"
#else
#include "chopstx-cortex-m.h"
#endif

/* ALLOW_SLEEP for the idle thread.  */
int chx_allow_sleep;
static struct chx_spinlock chx_enable_sleep_lock;

struct chx_queue {
  struct chx_qh q;
  struct chx_spinlock lock;
};

/* READY: priority queue. */
static struct chx_queue q_ready;

/* Queue of threads waiting for timer.  */
static struct chx_queue q_timer;

/* Queue of threads which wait for the exit of some thread.  */
static struct chx_queue q_join;

/* Queue of threads which wait for some interrupts.  */
static struct chx_queue q_intr;

/* Forward declaration(s). */
static void chx_request_preemption (uint16_t prio);
static int chx_wakeup (struct chx_pq *p);
static struct chx_thread * chx_timer_insert (struct chx_thread *tp, uint32_t usec);
static uint32_t chx_timer_dequeue (struct chx_thread *tp);



/**************/
static void chx_spin_init (struct chx_spinlock *lk)
{
  (void)lk;
}

static void chx_spin_lock (struct chx_spinlock *lk)
{
  (void)lk;
}

static void chx_spin_unlock (struct chx_spinlock *lk)
{
  (void)lk;
}

/**************/
struct chx_pq {
  struct chx_pq *next, *prev;
  uint32_t                  : 4;
  uint32_t                  : 5;
  uint32_t                  : 6;
  uint32_t flag_is_proxy    : 1;
  uint32_t                  : 8;
  uint32_t prio             : 8;
  struct chx_qh *parent;
  uintptr_t v;
};

struct chx_px {			/* inherits PQ */
  struct chx_pq *next, *prev;
  uint32_t                  : 4;
  uint32_t                  : 5;
  uint32_t                  : 6;
  uint32_t flag_is_proxy    : 1;
  uint32_t                  : 8;
  uint32_t prio             : 8;
  struct chx_qh *parent;
  uintptr_t v;
  struct chx_thread *master;
  uint32_t *counter_p;
  uint16_t *ready_p;
  struct chx_spinlock lock;	/* spinlock to update the COUNTER */
};

struct chx_thread {		/* inherits PQ */
  struct chx_pq *next, *prev;
  uint32_t state            : 4;
  uint32_t flag_detached    : 1;
  uint32_t flag_got_cancel  : 1;
  uint32_t flag_join_req    : 1;
  uint32_t flag_sched_rr    : 1;
  uint32_t flag_cancelable  : 1;
  uint32_t                  : 6;
  uint32_t flag_is_proxy    : 1;
  uint32_t prio_orig        : 8;
  uint32_t prio             : 8;
  struct chx_qh *parent;
  uintptr_t v;
  tcontext_t tc;
  struct chx_mtx *mutex_list;
  struct chx_cleanup *clp;
};


/*
 * Double linked list handling.
 */

static int
ll_empty (struct chx_qh *q)
{
  return q == (struct chx_qh *)q->next;
}

static struct chx_pq *
ll_dequeue (struct chx_pq *pq)
{
  pq->next->prev = pq->prev;
  pq->prev->next = pq->next;
  pq->prev = pq->next = pq;
  return pq;
}

static void
ll_insert (struct chx_pq *pq0, struct chx_qh *q)
{
  struct chx_pq *pq = (struct chx_pq *)q;

  pq0->next = (struct chx_pq *)pq;
  pq0->prev = pq->prev;
  pq->prev->next = (struct chx_pq *)pq0;
  pq->prev = pq0;
}


static struct chx_pq *
ll_pop (struct chx_qh *q)
{
  if (q == (struct chx_qh *)q->next)
    return NULL;

  return ll_dequeue (q->next);
}

static void
ll_prio_push (struct chx_pq *pq0, struct chx_qh *q0)
{
  struct chx_pq *p;

  for (p = q0->next; p != (struct chx_pq *)q0; p = p->next)
    if (p->prio <= pq0->prio)
      break;

  pq0->parent = q0;
  ll_insert (pq0, (struct chx_qh *)p);
}

static void
ll_prio_enqueue (struct chx_pq *pq0, struct chx_qh *q0)
{
  struct chx_pq *p;

  for (p = q0->next; p != (struct chx_pq *)q0; p = p->next)
    if (p->prio < pq0->prio)
      break;

  pq0->parent = q0;
  ll_insert (pq0, (struct chx_qh *)p);
}


/*
 * Thread status.
 */
enum  {
  THREAD_RUNNING=0,
  THREAD_READY,
  THREAD_WAIT_MTX,
  THREAD_WAIT_CND,
  THREAD_WAIT_TIME,
  THREAD_WAIT_EXIT,
  THREAD_WAIT_POLL,
  /**/
  THREAD_EXITED=0x0E,
  THREAD_FINISHED=0x0F
};


static struct chx_thread *
chx_ready_pop (void)
{
  struct chx_thread *tp;

  chx_spin_lock (&q_ready.lock);
  tp = (struct chx_thread *)ll_pop (&q_ready.q);
  if (tp)
    tp->state = THREAD_RUNNING;
  chx_spin_unlock (&q_ready.lock);

  if (tp && tp->flag_sched_rr)
    {
      chx_spin_lock (&q_timer.lock);
      chx_timer_insert (tp, PREEMPTION_USEC);
      chx_spin_unlock (&q_timer.lock);
    }
  return tp;
}


static void
chx_ready_push (struct chx_thread *tp)
{
  chx_spin_lock (&q_ready.lock);
  tp->state = THREAD_READY;
  ll_prio_push ((struct chx_pq *)tp, &q_ready.q);
  chx_spin_unlock (&q_ready.lock);
}


static void
chx_ready_enqueue (struct chx_thread *tp)
{
  chx_spin_lock (&q_ready.lock);
  tp->state = THREAD_READY;
  ll_prio_enqueue ((struct chx_pq *)tp, &q_ready.q);
  chx_spin_unlock (&q_ready.lock);
}

/*
 * Here comes architecture specific code.
 */

#ifdef GNU_LINUX_EMULATION
#include "chopstx-gnu-linux.c"
#else
#include "chopstx-cortex-m.c"
#endif

static void
chx_set_timer (struct chx_thread *tp, uint32_t ticks)
{
  if (tp == (struct chx_thread *)&q_timer.q)
    chx_systick_reload (ticks);
  else
    tp->v = ticks;
}

static struct chx_thread *
chx_timer_insert (struct chx_thread *tp, uint32_t usec)
{
  struct chx_pq *p;
  uint32_t ticks = usec_to_ticks (usec);
  uint32_t next_ticks = chx_systick_get ();

  for (p = q_timer.q.next; p != (struct chx_pq *)&q_timer.q; p = p->next)
    {
      if (ticks < next_ticks)
	{
	  tp->parent = &q_timer.q;
	  ll_insert ((struct chx_pq *)tp, (struct chx_qh *)p);
	  chx_set_timer ((struct chx_thread *)tp->prev, ticks);
	  chx_set_timer (tp, (next_ticks - ticks));
	  break;
	}
      else
	{
	  ticks -= next_ticks;
	  next_ticks = p->v;
	}
    }

  if (p == (struct chx_pq *)&q_timer.q)
    {
      tp->parent = &q_timer.q;
      ll_insert ((struct chx_pq *)tp, (struct chx_qh *)p);
      chx_set_timer ((struct chx_thread *)tp->prev, ticks);
      chx_set_timer (tp, 1);	/* Non-zero for the last entry. */
    }

  return tp;
}


static uint32_t
chx_timer_dequeue (struct chx_thread *tp)
{
  struct chx_thread *tp_prev;
  uint32_t ticks_remained;

  chx_spin_lock (&q_timer.lock);
  ticks_remained = chx_systick_get ();
  tp_prev = (struct chx_thread *)tp->prev;
  if (tp_prev == (struct chx_thread *)&q_timer.q)
    {
      if (tp->next == (struct chx_pq *)&q_timer.q)
	chx_systick_reload (0);                      /* Cancel timer.  */
      else
	chx_systick_reload (ticks_remained + tp->v); /* Update timer.  */
    }
  else
    {
      struct chx_pq *p;

      for (p = q_timer.q.next; p != (struct chx_pq *)tp; p = p->next)
	ticks_remained += p->v;

      tp_prev->v += tp->v;
    }
  ll_dequeue ((struct chx_pq *)tp);
  tp->v = 0;
  chx_spin_unlock (&q_timer.lock);
  return ticks_remained;
}


void
chx_timer_expired (void)
{
  struct chx_thread *tp;
  struct chx_thread *running = chx_running ();
  uint16_t prio = 0;			/* Use uint16_t here. */

  chx_spin_lock (&q_timer.lock);
  if ((tp = (struct chx_thread *)ll_pop (&q_timer.q)))
    {
      uint32_t next_tick = tp->v;

      tp->v = (uintptr_t)0;
      chx_ready_enqueue (tp);
      if (tp == running)	/* tp->flag_sched_rr == 1 */
	prio = MAX_PRIO;
      else
	if ((uint16_t)tp->prio > prio)
	  prio = (uint16_t)tp->prio;

      if (!ll_empty (&q_timer.q))
	{
	  struct chx_thread *tp_next;

	  for (tp = (struct chx_thread *)q_timer.q.next;
	       tp != (struct chx_thread *)&q_timer.q && next_tick == 0;
	       tp = tp_next)
	    {
	      next_tick = tp->v;
	      tp->v = (uintptr_t)0;
	      tp_next = (struct chx_thread *)tp->next;
	      ll_dequeue ((struct chx_pq *)tp);
	      chx_ready_enqueue (tp);
	      if (tp == running)
		prio = MAX_PRIO;
	      else
		if ((uint16_t)tp->prio > prio)
		  prio = (uint16_t)tp->prio;
	    }

	  if (!ll_empty (&q_timer.q))
	    chx_set_timer ((struct chx_thread *)&q_timer.q, next_tick);
	}
    }

  chx_spin_unlock (&q_timer.lock);
  chx_request_preemption (prio);
}


void
chx_systick_init (void)
{
  chx_systick_init_arch ();

  if ((CHX_FLAGS_MAIN & CHOPSTX_SCHED_RR))
    {
      struct chx_thread *running = chx_running ();

      chx_cpu_sched_lock ();
      chx_spin_lock (&q_timer.lock);
      chx_timer_insert (running, PREEMPTION_USEC);
      chx_spin_unlock (&q_timer.lock);
      chx_cpu_sched_unlock ();
    }
}

chopstx_t chopstx_main;

void
chx_init (struct chx_thread *tp)
{
  chx_interrupt_controller_init ();
  chx_init_arch (tp);
  chx_spin_init (&chx_enable_sleep_lock);

  q_ready.q.next = q_ready.q.prev = (struct chx_pq *)&q_ready.q;
  chx_spin_init (&q_ready.lock);
  q_timer.q.next = q_timer.q.prev = (struct chx_pq *)&q_timer.q;
  chx_spin_init (&q_timer.lock);
  q_join.q.next = q_join.q.prev = (struct chx_pq *)&q_join.q;
  chx_spin_init (&q_join.lock);
  q_intr.q.next = q_intr.q.prev = (struct chx_pq *)&q_intr.q;
  chx_spin_init (&q_intr.lock);
  tp->next = tp->prev = (struct chx_pq *)tp;
  tp->mutex_list = NULL;
  tp->clp = NULL;
  tp->state = THREAD_RUNNING;
  tp->flag_got_cancel = tp->flag_join_req = 0;
  tp->flag_cancelable = 1;
  tp->flag_sched_rr = (CHX_FLAGS_MAIN & CHOPSTX_SCHED_RR)? 1 : 0;
  tp->flag_detached = (CHX_FLAGS_MAIN & CHOPSTX_DETACHED)? 1 : 0;
  tp->flag_is_proxy = 0;
  tp->prio_orig = CHX_PRIO_MAIN_INIT;
  tp->prio = 0;
  tp->parent = NULL;
  tp->v = 0;

  if (CHX_PRIO_MAIN_INIT >= CHOPSTX_PRIO_INHIBIT_PREEMPTION)
    chx_cpu_sched_lock ();

  tp->prio = CHX_PRIO_MAIN_INIT;

  chopstx_main = (chopstx_t)tp;
}

#define CHX_SLEEP 0
#define CHX_YIELD 1


/*
 * Wakeup the thread TP.   Called with schedule lock held.
 */
static int
chx_wakeup (struct chx_pq *pq)
{
  int yield = 0;
  struct chx_thread *tp;
  struct chx_thread *running = chx_running ();

  if (pq->flag_is_proxy)
    {
      struct chx_px *px = (struct chx_px *)pq;

      chx_spin_lock (&px->lock);
      (*px->counter_p)++;
      *px->ready_p = 1;
      tp = px->master;
      if (tp->state == THREAD_WAIT_POLL)
	{
	  if (tp->parent == &q_timer.q)
	    tp->v = (uintptr_t)chx_timer_dequeue (tp);
	  else
	    tp->v = (uintptr_t)1;

	  chx_ready_enqueue (tp);
	  if (!running || tp->prio > running->prio)
	    yield = 1;
	}
      chx_spin_unlock (&px->lock);
    }
  else
    {
      tp = (struct chx_thread *)pq;
      tp->v = (uintptr_t)1;
      chx_ready_enqueue (tp);
      if (!running || tp->prio > running->prio)
	yield = 1;
    }

  return yield;
}


/* The RETVAL is saved into ->v.  */
static void __attribute__((noreturn))
chx_exit (void *retval)
{
  struct chx_pq *p;
  struct chx_thread *running = chx_running ();

  chx_cpu_sched_lock ();
  if (running->flag_join_req)
    {		       /* wake up a thread which requests to join */
      chx_spin_lock (&q_join.lock);
      for (p = q_join.q.next; p != (struct chx_pq *)&q_join.q; p = p->next)
	if (p->v == (uintptr_t)running)
	  {			/* should be one at most. */
	    ll_dequeue (p);
	    chx_wakeup (p);
	    break;
	  }
      chx_spin_unlock (&q_join.lock);
    }

  if (running->flag_sched_rr)
    chx_timer_dequeue (running);
  if (running->flag_detached)
    running->state = THREAD_FINISHED;
  else
    running->state = THREAD_EXITED;
  running->v = (uintptr_t)retval;
  chx_sched (CHX_SLEEP);
  /* never comes here. */
  for (;;);
}


/*
 * Lower layer mutex unlocking.  Called with schedule lock held.
 * Return PRIO of the thread which is waken up.
 */
static chopstx_prio_t
chx_mutex_unlock (chopstx_mutex_t *mutex)
{
  struct chx_thread *tp;
  struct chx_thread *running = chx_running ();

  mutex->owner = NULL;
  running->mutex_list = mutex->list;
  mutex->list = NULL;

  tp = (struct chx_thread *)ll_pop (&mutex->q);
  if (!tp)
    return 0;
  else
    {
      uint16_t newprio = running->prio_orig;
      chopstx_mutex_t *m;

      tp->v = (uintptr_t)0;
      chx_ready_enqueue (tp);

      /* Examine mutexes we hold, and determine new priority for running.  */
      for (m = running->mutex_list; m; m = m->list)
	if (!ll_empty (&m->q)
	    && ((struct chx_thread *)(m->q.next))->prio > newprio)
	  newprio = ((struct chx_thread *)m->q.next)->prio;
      /* Then, assign it.  */
      running->prio = newprio;

      return tp->prio;
    }
}

#define CHOPSTX_PRIO_MASK ((1 << CHOPSTX_PRIO_BITS) - 1)

/**
 * chopstx_create - Create a thread
 * @flags_and_prio: Flags and priority
 * @stack_addr: Stack address
 * @stack_size: Size of stack
 * @thread_entry: Entry function of new thread
 * @arg: Argument to the thread entry function
 *
 * Create a thread.  Returns thread ID.
 */
chopstx_t
chopstx_create (uint32_t flags_and_prio,
		uintptr_t stack_addr, size_t stack_size,
		voidfunc thread_entry, void *arg)
{
  struct chx_thread *tp;
  chopstx_prio_t prio = (flags_and_prio & CHOPSTX_PRIO_MASK);
  struct chx_thread *running = chx_running ();

  tp = chopstx_create_arch (stack_addr, stack_size, thread_entry,
			    arg);
  tp->next = tp->prev = (struct chx_pq *)tp;
  tp->mutex_list = NULL;
  tp->clp = NULL;
  tp->state = THREAD_EXITED;
  tp->flag_got_cancel = tp->flag_join_req = 0;
  tp->flag_cancelable = 1;
  tp->flag_sched_rr = (flags_and_prio & CHOPSTX_SCHED_RR)? 1 : 0;
  tp->flag_detached = (flags_and_prio & CHOPSTX_DETACHED)? 1 : 0;
  tp->flag_is_proxy = 0;
  tp->prio_orig = tp->prio = prio;
  tp->parent = NULL;
  tp->v = 0;

  chx_cpu_sched_lock ();
  chx_ready_enqueue (tp);
  if (tp->prio > running->prio)
    chx_sched (CHX_YIELD);
  else
    chx_cpu_sched_unlock ();

  return (chopstx_t)tp;
}

/*
 * Internal timer uses SYSTICK and it has rather smaller upper limit.
 * Thus, we can't let the thread sleep too long, but let it loops.
 *
 * The caller should make a loop with chx_snooze.
 */
#define MAX_USEC_FOR_TIMER (16777215/MHZ) /* SYSTICK is 24-bit. */

/*
 * Sleep for some event (MAX_USEC_FOR_TIMER at max).
 *
 * Returns:
 *         -1 on cancellation of the thread.
 *          0 on timeout.
 *          1 when no sleep is needed any more, or some event occurs.
 */
static int
chx_snooze (uint32_t state, uint32_t *usec_p)
{
  struct chx_thread *running = chx_running ();
  uint32_t usec = *usec_p;
  uint32_t usec0;
  int r;

  if (usec == 0)
    {
      chx_cpu_sched_unlock ();
      return 1;
    }

  usec0 = (usec > MAX_USEC_FOR_TIMER) ? MAX_USEC_FOR_TIMER: usec;
  if (running->flag_sched_rr)
    chx_timer_dequeue (running);

  chx_spin_lock (&q_timer.lock);
  running->state = state;
  chx_timer_insert (running, usec0);
  chx_spin_unlock (&q_timer.lock);
  r = chx_sched (CHX_SLEEP);
  if (r == 0)
    *usec_p -= usec0;
  else if (r > 0)
    {
      *usec_p -= (usec0 - ticks_to_usec (r));
      r = 1;
    }

  return r;
}


static void
chopstx_usec_wait_var (uint32_t *var)
{
  int r = 0;

  do
    {
      chopstx_testcancel ();
      chx_cpu_sched_lock ();
      r = chx_snooze (THREAD_WAIT_TIME, var);
    }
  while (r == 0);
}


/**
 * chopstx_usec_wait - Sleep for micro seconds
 * @usec: number of micro seconds
 *
 * Sleep for @usec.
 */
void
chopstx_usec_wait (uint32_t usec)
{
  chopstx_usec_wait_var (&usec);
}


/**
 * chopstx_mutex_init - Initialize the mutex
 * @mutex: Mutex
 *
 * Initialize @mutex.
 */
void
chopstx_mutex_init (chopstx_mutex_t *mutex)
{
  chx_spin_init (&mutex->lock);
  mutex->q.next = mutex->q.prev = (struct chx_pq *)&mutex->q;
  mutex->list = NULL;
  mutex->owner = NULL;
}


/*
 * Re-queue TP after priority change.
 * Returns a thread which can wake up this thread TP.
 */
static struct chx_thread *
requeue (struct chx_thread *tp)
{
  if (tp->state == THREAD_READY)
    {
      chx_spin_lock (&q_ready.lock);
      ll_prio_enqueue (ll_dequeue ((struct chx_pq *)tp), tp->parent);
      chx_spin_unlock (&q_ready.lock);
    }
  else if (tp->state == THREAD_WAIT_MTX)
    {
      struct chx_mtx *mutex = (struct chx_mtx *)tp->parent;

      chx_spin_lock (&mutex->lock);
      ll_prio_enqueue (ll_dequeue ((struct chx_pq *)tp), tp->parent);
      chx_spin_unlock (&mutex->lock);
      return mutex->owner;
    }
  else if (tp->state == THREAD_WAIT_CND)
    {
      struct chx_cond *cond = (struct chx_cond *)tp->parent;

      chx_spin_lock (&cond->lock);
      ll_prio_enqueue (ll_dequeue ((struct chx_pq *)tp), tp->parent);
      chx_spin_unlock (&cond->lock);
      /* We don't know who can wake up this thread.  */
    }
  else if (tp->state == THREAD_WAIT_EXIT)
    /* Requeue is not needed as waiting for the thread is only by one.  */
    return (struct chx_thread *)tp->v;

  return NULL;
}

/**
 * chopstx_mutex_lock - Lock the mutex
 * @mutex: Mutex
 *
 * Lock @mutex.
 */
void
chopstx_mutex_lock (chopstx_mutex_t *mutex)
{
  struct chx_thread *tp = chx_running ();

  while (1)
    {
      chopstx_mutex_t *m = mutex;
      struct chx_thread *tp0;

      chx_cpu_sched_lock ();
      chx_spin_lock (&m->lock);
      if (m->owner == NULL)
	{
	  /* The mutex is acquired.  */
	  m->owner = tp;
	  m->list = tp->mutex_list;
	  tp->mutex_list = m;
	  chx_spin_unlock (&m->lock);
	  chx_cpu_sched_unlock ();
	  break;
	}

      /* Priority inheritance.  */
      tp0 = m->owner;
      while (tp0 && tp0->prio < tp->prio)
	{
	  tp0->prio = tp->prio;
	  if (tp0->state == THREAD_WAIT_TIME
	      || tp0->state == THREAD_WAIT_POLL)
	    {
	      if (tp0->parent == &q_timer.q)
		tp0->v = (uintptr_t)chx_timer_dequeue (tp0);
	      else
		tp0->v = (uintptr_t)1;

	      chx_ready_enqueue (tp0);
	      tp0 = NULL;
	    }
	  else
	    tp0 = requeue (tp0);
	}

      if (tp->flag_sched_rr)
	chx_timer_dequeue (tp);
      ll_prio_enqueue ((struct chx_pq *)tp, &mutex->q);
      tp->state = THREAD_WAIT_MTX;
      chx_spin_unlock (&mutex->lock);
      chx_sched (CHX_SLEEP);
    }
}


/**
 * chopstx_mutex_unlock - Unlock the mutex
 * @mutex: Mutex
 *
 * Unlock @mutex.
 */
void
chopstx_mutex_unlock (chopstx_mutex_t *mutex)
{
  struct chx_thread *running = chx_running ();
  chopstx_prio_t prio;

  chx_cpu_sched_lock ();
  chx_spin_lock (&mutex->lock);
  prio = chx_mutex_unlock (mutex);
  chx_spin_unlock (&mutex->lock);
  if (prio > running->prio)
    chx_sched (CHX_YIELD);
  else
    chx_cpu_sched_unlock ();
}


/**
 * chopstx_cond_init - Initialize the condition variable
 * @cond: Condition variable
 *
 * Initialize @cond.
 */
void
chopstx_cond_init (chopstx_cond_t *cond)
{
  chx_spin_init (&cond->lock);
  cond->q.next = cond->q.prev = (struct chx_pq *)&cond->q;
}


/**
 * chopstx_cond_wait - Wait on the condition variable
 * @cond: Condition variable
 * @mutex: Associated mutex
 *
 * Wait for @cond with @mutex.
 */
void
chopstx_cond_wait (chopstx_cond_t *cond, chopstx_mutex_t *mutex)
{
  struct chx_thread *tp = chx_running ();
  int r;

  chopstx_testcancel ();
  chx_cpu_sched_lock ();

  if (mutex)
    {
      chx_spin_lock (&mutex->lock);
      chx_mutex_unlock (mutex);
      chx_spin_unlock (&mutex->lock);
    }

  if (tp->flag_sched_rr)
    chx_timer_dequeue (tp);
  chx_spin_lock (&cond->lock);
  ll_prio_enqueue ((struct chx_pq *)tp, &cond->q);
  tp->state = THREAD_WAIT_CND;
  chx_spin_unlock (&cond->lock);
  r = chx_sched (CHX_SLEEP);

  if (mutex)
    chopstx_mutex_lock (mutex);

  if (r < 0)
    chopstx_exit (CHOPSTX_CANCELED);
}


/**
 * chopstx_cond_signal - Wake up a thread waiting on the condition variable
 * @cond: Condition variable
 *
 * Wake up a thread waiting on @cond.
 */
void
chopstx_cond_signal (chopstx_cond_t *cond)
{
  struct chx_pq *p;
  int yield = 0;

  chx_cpu_sched_lock ();
  chx_spin_lock (&cond->lock);
  p = ll_pop (&cond->q);
  if (p)
    yield = chx_wakeup (p);
  chx_spin_unlock (&cond->lock);
  if (yield)
    chx_sched (CHX_YIELD);
  else
    chx_cpu_sched_unlock ();
}


/**
 * chopstx_cond_broadcast - Wake up all waiting on the condition variable
 * @cond: Condition Variable
 *
 * Wake up all threads waiting on @cond.
 */
void
chopstx_cond_broadcast (chopstx_cond_t *cond)
{
  struct chx_pq *p;
  int yield = 0;

  chx_cpu_sched_lock ();
  chx_spin_lock (&cond->lock);
  while ((p = ll_pop (&cond->q)))
    yield |= chx_wakeup (p);
  chx_spin_unlock (&cond->lock);
  if (yield)
    chx_sched (CHX_YIELD);
  else
    chx_cpu_sched_unlock ();
}


static void
chx_cond_hook (struct chx_px *px, struct chx_poll_head *pd)
{
  struct chx_poll_cond *pc = (struct chx_poll_cond *)pd;

  chopstx_testcancel ();

  if (pc->mutex)
    chopstx_mutex_lock (pc->mutex);

  if ((*pc->check) (pc->arg) != 0)
    {
      chx_spin_lock (&px->lock);
      (*px->counter_p)++;
      *px->ready_p = 1;
      chx_spin_unlock (&px->lock);
    }
  else
    { /* Condition doesn't met.
       * Register the proxy to wait for the condition.
       */
      pc->ready = 0;
      chx_cpu_sched_lock ();
      chx_spin_lock (&pc->cond->lock);
      ll_prio_enqueue ((struct chx_pq *)px, &pc->cond->q);
      chx_spin_unlock (&pc->cond->lock);
      chx_cpu_sched_unlock ();
    }

  if (pc->mutex)
    chopstx_mutex_unlock (pc->mutex);
}


/**
 * chopstx_claim_irq - Claim interrupt request to handle
 * @intr: Pointer to INTR structure
 * @irq_num: IRQ Number (hardware specific)
 *
 * Claim interrupt @intr with @irq_num
 */
void
chopstx_claim_irq (chopstx_intr_t *intr, uint8_t irq_num)
{
  int intr_before_claim;

  intr->type = CHOPSTX_POLL_INTR;
  intr->ready = 0;
  intr->irq_num = irq_num;

  chx_cpu_sched_lock ();
  chx_spin_lock (&q_intr.lock);
  intr_before_claim = chx_disable_intr (irq_num);
  chx_clr_intr (irq_num);
  chx_set_intr_prio (irq_num);
  chx_spin_unlock (&q_intr.lock);
  chx_cpu_sched_unlock ();
  if (intr_before_claim)
    intr->ready = 1;
}


static void
chx_intr_hook (struct chx_px *px, struct chx_poll_head *pd)
{
  struct chx_intr *intr = (struct chx_intr *)pd;

  chopstx_testcancel ();
  chx_cpu_sched_lock ();
  px->v = intr->irq_num;
  if (intr->ready)
    {
      chx_spin_lock (&px->lock);
      (*px->counter_p)++;
      chx_spin_unlock (&px->lock);
    }
  else
    {
      chx_spin_lock (&q_intr.lock);
      ll_prio_enqueue ((struct chx_pq *)px, &q_intr.q);
      chx_enable_intr (intr->irq_num);
      chx_spin_unlock (&q_intr.lock);
    }
  chx_cpu_sched_unlock ();
}


/**
 * chopstx_intr_wait - Wait for interrupt request from hardware
 * @intr: Pointer to INTR structure
 *
 * Wait for the interrupt @intr to be occured.
 *
 */
void
chopstx_intr_wait (chopstx_intr_t *intr)
{
  chopstx_poll (NULL, 1, (struct chx_poll_head **)&intr);
}


/**
 * chopstx_intr_done - Finish an IRQ handling
 * @intr: Pointer to INTR structure
 *
 * Finish for the interrupt @intr occurred.
 *
 */
void
chopstx_intr_done (chopstx_intr_t *intr)
{
  chx_dmb ();

  if (intr->ready)
    {
      chx_clr_intr (intr->irq_num);
      intr->ready = 0;
    }
}


/**
 * chopstx_cleanup_push - Register a clean-up
 * @clp: Pointer to clean-up structure
 *
 * Register a clean-up structure.
 */
void
chopstx_cleanup_push (struct chx_cleanup *clp)
{
  struct chx_thread *running = chx_running ();

  clp->next = running->clp;
  running->clp = clp;
}

/**
 * chopstx_cleanup_pop - Release a clean-up
 * @execute: Execute the clen-up function on release
 *
 * Unregister a clean-up structure.  When @execute is non-zero, the
 * clean-up will be executed.
 */
void
chopstx_cleanup_pop (int execute)
{
  struct chx_thread *running = chx_running ();
  struct chx_cleanup *clp = running->clp;

  if (clp)
    {
      running->clp = clp->next;
      if (execute)
	clp->routine (clp->arg);
    }
}


/**
 * chopstx_exit - Terminate the execution of running thread
 * @retval: Return value (to be caught by a joining thread)
 *
 * Calling this function terminates the execution of running thread,
 * after calling clean up functions.  If the calling thread still
 * holds mutexes, they will be released.  This function never
 * returns.
 */
void
chopstx_exit (void *retval)
{
  struct chx_mtx *m, *m_next;
  struct chx_thread *running = chx_running ();
  struct chx_cleanup *clp = running->clp;

  running->clp = NULL;
  while (clp)
    {
      clp->routine (clp->arg);
      clp = clp->next;
    }

  /* Release all mutexes this thread still holds.  */
  for (m = running->mutex_list; m; m = m_next)
    {
      m_next = m->list;

      chx_cpu_sched_lock ();
      chx_spin_lock (&m->lock);
      chx_mutex_unlock (m);
      chx_spin_unlock (&m->lock);
      chx_cpu_sched_unlock ();
    }

  chx_exit (retval);
}


/**
 * chopstx_join - join with a terminated thread
 * @thd: Thread to wait
 * @ret: Pointer to void * to store return value
 *
 * Waits for the thread of @thd to terminate.
 * Returns 0 on success, 1 when waiting is interrupted.
 */
int
chopstx_join (chopstx_t thd, void **ret)
{
  struct chx_thread *running = chx_running ();
  struct chx_thread *tp = (struct chx_thread *)thd;
  int r = 0;

  /*
   * We don't offer deadlock detection.  It's users' responsibility.
   */

  chopstx_testcancel ();

  chx_cpu_sched_lock ();
  if (tp->flag_detached)
    {
      chx_cpu_sched_unlock ();
      chx_fatal (CHOPSTX_ERR_JOIN);
    }

  if (tp->state != THREAD_EXITED)
    {
      struct chx_thread *tp0 = tp;

      if (running->flag_sched_rr)
	chx_timer_dequeue (running);
      chx_spin_lock (&q_join.lock);
      ll_prio_enqueue ((struct chx_pq *)running, &q_join.q);
      running->v = (uintptr_t)tp;
      running->state = THREAD_WAIT_EXIT;
      tp->flag_join_req = 1;

      /* Priority inheritance.  */
      tp0 = tp;
      while (tp0 && tp0->prio < running->prio)
	{
	  tp0->prio = running->prio;
	  tp0 = requeue (tp0);
	}
      chx_spin_unlock (&q_join.lock);
      r = chx_sched (CHX_SLEEP);
    }
  else
    chx_cpu_sched_unlock ();

  if (r < 0)
    chopstx_exit (CHOPSTX_CANCELED);

  if (r == 0)
    {
      tp->state = THREAD_FINISHED;
      if (ret)
	*ret = (void *)tp->v;
    }

  return r;
}


static void
chx_join_hook (struct chx_px *px, struct chx_poll_head *pd)
{
  struct chx_poll_join *pj = (struct chx_poll_join *)pd;
  struct chx_thread *tp = (struct chx_thread *)pj->thd;

  chopstx_testcancel ();
  chx_cpu_sched_lock ();

  if (tp->flag_detached)
    {
      chx_cpu_sched_unlock ();
      chx_fatal (CHOPSTX_ERR_JOIN);
    }

  if (tp->state == THREAD_EXITED)
    {
      chx_spin_lock (&px->lock);
      (*px->counter_p)++;
      *px->ready_p = 1;
      chx_spin_unlock (&px->lock);
    }
  else
    { /* Not yet exited.
       * Register the proxy to wait for TP's exit.
       */
      pj->ready = 0;
      px->v = (uintptr_t)tp;
      chx_spin_lock (&q_join.lock);
      ll_prio_enqueue ((struct chx_pq *)px, &q_join.q);
      chx_spin_unlock (&q_join.lock);
      tp->flag_join_req = 1;
    }
  chx_cpu_sched_unlock ();
}


/**
 * chopstx_cancel - request a cancellation to a thread
 * @thd: Thread to be canceled
 *
 * This function requests a cancellation of a thread @thd.
 * No return value.
 */
void
chopstx_cancel (chopstx_t thd)
{
  struct chx_thread *tp = (struct chx_thread *)thd;
  struct chx_thread *running = chx_running ();

  chx_cpu_sched_lock ();
  tp->flag_got_cancel = 1;
  if (!tp->flag_cancelable)
    {
      chx_cpu_sched_unlock ();
      return;
    }

  /* Cancellation points: cond_wait, usec_wait, join, and poll.  */
  if (tp->state == THREAD_WAIT_CND)
    {
      struct chx_cond *cond = (struct chx_cond *)tp->parent;

      chx_spin_lock (&cond->lock);
      ll_dequeue ((struct chx_pq *)tp);
      chx_spin_unlock (&cond->lock);
    }
  else if (tp->state == THREAD_WAIT_TIME)
    chx_timer_dequeue (tp);
  else if (tp->state == THREAD_WAIT_EXIT)
    {
      chx_spin_lock (&q_join.lock);
      ll_dequeue ((struct chx_pq *)tp);
      chx_spin_unlock (&q_join.lock);
    }
  else if (tp->state == THREAD_WAIT_POLL)
    {
      if (tp->parent == &q_timer.q)
	chx_timer_dequeue (tp);
    }
  else
    {
      chx_cpu_sched_unlock ();
      return;
    }

  tp->v = (uintptr_t)-1;
  chx_ready_enqueue (tp);
  if (tp->prio > running->prio)
    chx_sched (CHX_YIELD);
  else
    chx_cpu_sched_unlock ();
}


/**
 * chopstx_testcancel - catch pending cancellation request
 *
 * Calling chopstx_testcancel creates a cancellation point.
 * No return value.  If the thread is canceled, this function
 * does not return.
 */
void
chopstx_testcancel (void)
{
  struct chx_thread *running = chx_running ();

  if (running->flag_cancelable && running->flag_got_cancel)
    chopstx_exit (CHOPSTX_CANCELED);
}


/**
 * chopstx_setcancelstate - set cancelability state
 * @cancel_disable: 0 to enable cancelation, otherwise disabled.
 *
 * Calling chopstx_setcancelstate sets cancelability state.
 *
 * Returns old state which is 0 when it was enabled.
 */
int
chopstx_setcancelstate (int cancel_disable)
{
  struct chx_thread *running = chx_running ();
  int old_state = !running->flag_cancelable;

  running->flag_cancelable = (cancel_disable == 0);
  chopstx_testcancel ();
  return old_state;
}

static void
chx_proxy_init (struct chx_px *px, uint32_t *cp)
{
  struct chx_thread *running = chx_running ();

  px->next = px->prev = (struct chx_pq *)px;
  px->flag_is_proxy = 1;
  px->prio = running->prio;
  px->parent = NULL;
  px->v = 0;
  px->master = running;
  px->counter_p = cp;
  px->ready_p = NULL;
  chx_spin_init (&px->lock);
}


/**
 * chopstx_poll - wait for condition variable, thread's exit, or IRQ
 * @usec_p: Pointer to usec for timeout.  Forever if NULL.  It is
 * updated on return
 * @n: Number of poll descriptors
 * @pd_array: Pointer to an array of poll descriptor pointer which
 * should be one of:
 *           chopstx_poll_cond_t, chopstx_poll_join_t, or chopstx_intr_t.
 *
 * Returns number of active descriptors.
 */
int
chopstx_poll (uint32_t *usec_p, int n, struct chx_poll_head *const pd_array[])
{
  uint32_t counter = 0;
  int i;
  struct chx_px px[n];
  struct chx_poll_head *pd;
  int r = 0;
  struct chx_thread *running = chx_running ();

  chx_dmb ();
  chopstx_testcancel ();

  for (i = 0; i < n; i++)
    chx_proxy_init (&px[i], &counter);

 again:
  for (i = 0; i < n; i++)
    {
      pd = pd_array[i];
      px[i].ready_p = &pd->ready;
      if (pd->type == CHOPSTX_POLL_COND)
	chx_cond_hook (&px[i], pd);
      else if (pd->type == CHOPSTX_POLL_INTR)
	chx_intr_hook (&px[i], pd);
      else
	chx_join_hook (&px[i], pd);
    }

  chx_cpu_sched_lock ();
  chx_spin_lock (&px->lock);
  if (counter)
    {
      chx_spin_unlock (&px->lock);
      chx_cpu_sched_unlock ();
    }
  else if (usec_p == NULL)
    {
      if (running->flag_sched_rr)
	chx_timer_dequeue (running);

      running->state = THREAD_WAIT_POLL;
      chx_spin_unlock (&px->lock);
      r = chx_sched (CHX_SLEEP);
    }
  else
    {
      chx_spin_unlock (&px->lock);
      chx_cpu_sched_unlock ();
      do
	{
	  chopstx_testcancel ();
	  chx_cpu_sched_lock ();
	  if (counter)
	    {
	      chx_cpu_sched_unlock ();
	      break;
	    }
	  r = chx_snooze (THREAD_WAIT_POLL, usec_p);
	}
      while (r == 0);
    }

  chx_dmb ();
  for (i = 0; i < n; i++)
    {
      pd = pd_array[i];
      chx_cpu_sched_lock ();
      chx_spin_lock (&px[i].lock);
      if (pd->type == CHOPSTX_POLL_COND)
	{
	  struct chx_poll_cond *pc = (struct chx_poll_cond *)pd;

	  if (pc->ready == 0)
	    {
	      chx_spin_lock (&pc->cond->lock);
	      ll_dequeue ((struct chx_pq *)&px[i]);
	      chx_spin_unlock (&pc->cond->lock);
	    }
	  else
	    { /* Check the condition again after woken up.  */
	      if (pc->mutex)
		chopstx_mutex_lock (pc->mutex);

	      if ((*pc->check) (pc->arg) == 0)
		{ /* Condition doesn't met.  */
		  pc->ready = 0;
		  counter--;
		}

	      if (pc->mutex)
		chopstx_mutex_unlock (pc->mutex);
	    }
	}
      else if (pd->type == CHOPSTX_POLL_INTR)
	{
	  struct chx_intr *intr = (struct chx_intr *)pd;

	  if (intr->ready == 0)
	    {
	      chx_spin_lock (&q_intr.lock);
	      ll_dequeue ((struct chx_pq *)&px[i]);
	      chx_spin_unlock (&q_intr.lock);
	      chx_disable_intr (intr->irq_num);
	    }
	}
      else
	{
	  struct chx_poll_join *pj = (struct chx_poll_join *)pd;

	  if (pj->ready == 0)
	    {
	      chx_spin_lock (&q_join.lock);
	      ll_dequeue ((struct chx_pq *)&px[i]);
	      chx_spin_unlock (&q_join.lock);
	    }
	}
      chx_spin_unlock (&px[i].lock);
      chx_cpu_sched_unlock ();
    }

  if (r < 0)
    chopstx_exit (CHOPSTX_CANCELED);

  if (counter == 0 && (usec_p == NULL || *usec_p))
    goto again;

  return counter;
}


/**
 * chopstx_setpriority - change the schedule priority of running thread
 * @prio: priority
 *
 * Change the schedule priority with @prio.
 * Returns the old priority.
 *
 * In general, it is not recommended to use this function because
 * dynamically changing schedule priorities complicates the system.
 * Only a possible valid usage of this function is in the main thread
 * which starts its execution with priority of CHX_PRIO_MAIN_INIT, and
 * let it change its priority after initialization of other threads.
 */
chopstx_prio_t
chopstx_setpriority (chopstx_prio_t prio_new)
{
  struct chx_thread *tp = chx_running ();
  chopstx_prio_t prio_orig, prio_cur;

  chx_cpu_sched_lock ();
  prio_orig = tp->prio_orig;
  prio_cur = tp->prio;

  tp->prio_orig = prio_new;
  if (prio_cur == prio_orig)
    /* No priority inheritance is active.  */
    tp->prio = prio_new;
  else
    /* Priority inheritance is active.  */
    /* In this case, only when new priority is greater, change the
       priority of this thread.  */
    if (prio_new > prio_cur)
      tp->prio = prio_new;

  if (tp->prio < prio_cur)
    chx_sched (CHX_YIELD);
  else if (tp->prio < CHOPSTX_PRIO_INHIBIT_PREEMPTION)
    chx_cpu_sched_unlock ();

  return prio_orig;
}


/**
 * chopstx_conf_idle - Configure IDLE thread
 * @enable_sleep: Enable sleep on idle or not
 *
 * If @enable_sleep is > 0, allow sleep for the idle thread.
 *
 * Behavior of @enable_sleep >= 1 depends on MCU.
 *
 * For STM32F0, 1 for Sleep (CPU clock OFF only), 2 for Stop (Wakeup
 * by EXTI, voltage regulator on), 3 for Stop (Wakeup by EXTI, voltage
 * regulator low-power), 4 for Standby (Wakeup by RESET, voltage
 * regulator off).
 *
 * For STM32F103, 1 for normal sleep, and 2 for sleep with lower 8MHz
 * clock.
 *
 * Return previous value of @enable_sleep.
 */
extern void chx_sleep_mode (int enable_sleep);

int
chopstx_conf_idle (int enable_sleep)
{
  int r;

  chx_spin_lock (&chx_enable_sleep_lock);
  r = chx_allow_sleep;
  chx_sleep_mode (enable_sleep);
  chx_allow_sleep = enable_sleep;
  chx_spin_unlock (&chx_enable_sleep_lock);

  return r;
}
