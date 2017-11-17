struct PWR
{
  volatile uint32_t CR;
  volatile uint32_t CSR;
};
static struct PWR *const PWR = ((struct PWR *)0x40007000);
#define PWR_CR_LPDS 0x0001	/* Low-power deepsleep  */
#define PWR_CR_PDDS 0x0002	/* Power down deepsleep */
#define PWR_CR_CWUF 0x0004	/* Clear wakeup flag    */
