#define BOARD_NAME "ST Nucleo L432"
#define BOARD_ID    0x3a8d5116

/*
 * When using USB, please add USB cable to ST Nucleo L432.
 *
 * At CN4, connect USB cable, only when ST Link is not connected
 *  Vbus RED   -->  4
 *
 * At CN3, connect USB cable
 *  D-   GREEN --> 13 PA11
 *  D+   WHITE -->  5 PA12
 *  GND  BLACK -->  4 GND
 */

#define MCU_STM32L4 1

#define GPIO_LED_BASE   GPIOB_BASE
#define GPIO_LED_SET_TO_EMIT            3
#undef  GPIO_USB_BASE		/* No external DISCONNECT/RENUM circuit.  */
#define GPIO_OTHER_BASE GPIOA_BASE

/*
 * Port A setup.
 *
 * MODER: 10 11 - 11 01 - 01 11 - 10 10    11 11 - 11 11 - 11 10 - 11 11
 *
 * PA2  - USART2-TX: AF7
 * PA8  - USART1-CK: AF7
 * PA9  - USART1-TX: AF7 Open-drain pull-up
 * PA11 - Push Pull output medium-speed 0 (until USB enabled) (USBDM: AF10)
 * PA12 - Push Pull output medium-speed 0 (until USB enabled) (USBDP: AF10)
 * PA15 - USART2-RX: AF3
 * ------------------------ Default
 * PAx  - analog input
 */
#define VAL_GPIO_LED_MODER   0xBD7AFFEF
#define VAL_GPIO_LED_OTYPER  0x00000200
#define VAL_GPIO_LED_OSPEEDR 0xFB7FFFFF
#define VAL_GPIO_LED_PUPDR   0x00040000

#define VAL_GPIO_LED_AFRL    0x00000700
#define VAL_GPIO_LED_AFRH    0x30000077

/*
 * Port B setup.
 *
 * MODER: 11 11 - 11 11 - 11 11 - 11 11    11 11 - 11 11 - 01 11 - 11 11
 *
 * PB3  - ON (LED 1:ON 0:OFF)
 * ------------------------ Default
 * PAx  - analog input
 */
#define VAL_GPIO_OTHER_MODER   0xFFFFFF7F
#define VAL_GPIO_OTHER_OTYPER  0x00000000
#define VAL_GPIO_OTHER_OSPEEDR 0x00000000
#define VAL_GPIO_OTHER_PUPDR   0x00000000

#define RCC_PHR_GPIO   (RCC_PHR_GPIOA | RCC_PHR_GPIOB)
