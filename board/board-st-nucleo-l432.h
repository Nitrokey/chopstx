#define BOARD_NAME "ST Nucleo L432"
#define BOARD_ID    0x3a8d5116

/*
 * When using USB, please add USB cable to ST Nucleo L432.
 *
 * At CN4, connect USB cable, only when ST Link is not connected
 *  Vbus RED   -->  4
 *
 * At CN3, connect USB cable
 *  D-   WHITE --> 13 PA11
 *  D+   GREEN -->  5 PA12
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
 * MODER: 10 10 - 10 10 - 10 11 - 10 10    11 11 - 11 11 - 11 10 - 11 11
 *
 * PA2  - USART2-TX: AF7 output push-pull
 * PA8  - USART1-CK: AF7 output push-pull
 * PA9  - USART1-TX: AF7 output(input) Open-drain pull-up
 * PA11 - USBDM:     AF10 input/output
 * PA12 - USBDP:     AF10 input/output
 * PA13 - SWDIO:     AF0
 * PA14 - SWDCLK:    AF0
 * PA15 - USART2-RX: AF3 input
 * ------------------------ Default
 * PAx  - analog input
 */
#define VAL_GPIO_OTHER_MODER   0xAABAFFEF
#define VAL_GPIO_OTHER_OTYPER  0x00000200
#define VAL_GPIO_OTHER_OSPEEDR 0xFFFFFFFF
#define VAL_GPIO_OTHER_PUPDR   0x00040000
#define VAL_GPIO_OTHER_AFRL    0x00000700
#define VAL_GPIO_OTHER_AFRH    0x300AA077

/*
 * Port B setup.
 *
 * MODER: 11 11 - 11 11 - 11 11 - 11 11    11 11 - 11 11 - 01 11 - 11 11
 *
 * PB3  - ON (LED 1:ON 0:OFF)
 * ------------------------ Default
 * PBx  - analog input
 */
#define VAL_GPIO_LED_MODER   0xFFFFFF7F
#define VAL_GPIO_LED_OTYPER  0x00000000
#define VAL_GPIO_LED_OSPEEDR 0x00000000
#define VAL_GPIO_LED_PUPDR   0x00000000
#define VAL_GPIO_LED_AFRL    0x00000000
#define VAL_GPIO_LED_AFRH    0x00000000

#define RCC_AHB2_GPIO   (RCC_AHB2_GPIOA | RCC_AHB2_GPIOB)
