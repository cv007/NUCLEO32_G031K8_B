//Boards.hpp
#pragma once

#include "MyStm32.hpp"

#include "Gpio.hpp"
#include "Uart.hpp"

/*=============================================================
    Boards
=============================================================*/
struct Boards {

    struct Nucleo32g031 {
        //Uart2, TX=PA2,RX=PA3
        static constexpr Uart::usartT uart{ Uart2_A2A3 };
        //fixed green led- LD3
        GpioPin led{ GpioPin(PINS::PC6).mode(PINS::OUTPUT).lock().off() };
        //board pin names to actual pins
        static constexpr PINS::PIN D[]{ //0-12
            PINS::PB7, PINS::PB6, PINS::PA15, PINS::PB1,
            PINS::PA10, PINS::PA9, PINS::PB0, PINS::PB2,
            PINS::PB8, PINS::PA8, PINS::PB9, PINS::PB5,
            PINS::PB4
        };
        static constexpr PINS::PIN A[]{ //0-7
            PINS::PA0, PINS::PA1, PINS::PA4, PINS::PA5,
            PINS::PA12, PINS::PA11, PINS::PA6, PINS::PA7
        }; 

    };

};

