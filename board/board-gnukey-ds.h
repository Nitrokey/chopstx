#define BOARD_NAME "Gnukey DS"
#define BOARD_ID    0x67ee65a3
/* echo -n "Gnukey DS" | sha256sum | sed -e 's/^.*\(........\)  -$/\1/' */

#define MCU_STM32F1 1
#define STM32F10X_MD		/* Medium-density device */

#define STM32_PLLXTPRE                  STM32_PLLXTPRE_DIV1
#define STM32_PLLMUL_VALUE              9 // 8MHz * 9 = 72 MHz
#define STM32_HSECLK                    8000000

#define GPIO_LED_BASE   GPIOA_BASE
#define GPIO_LED_SET_TO_EMIT            3
#define GPIO_USB_BASE   GPIOA_BASE
#undef  GPIO_OTHER_BASE

/*
 * Port A setup.
 * PA0  - input with pull-up: AN0 for NeuG
 * PA1  - input with pull-up: AN1 for NeuG
 * PA2  - input with pull-up: Hall effect sensor output
 * PA3  - Push pull output 10MHz 0 default (LED 1:ON 0:OFF)
 * PA11 - Push Pull output 10MHz 0 default (until USB enabled) (USBDM)
 * PA12 - Push Pull output 10MHz 0 default (until USB enabled) (USBDP)
 * ------------------------ Default
 * PAx  - input with pull-up.
 */
#define VAL_GPIO_LED_ODR            0xFFFFE7F7      /* 0/1 Pull Down/Up */
#define VAL_GPIO_LED_CRL            0x88881888      /*  PA7...PA0 */
#define VAL_GPIO_LED_CRH            0x88811888      /* PA15...PA8 */

#define RCC_ENR_IOP_EN      RCC_APB2ENR_IOPAEN
#define RCC_RSTR_IOP_RST    RCC_APB2RSTR_IOPARST

/*
 * Board specific information other than clock and GPIO initial
 * setting should not be in board-*.h, but each driver should include
 * such specific information by itself.
 */
