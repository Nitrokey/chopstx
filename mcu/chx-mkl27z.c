extern int chx_allow_sleep;

void
chx_sleep_mode (int enable_sleep)
{
  (void)enable_sleep;
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
