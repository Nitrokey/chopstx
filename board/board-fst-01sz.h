#define BOARD_NAME "FST-01SZ"
#define BOARD_ID    0x7e6fb084
/* echo -n "FST-01SZ" | sha256sum | sed -e 's/^.*\(........\)  -$/\1/' */

#define MCU_STM32F1_GD32F1 1
#define STM32_USBPRE                    STM32_USBPRE_DIV2
#define STM32_ADCPRE                    STM32_ADCPRE_DIV8

#define MCU_STM32F1 1
#define STM32F10X_MD		/* Medium-density device */

#define STM32_PLLXTPRE                  STM32_PLLXTPRE_DIV1
#define STM32_PLLMUL_VALUE              8
#define STM32_HSECLK                    12000000

#define GPIO_LED_BASE   GPIOA_BASE
#define GPIO_LED_SET_TO_EMIT            8
#define GPIO_USB_BASE   GPIOA_BASE
#undef  GPIO_OTHER_BASE

/*
 * Port A setup.
 * PA0  - input with pull-up: AN0 for NeuG
 * PA1  - input with pull-up: AN1 for NeuG
 * PA3  - input with pull-up: Hall sensor output
 * PA8  - Push pull output 10MHz 0 default (LED 1:ON 0:OFF)
 * PA11 - Push Pull output 10MHz 0 default (until USB enabled) (USBDM)
 * PA12 - Push Pull output 10MHz 0 default (until USB enabled) (USBDP)
 * ------------------------ Default
 * PAx  - input with pull-up.
 */
#define VAL_GPIO_LED_ODR            0xFFFFE6FF
#define VAL_GPIO_LED_CRL            0x88888888      /*  PA7...PA0 */
#define VAL_GPIO_LED_CRH            0x88811881      /* PA15...PA8 */

#define RCC_ENR_IOP_EN      RCC_APB2ENR_IOPAEN
#define RCC_RSTR_IOP_RST    RCC_APB2RSTR_IOPARST
