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
struct Uart : FMT::Print {

//-------------|
    private:
//-------------|

                using u32 = uint32_t;

                USART_TypeDef& reg_;

                static inline Uart* instances_[2]; //for isr use

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
//isr added, 'put' changed
static constexpr auto BUFSIZE{ 64 };
u8 buf_[BUFSIZE];
u8 bufIdxIn_;
u8 bufIdxOut_;
volatile u8 bufCount_;


                auto //default 16 sample rate
baudReg         (u32 baud) { reg_.BRR = System::cpuMHz*1000000/baud; }

                auto
txOn            () { reg_.CR1 = 9; } //TE=1,UE=1
                auto
txeIrqOn        () { reg_.CR1 or_eq (1<<7); } //TXEIE=1
                auto
txeIrqOff       () { reg_.CR1 and_eq compl (1<<7); } //TXEIE=0

                auto
writeTxData     (const uint8_t v){ reg_.TDR = v; }
public:
                virtual bool
write           (const char c)
                {
                while( bufCount_ >= BUFSIZE ){} //if buffer full, wait for txe isr to make room in buffer
                buf_[bufIdxIn_] = c;
                if( ++bufIdxIn_ >= BUFSIZE ) bufIdxIn_ = 0;
                { InterruptLock lock; bufCount_++; } //protect increment
                txeIrqOn();
                return true;
                }

//-------------|
    public:
//-------------|

                static auto //static so we can put address into vector table, use instances_ to get object
isr             ()
                {
                Uart* uart = irqActive() == USART1_IRQn ? instances_[0] : instances_[1];
                //not checking ISR.TXE bit (should be 1) as we only use TXEIE so only one way to get here
                //flag is cleared when TDR is written
                if( uart->bufCount_ == 0 ) return uart->txeIrqOff();
                uart->writeTxData( uart->buf_[uart->bufIdxOut_] );
                if( ++uart->bufIdxOut_ >= uart->BUFSIZE ) uart->bufIdxOut_ = 0;
                uart->bufCount_--;
                }

                //struct with info about specific uart (only tx/rx pin)
                using
usartT          = struct {
                USART_TypeDef*  uart;
                PINS::PIN       txPin;
                PINS::ALTFUNC   txAltFunc;
                PINS::PIN       rxPin;
                PINS::ALTFUNC   rxAltFunc;
                void(*rccEnable)();
                };

Uart            (usartT u, u32 baud)
                : reg_(*u.uart)
                {
                if( u.uart == USART1 ) instances_[0] = this; else instances_[1] = this;
                u.rccEnable();
                //first set default state when tx not enabled (input/pullup)
                GpioPin(u.txPin).mode(PINS::INPUT).pull(PINS::PULLUP).altFunc(u.txAltFunc);
                baudReg( baud );
                irqFunction( u.uart == USART1 ? USART1_IRQn : USART2_IRQn, isr );
                txOn();
                }


};
