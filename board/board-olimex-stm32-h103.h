#define BOARD_NAME "Olimex STM32-H103"
#define BOARD_ID    0xf92bb594

#define MCU_STM32F1 1
#define STM32F10X_MD		/* Medium-density device */

#define STM32_PLLXTPRE                  STM32_PLLXTPRE_DIV1
#define STM32_PLLMUL_VALUE              9
#define STM32_HSECLK                    8000000

#define GPIO_LED_BASE   GPIOC_BASE
#define GPIO_LED_CLEAR_TO_EMIT          12
#define GPIO_USB_BASE   GPIOC_BASE
#define GPIO_USB_CLEAR_TO_ENABLE        11
#undef  GPIO_OTHER_BASE

/*
 * Port C setup.
 * PC0  - input with pull-up.  AN10 for NeuG
 * PC1  - input with pull-up.  AN11 for NeuG
 * PC6  - input without pull-up/down
 * PC7  - input without pull-up/down
 * PC11 - Open-drain output 50MHz (USB 0:ON 1:OFF)  default 0
 * PC12 - Push Pull output 50MHz (LED).
 * ------------------------ Default
 * PCx  - input with pull-up
 */
#define VAL_GPIO_LED_ODR            0xFFFFF7FF
#define VAL_GPIO_LED_CRL            0x44888888      /*  PC7...PC0 */
#define VAL_GPIO_LED_CRH            0x88837888      /* PC15...PC8 */

#define RCC_ENR_IOP_EN      RCC_APB2ENR_IOPCEN
#define RCC_RSTR_IOP_RST    RCC_APB2RSTR_IOPCRST
