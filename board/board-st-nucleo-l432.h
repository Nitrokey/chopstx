#define BOARD_NAME "ST Nucleo L432"
#define BOARD_ID    0x3a8d5116

/*
 * Please add USB cable to ST Nucleo L432.
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

#define RCC_HSICLK                    48000000

#define GPIO_LED_BASE   GPIOB_BASE
#define GPIO_LED_SET_TO_EMIT            3
#undef  GPIO_USB_BASE		/* No external DISCONNECT/RENUM circuit.  */
#define GPIO_OTHER_BASE GPIOB_BASE

/*
 * Port A setup.
 * PA0  - Input with pull-up                       USART2-CTS
 * PA1  - Alternate function push pull output 2MHz USART2-RTS
 * PA2  - Alternate function push pull output 2MHz USART2-TX
 * PA3  - Input with pull-up                       USART2-RX
 * PA4  - Alternate function push pull output 2MHz USART2-CK
 * PA5  - Push pull output 2MHz (LED 1:ON 0:OFF)
 * PA11 - Push Pull output 10MHz 0 default (until USB enabled) (USBDM) 
 * PA12 - Push Pull output 10MHz 0 default (until USB enabled) (USBDP)
 * ------------------------ Default
 * PAx  - input with pull-up
 */
#define VAL_GPIO_LED_ODR            0xFFFFE7FF

/*
 * Port B setup.
 * PB0  - input with pull-up: AN8 for NeuG
 * PB1  - input with pull-up: AN9 for NeuG
 * ---
 * ---
 * PB4  - Input with pull-up: Card insertion detect: 0 when detected
 * ---
 * PB6  - Output push pull 2MHz: Vcc for card: default 0
 * ---
 * PB8  - Output push pull 2MHz: Vpp for card: default 0
 * PB9  - Output push pull 2MHz: RST for card: default 0
 * PB10 - Alternate function open-drain output 50MHz USART3-TX
 * PB11 - Input with pull-up                         USART3-RX
 * PB12 - Alternate function push pull output  50MHz USART3-CK
 * PB13 - Input with pull-up                         USART3-CTS
 * PB14 - Alternate function push pull output  50MHz USART3-RTS
 * ---
 * ------------------------ Default
 * PBx  - input with pull-up.
 */
#define VAL_GPIO_OTHER_ODR          0xFFFFFCBF
