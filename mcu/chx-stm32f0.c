#include <stdint.h>
#include <mcu/cortex-m.h>
#define MCU_STM32F0
#include <mcu/stm32.h>

extern int chx_allow_sleep;

void
chx_sleep_mode (int how)
{
  PWR->CR |= PWR_CR_CWUF;
  PWR->CR &= ~(PWR_CR_PDDS|PWR_CR_LPDS);

  if (how == 0 || how == 1 /* Sleep only (not deepsleep) */)
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP;
  else
    {			   /* Deepsleep */
      /* how == 2: deepsleep but regulator ON */
      if (how == 3)
	PWR->CR |= PWR_CR_LPDS;	/* regulator low-power mode */
      else if (how == 4)
	PWR->CR |= PWR_CR_PDDS;	/* Power down: All OFF     */

      SCB->SCR |= SCB_SCR_SLEEPDEEP;
    }
}

void __attribute__((naked))
chx_idle (void)
{
  int sleep_enabled;

  for (;;)
    {
      asm ("ldr	%0, %1" : "=r" (sleep_enabled): "m" (chx_allow_sleep));
      if (sleep_enabled)
	{
	  asm volatile ("wfi" : : : "memory");
	  /* NOTE: it never comes here.  Don't add lines after this.  */
	}
    }
}
