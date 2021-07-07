#define PERIPH_BASE	0x40000000
#define APB1PERIPH_BASE PERIPH_BASE
#define APB2PERIPH_BASE	(PERIPH_BASE + 0x10000)
#define AHB1PERIPH_BASE	(PERIPH_BASE + 0x20000)
#define AHB2PERIPH_BASE	(PERIPH_BASE + 0x08000000)

struct RCC {
  volatile uint32_t CR;
  volatile uint32_t ICSCR;
  volatile uint32_t CFGR;
  volatile uint32_t PLLCFGR;

  volatile uint32_t PLLSAI1CFGR;
  volatile uint32_t RESERVED0;
  volatile uint32_t CIER;
  volatile uint32_t CIFR;

  volatile uint32_t CICR;
  volatile uint32_t RESERVED1;
  volatile uint32_t AHB1RSTR;
  volatile uint32_t AHB2RSTR;

  volatile uint32_t AHB3RSTR;
  volatile uint32_t RESERVED2;
  volatile uint32_t APB1RSTR1;
  volatile uint32_t APB1RSTR2;

  volatile uint32_t APB2RSTR;
  volatile uint32_t RESERVED3;
  volatile uint32_t AHB1ENR;
  volatile uint32_t AHB2ENR;

  volatile uint32_t AHB3ENR;
  volatile uint32_t RESERVED4;
  volatile uint32_t APB1ENR1;
  volatile uint32_t APB1ENR2;

  volatile uint32_t APB2ENR;
  volatile uint32_t RESERVED5;
  volatile uint32_t AHB1SMENR;
  volatile uint32_t AHB2SMENR;

  volatile uint32_t AHB3SMENR;
  volatile uint32_t RESERVED6;
  volatile uint32_t APB1SMENR1;
  volatile uint32_t APB1SMENR2;

  volatile uint32_t APB2SMENR;
  volatile uint32_t RESERVED7;
  volatile uint32_t CCIPR;
  volatile uint32_t RESERVED8;

  volatile uint32_t BDCR;
  volatile uint32_t CSR;
  volatile uint32_t CRRCR;
  volatile uint32_t CCIPR2;
};

#define RCC_BASE		(AHB1PERIPH_BASE + 0x1000)
static struct RCC *const RCC = (struct RCC *)RCC_BASE;

#define RCC_AHB2_GPIOA       0x00000001
#define RCC_AHB2_GPIOB       0x00000002
#define RCC_AHB2_GPIOC       0x00000004
#define RCC_AHB2_GPIOD       0x00000008
#define RCC_AHB2_GPIOE       0x00000010
#define RCC_AHB2_GPIOH       0x00000080

#define RCC_APB1_1_USB         (1 << 26)
#define RCC_APB1_1_CRS         (1 << 24)


struct PWR
{
  volatile uint32_t CR1;
  volatile uint32_t CR2;
  volatile uint32_t CR3;
  volatile uint32_t CR4;
  volatile uint32_t SR1;
  volatile uint32_t SR2;
  volatile uint32_t SCR;
  volatile uint32_t PUCRA;
  volatile uint32_t PDCRA;
  volatile uint32_t PUCRB;
  volatile uint32_t PDCRB;
  volatile uint32_t PUCRC;
  volatile uint32_t PDCRC;
  volatile uint32_t PUCRD;
  volatile uint32_t PDCRD;
  volatile uint32_t PUCRE;
  volatile uint32_t PDCRE;
  volatile uint32_t PUCRH;
  volatile uint32_t PDCRH;
};
static struct PWR *const PWR = ((struct PWR *)0x40007000);

struct GPIO {
  volatile uint32_t MODER;
  volatile uint32_t OTYPER;
  volatile uint32_t OSPEEDR;
  volatile uint32_t PUPDR;
  volatile uint32_t IDR;
  volatile uint32_t ODR;
  volatile uint32_t BSRR;
  volatile uint32_t LCKR;
  volatile uint32_t AFRL;
  volatile uint32_t AFRH;
  volatile uint32_t BRR;
};

#define GPIOA_BASE	(AHB2PERIPH_BASE)
static struct GPIO *const GPIOA	= (struct GPIO *)GPIOA_BASE;
#define GPIOB_BASE	(AHB2PERIPH_BASE + 0x0400)
static struct GPIO *const GPIOB	= (struct GPIO *)GPIOB_BASE;


struct FLASH {
  volatile uint32_t ACR;
  volatile uint32_t PDKEYR;
  volatile uint32_t KEYR;
  volatile uint32_t OPTKEYR;
  volatile uint32_t SR;
  volatile uint32_t CR;
  volatile uint32_t ECCR;
  volatile uint32_t RESERVED;
  volatile uint32_t OPTR;
  volatile uint32_t PCROP1SR;
  volatile uint32_t PCROP1ER;
  volatile uint32_t WRP1AR;
  volatile uint32_t WRP1BR;
};

#define FLASH_R_BASE	(AHB1PERIPH_BASE + 0x2000)
static struct FLASH *const FLASH = (struct FLASH *)FLASH_R_BASE;

struct USB_STM32L4 {
  volatile uint16_t LPMCSR;
  volatile uint16_t reserved0;
  volatile uint16_t BCDR;
  volatile uint16_t reserved1;
};

#define USB_STM32L4_BASE  (0x40006854UL)
static struct USB_STM32L4 *const USB_STM32L4 = (struct USB_STM32L4 *)USB_STM32L4_BASE;

struct SYSCFG {
  volatile uint32_t MEMRMP;
  volatile uint32_t CFGR1;
  volatile uint32_t EXTICR[4];
  volatile uint32_t SCSR;
  volatile uint32_t CFGR2;
  volatile uint32_t SWPR;
  volatile uint32_t SKR;
};

#define SYSCFG_BASE  (APB2PERIPH_BASE+0UL)
static struct SYSCFG *const SYSCFG = (struct SYSCFG *)SYSCFG_BASE;

#define EXTI0_IRQ    6
#define SYSCFG_EXTICR1_EXTI0_PA 0

struct EXTI {
  union {
    volatile uint32_t IMR1;
    volatile uint32_t IMR;
  };
  union {
    volatile uint32_t EMR1;
    volatile uint32_t EMR;
  };
  union {
    volatile uint32_t RTSR1;
    volatile uint32_t RTSR;
  };
  union {
    volatile uint32_t FTSR1;
    volatile uint32_t FTSR;
  };
  union {
    volatile uint32_t SWIER1;
    volatile uint32_t SWIER;
  };
  union {
    volatile uint32_t PR1;
    volatile uint32_t PR;
  };
  volatile uint32_t reserved;
  volatile uint32_t IMR2;
  volatile uint32_t EMR2;
  volatile uint32_t RTSR2;
  volatile uint32_t FTSR2;
  volatile uint32_t SWIER2;
  volatile uint32_t PR2;
};

#define EXTI_BASE (APB2PERIPH_BASE+0x0400)
static struct EXTI *const EXTI = (struct EXTI *)EXTI_BASE;
