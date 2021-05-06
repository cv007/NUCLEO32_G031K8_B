//Uart.hpp
#pragma once

#include "MyStm32.hpp"
#include "Printer.hpp"
#include "Gpio.hpp"

/*=============================================================
    Uart class- quick and simple, tx only,
    inherit Printer for printf style use via .print
=============================================================*/
struct Uart : Printer {

//-------------|
    private:
//-------------|

                USART_TypeDef& reg_;

                auto
isTxFull        (){ return (reg_.ISR bitand (1<<7)) == 0; }

                //blocking
                virtual void
write           (const char c)
                {
                while( isTxFull() ){}
                reg_.TDR = c;
                }

                auto //default 16 sample rate
baudReg         (u32 baud) { reg_.BRR = System::cpuMHz*1000000/baud; }

                auto
txOn            () { reg_.CR1 or_eq 9; } //TE=1,UE=1

//-------------|
    public:
//-------------|

                //struct with info about specific uart (only tx/rx pin)
                using usartT = struct {
                u32             baseAddress;
                PINS::PIN       txPin;
                PINS::ALTFUNC   txAltFunc;
                PINS::PIN       rxPin;
                PINS::ALTFUNC   rxAltFunc;
                void(*rccEnable)();
                };

                // using usartCfgT = struct { usartT uart; u32 baud; };

                II
Uart            (usartT u, u32 baud)
                : reg_( *reinterpret_cast<USART_TypeDef*>(u.baseAddress) )
                {
                u.rccEnable();
                //first set default state when tx not enabled (input/pullup)
                GpioPin(u.txPin).mode(PINS::INPUT).pull(PINS::PULLUP).altFunc(u.txAltFunc);
                baudReg( baud );
                txOn();
                }

};
