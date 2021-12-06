// #include "chip_config.h"
#define BOARD_NAME "NITROKEY-START-G"
#define BOARD_ID    0x8e8266af

//GD32 changes:
//#define MCU_STM32F1_GD32F1      1
// DELIBARATELY_DO_IT_WRONG_START_STOP: GD32 -> 0 STM32 -> 1
#define DELIBARATELY_DO_IT_WRONG_START_STOP (detect_chip()->clock.i_DELIBARATELY_DO_IT_WRONG_START_STOP)
// STM32_USBPRE_DIV1P5: GD32 -> STM32_USBPRE_DIV2 STM32 -> STM32_USBPRE_DIV1P5
#define STM32_USBPRE            (detect_chip()->clock.i_STM32_USBPRE)
// STM32_PLLMUL_VALUE: GD32 -> 8 STM32 -> 6
#define STM32_PLLMUL_VALUE      8
// STM32_ADCPRE: GD32 -> STM32_ADCPRE_DIV8 STM32 -> STM32_ADCPRE_DIV6 ------ unnecessary?
#define STM32_ADCPRE            STM32_ADCPRE_DIV8

#define MCU_STM32F1 1
#define STM32F10X_MD		/* Medium-density device */

#define STM32_PLLXTPRE          STM32_PLLXTPRE_DIV1
#define STM32_HSECLK            12000000

#define GPIO_LED_UNSET          -1
#define GPIO_LED_HW3            7
#define GPIO_LED_HW4            4

#define GPIO_LED_BASE   GPIOA_BASE
#define GPIO_LED_SET_TO_EMIT            GPIO_LED_HW3
#define GPIO_USB_BASE   GPIOA_BASE
#define GPIO_USB_SET_TO_ENABLE          15
#define GPIO_OTHER_BASE GPIOB_BASE

/*
 * Port A setup.
 * PA0 - input with pull-up: AN0 for NeuG
 * PA1 - input with pull-up: AN1 for NeuG
 * PA2 - floating input
 * PA3 - floating input
 * PA4 - Push pull output   (Red LED1 1:ON 0:OFF; HW4)
 * PA5 - floating input
 * PA6 - floating input
 * PA7 - Push pull output   (Red LED1 1:ON 0:OFF; HW3; Blue LED on HW4)
 * PA8 - floating input (smartcard, SCDSA)
 * PA9 - floating input
 * PA10 - floating input
 * PA11 - Push Pull output 10MHz 0 default (until USB enabled) (USBDM)
 * PA12 - Push Pull output 10MHz 0 default (until USB enabled) (USBDP)
 * PA15 - Push pull output  (USB_EN 1:ON 0:OFF)
 * ------------------------ Default
 * PA8  - input with pull-up.
 * PA9  - floating input.
 * PA10 - floating input.
 * PA13 - input with pull-up.
 * PA14 - input with pull-up.
 * PA15 - Push pull output   (USB 1:ON 0:OFF)
 */
#define VAL_GPIO_USB_ODR            0xFFFFE76F
#define VAL_GPIO_USB_CRL            0x34434488      /*  PA7...PA0 */
#define VAL_GPIO_USB_CRH            0x38811444      /* PA15...PA8 */

/*
 * Port B setup.
 * PB0  - Push pull output   (Green LED2 1:ON 0:OFF)
 * ------------------------ Default
 * PBx  - input with pull-up.
 */
//Leaving old port B configuration use with Green LED
//#define VAL_GPIO_LED_ODR            0xFFFFFFFF
//#define VAL_GPIO_LED_CRL            0x88888883      /*  PA7...PA0 */
//#define VAL_GPIO_LED_CRH            0x88888888      /* PA15...PA8 */
//We have to use USB GPIO registers here since LED GPIO are initialized first
//see /mcu/clk_gpio_init-stm32.c line 351
#define VAL_GPIO_LED_ODR            VAL_GPIO_USB_ODR
#define VAL_GPIO_LED_CRL            VAL_GPIO_USB_CRL      /*  PA7...PA0 */
#define VAL_GPIO_LED_CRH            VAL_GPIO_USB_CRH      /* PA15...PA8 */

/*
 * Port B setup.
 * PB7  - input with pull-up   (HW REVISION 0:HW4 1:HW1-3)
 * ------------------------ Default
 * PBx  - input with pull-up.
 */

#define VAL_GPIO_OTHER_ODR            0xFFFFFFFF
#define VAL_GPIO_OTHER_CRL            0x84444444  /*  PA7...PA0 */
#define VAL_GPIO_OTHER_CRH            0x44444444        /* PA15...PA8 */

#define RCC_ENR_IOP_EN      \
	(RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN)
#define RCC_RSTR_IOP_RST    \
	(RCC_APB2RSTR_IOPARST | RCC_APB2RSTR_IOPBRST | RCC_APB2RSTR_AFIORST)

#define AFIO_MAPR_SOMETHING   AFIO_MAPR_SWJ_CFG_JTAGDISABLE
