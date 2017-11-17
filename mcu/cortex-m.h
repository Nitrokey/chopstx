/* System Control Block */
struct SCB
{
  volatile uint32_t CPUID;
  volatile uint32_t ICSR;
  volatile uint32_t VTOR;
  volatile uint32_t AIRCR;
  volatile uint32_t SCR;
  volatile uint32_t CCR;
  volatile uint8_t  SHP[12];
  volatile uint32_t SHCSR;
  volatile uint32_t CFSR;
  volatile uint32_t HFSR;
  volatile uint32_t DFSR;
  volatile uint32_t MMAR;
  volatile uint32_t BFAR;
  volatile uint32_t AFSR;
  volatile uint32_t PFR[2];
  volatile uint32_t DFR;
  volatile uint32_t AFR;
  volatile uint32_t MMFR[4];
  volatile uint32_t ISAR[5];
};

#define SCS_BASE 0xE000E000
#define SCB_BASE (SCS_BASE + 0x0D00)
static struct SCB *const SCB = (struct SCB *)SCB_BASE;
#define SCB_SCR_SLEEPDEEP (1 << 2)
#define SCB_AIRCR_SYSRESETREQ 0x04
