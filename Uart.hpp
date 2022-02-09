#pragma once //Uart.hpp

#include "MyStm32.hpp"
#include "Format.hpp"
#include "Gpio.hpp"
#include "Util.hpp"
using namespace UTIL;

/*=============================================================
    Uart class- quick and simple, tx only,
    inherit Format for cout style use
=============================================================*/
struct Uart : Format {

//-------------|
    private:
//-------------|

                using u32 = uint32_t;

                USART_TypeDef& reg_;

//                 auto
// isTxFull        (){ return (reg_.ISR bitand (1<<7)) == 0; }
//
//                 //blocking
//                 virtual bool
// put             (const char c)
//                 {
//                 while( isTxFull() ){}
//                 reg_.TDR = c;
//                 return true;
//                 }

//changed to buffered tx to test, above commented out, below buffer added
//isr added, put changed
//this will require external code to 'hookup' the isr function to the irq vector
//as this class is not static so we ultimately need a way to get the class instance
//such as-
//      irqFunction( uart.irqN, []{ uart.isr(); } );
//where the instance is known and can call the isr method with no cpatures

static constexpr auto BUFSIZE{ 64 };
u8 buf_[BUFSIZE];
u8 bufIdxIn_;
u8 bufIdxOut_;
volatile u8 bufCount_;

public:
                auto
isr             ()
                {
                //not checking ISR.TXE bit (should be 1) as we only use TXEIE so only one way to get here
                //flag is cleared when TDR is written
                if( bufCount_ == 0 ) { reg_.CR1 and_eq compl (1<<7); return; } //TXEIE=0, done
                reg_.TDR = buf_[bufIdxOut_];
                if( ++bufIdxOut_ >= BUFSIZE ) bufIdxOut_ = 0;
                bufCount_--;
                }

                //so can get our irq vector number
                IRQn_Type      irqN;

private:

                virtual bool
put             (const char c)
                {
                while( bufCount_ >= BUFSIZE ){} //if buffer full, wait for txe isr to make room in buffer
                buf_[bufIdxIn_] = c;
                if( ++bufIdxIn_ >= BUFSIZE ) bufIdxIn_ = 0;
                { InterruptLock lock; bufCount_++; } //protect increment
                reg_.CR1 or_eq (1<<7); //TXEIE=1
                return true;
                }

                auto //default 16 sample rate
baudReg         (u32 baud) { reg_.BRR = System::cpuMHz*1000000/baud; }

                auto
txOn            () { reg_.CR1 = 9; } //TE=1,UE=1


//-------------|
    public:
//-------------|

                //struct with info about specific uart (only tx/rx pin)
                using
usartT          = struct {
                u32             baseAddress;
                PINS::PIN       txPin;
                PINS::ALTFUNC   txAltFunc;
                PINS::PIN       rxPin;
                PINS::ALTFUNC   rxAltFunc;
                void(*rccEnable)();
                IRQn_Type       irqN;
                };

                // using usartCfgT = struct { usartT uart; u32 baud; };

                II
Uart            (usartT u, u32 baud)
                : reg_( *reinterpret_cast<USART_TypeDef*>(u.baseAddress) ), irqN(u.irqN)
                {
                u.rccEnable();
                //first set default state when tx not enabled (input/pullup)
                GpioPin(u.txPin).mode(PINS::INPUT).pull(PINS::PULLUP).altFunc(u.txAltFunc);
                baudReg( baud );
                txOn();
                }


};
