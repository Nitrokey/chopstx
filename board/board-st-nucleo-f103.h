#define BOARD_NAME "ST Nucleo F103"
#define BOARD_ID    0x9b87c16d

/*
 * Please add X3 and USB cable to ST Nucleo F103.
 *
 * Solder X3 XTAL of 8MHz (and put C33 and C34 of 22pF).  
 * Solder the bridges for R35 and R37, since it's 0 ohm. 
 *
 * (Optional) Remove SB54 and SB55.
 *
 * At CN10, connect USB cable
 *  Vbus RED   --> 10 NC   ----------> CN7 (6 E5V)
 *  D+   GREEN --> 12 PA12 ---[1K5]--> CN6 (4 3V3)
 *  D-   WHITE --> 14 PA11
 *                 16 PB12 (USART3-CK) ---> smartcard CK
 *                 18
 *  GND  BLACK --> 20 GND
 */

#define MCU_STM32F1 1
#define STM32F10X_MD		/* Medium-density device */

#define STM32_PLLXTPRE                  STM32_PLLXTPRE_DIV1
#define STM32_PLLMUL_VALUE              9
#define STM32_HSECLK                    8000000

#define GPIO_LED_BASE   GPIOA_BASE
#define GPIO_LED_SET_TO_EMIT            5
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
#define VAL_GPIO_LED_CRL            0x882A8AA8      /*  PA7...PA0 */
#define VAL_GPIO_LED_CRH            0x88811888      /* PA15...PA8 */

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
#define VAL_GPIO_OTHER_CRL          0x82888888      /*  PB7...PB0 */
#define VAL_GPIO_OTHER_CRH          0x8B8B8F22      /* PB15...PB8 */

#define RCC_ENR_IOP_EN      (RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN)
#define RCC_RSTR_IOP_RST    (RCC_APB2RSTR_IOPARST | RCC_APB2RSTR_IOPBRST | RCC_APB2RSTR_AFIORST)
