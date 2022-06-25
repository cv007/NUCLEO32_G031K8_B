//stm32g031k8.hpp
#pragma once

/*-------------------------------------------------------------
    MCU specific pin enums, in a namespace so both can keep out
    of global namespace and can use with 'using namespace' if
    want to use enums without having to specify PINS::
--------------------------------------------------------------*/
namespace PINS {

                enum
PIN             {
                PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7,
                PA8, PA9, PA10, PA11, PA12, PA13, PA14, PA15,
                PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7,
                PB8, PB9,
                PC6 = 38, PC14 = 46, PC15,

                SWDIO = PA13, SWCLK = PA14, //+ any alias names
                };

}

/*=============================================================
    Uart instances available for this mcu

    Uartn_TXRX - pin names specified in instance name

    UART1 TX - PA9/AF1, PB6/AF0
          RX - PA10/AF1, PB7/AF0
    A9A10, A9B7, B6A10, B6B7

    UART2 TX - PA2/AF1, PA14/AF1
          RX - PA3/AF1, PA15/AF1
    A2A3, A2A15, A14A3, A14A15

    a little verbose, and would get worse for all the other
    uart pins, but will do for now
=============================================================*/
#include "Uart.hpp"
#include "Gpio.hpp"


static constexpr Uart::usartT Uart1_A9A10 {
    //c++ designated initializers have to be in order
    //just shown here for info
    .uart = USART1,
    .txPin = PINS::PA9,  .txAltFunc = PINS::AF1,
    .rxPin = PINS::PA10, .rxAltFunc = PINS::AF1,
    .rccEnable = []{ RCC->APBENR2 or_eq RCC_APBENR2_USART1EN_Msk; }
};
static constexpr Uart::usartT Uart1_A9B7 {
    USART1,
    PINS::PA9, PINS::AF1,
    PINS::PB7, PINS::AF0,
    []{ RCC->APBENR2 or_eq RCC_APBENR2_USART1EN_Msk; }
};
static constexpr Uart::usartT Uart1_B6A10 {
    USART1,
    PINS::PB6, PINS::AF0,
    PINS::PA10, PINS::AF1,
    []{ RCC->APBENR2 or_eq RCC_APBENR2_USART1EN_Msk; }
};
static constexpr Uart::usartT Uart1_B6B7 {
    USART1,
    PINS::PB6, PINS::AF0,
    PINS::PB7, PINS::AF0,
    []{ RCC->APBENR2 or_eq RCC_APBENR2_USART1EN_Msk; }
};

static constexpr Uart::usartT Uart2_A2A3 {
    USART2,
    PINS::PA2, PINS::AF1,
    PINS::PA3, PINS::AF1,
    []{ RCC->APBENR1 or_eq RCC_APBENR1_USART2EN_Msk; }
};
static constexpr Uart::usartT Uart2_A2A15 {
    USART2,
    PINS::PA2, PINS::AF1,
    PINS::PA15, PINS::AF1,
    []{ RCC->APBENR1 or_eq RCC_APBENR1_USART2EN_Msk; }
};
static constexpr Uart::usartT Uart2_A14A3 {
    USART2,
    PINS::PA14, PINS::AF1,
    PINS::PA3, PINS::AF1,
    []{ RCC->APBENR1 or_eq RCC_APBENR1_USART2EN_Msk; }
};
static constexpr Uart::usartT Uart2_A14A15 {
    USART2,
    PINS::PA14, PINS::AF1,
    PINS::PA15, PINS::AF1,
    []{ RCC->APBENR1 or_eq RCC_APBENR1_USART2EN_Msk; }
};
