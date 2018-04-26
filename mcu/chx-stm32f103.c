#include <stdint.h>
#include <mcu/stm32f103.h>
#include "board.h"

extern int chx_allow_sleep;

#define STM32_PLLSRC		STM32_PLLSRC_HSE
#define STM32_PLLMUL		((STM32_PLLMUL_VALUE - 2) << 18)
#ifndef STM32_ADCPRE
#define STM32_ADCPRE		STM32_ADCPRE_DIV6
#endif
#ifndef STM32_USBPRE
#define STM32_USBPRE		STM32_USBPRE_DIV1P5
#endif

static void
configure_clock (int high)
{
  uint32_t cfg_sw;
  uint32_t cfg;

  if (high)
    {
      cfg = STM32_MCO_NOCLOCK | STM32_USBPRE
	| STM32_PLLMUL | STM32_PLLXTPRE	| STM32_PLLSRC
	| STM32_ADCPRE | STM32_PPRE2_DIV1
	| STM32_PPRE1_DIV2 | STM32_HPRE_DIV1;

      cfg_sw = RCC_CFGR_SW_PLL;
    }
  else
    {
      cfg = STM32_MCO_NOCLOCK | STM32_USBPRE
	| STM32_PLLMUL | STM32_PLLXTPRE	| STM32_PLLSRC
	| STM32_ADCPRE_DIV8 | STM32_PPRE2_DIV16
	| STM32_PPRE1_DIV16 | STM32_HPRE_DIV8;

      cfg_sw = RCC_CFGR_SW_HSI;
    }

  RCC->CFGR = cfg | cfg_sw;
  while ((RCC->CFGR & RCC_CFGR_SWS) != (cfg_sw << 2))
    ;
}

/*
 * When HOW=0 or HOW=1, SYSCLK is PLL (72MHz).
 * When HOW=2, SYSCLK will be 1MHz with HSI (8MHz) on sleep.
 *
 * With lower clock, it can achieve lower power consumption.
 *
 * Implementation note: Deepsleep is only useful with RTC, Watch Dog,
 * or WKUP pin.  We can't use deepsleep for USB, it never wakes up.
 *
 */
void
chx_sleep_mode (int how)
{
  if (how == 0 || how == 1)
    configure_clock (1);

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
	      configure_clock (0);
	    }
	  asm volatile ("cpsie	i" : : : "memory");

	  asm volatile ("wfi" : : : "memory");
	  /* NOTE: it never comes here.  Don't add lines after this.  */
	}
    }
}
