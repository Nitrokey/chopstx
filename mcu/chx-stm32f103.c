#include <stdint.h>
#include <mcu/stm32f103.h>

extern int chx_allow_sleep;

static void
configure_clock (uint32_t cfg_sw)
{
  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_MASK) | cfg_sw;
  while ((RCC->CFGR & RCC_CFGR_SWS) != (cfg_sw << 2))
    ;
}

/*
 * When HOW=0 or HOW=1, clock is PLL (72MHz).
 * When HOW=2, clock will be HSI (8MHz) on sleep.
 *
 * With HSI clock, it can achieve lower power consumption.
 *
 * Implementation note: Deepsleep is only useful with RTC, Watch Dog,
 * or WKUP pin.  We can't use deepsleep for USB, it never wakes up.
 *
 */
void
chx_sleep_mode (int how)
{
  if (how == 0 || how == 1)
    configure_clock (RCC_CFGR_SW_PLL);

  /* how == 2: Defer setting to 8MHz clock to the idle function */
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
	  asm volatile ("cpsid	i" : : : "memory");
	  if (sleep_enabled == 1)
	    {
	      /* Allow JTAG/SWD access on sleep.  */
	      DBGMCU->CR |= DBG_SLEEP;
	    }
	  else if (sleep_enabled == 2)
	    {
	      DBGMCU->CR &= ~DBG_SLEEP; /* Disable HCLK on sleep */
	      configure_clock (RCC_CFGR_SW_HCI);
	    }
	  asm volatile ("cpsie	i" : : : "memory");

	  asm volatile ("wfi" : : : "memory");
	  /* NOTE: it never comes here.  Don't add lines after this.  */
	}
    }
}
