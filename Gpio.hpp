//Gpio.hpp
#pragma once

#include "MyStm32.hpp"

/*--------------------------------------------------------------
    enum additions to the PINS namespace for GpioPin use
--------------------------------------------------------------*/
namespace PINS {

                enum
MODE            { INPUT, OUTPUT, ALTERNATE, ANALOG };
                enum
OTYPE           { PUSHPULL, ODRAIN };
                enum
PULL            { NOPULL, PULLUP, PULLDOWN };
                enum
SPEED           { SPEED0, SPEED1, SPEED2, SPEED3 }; //VLOW to VHIGH
                enum
ALTFUNC         { AF0, AF1, AF2, AF3, AF4, AF5, AF6, AF7 };
                enum
INVERT          { HIGHISON, LOWISON };

};

/*--------------------------------------------------------------
    GpioPort class
--------------------------------------------------------------*/
struct GpioPort {

//-------------|
    protected:
//-------------|

                u8 port_; //port number(letter)

//-------------|
    public:
//-------------|

                GPIO_TypeDef& reg_;

                II
GpioPort        (PINS::PIN pin)
                : port_( pin/16 ),
                  reg_( *(reinterpret_cast<GPIO_TypeDef*>( GPIOA_BASE + (GPIOB_BASE-GPIOA_BASE)*(pin/16)) ) )
                {
                }

                II auto
enable          () { RCC->IOPENR or_eq (1<<port_); }

                //lock pin(s) on this port (bitmask)
                II auto
lock            (u16 bm)
                {
                u32 vL = (1<<16) bitor bm;
                reg_.LCKR = vL;
                reg_.LCKR = bm;
                reg_.LCKR = vL;
                return (reg_.LCKR bitand vL) == vL;
                }

};

/*--------------------------------------------------------------
    GpioPin class
--------------------------------------------------------------*/
struct GpioPin : GpioPort {

//-------------|
    private:
//-------------|

                u8 pin_;        //0-15
                u16 pinmask_;   //for bsr/bsrr
                bool invert_;   //so can do on/off

//-------------|
    public:
//-------------|

                //no init, rcc clock enabled by default unless not wanted
                II
GpioPin         (PINS::PIN pin, PINS::INVERT inv = PINS::HIGHISON, bool clken = true)
                : GpioPort(pin), pin_(pin%16), pinmask_(1<<(pin%16)), invert_(inv)
                {
                if( clken ) enable();
                }

// properties

                II auto
lock            () { GpioPort::lock(pinmask_); return *this; }  //lock return value ignored

                II auto
mode            (PINS::MODE e)
                {
                reg_.MODER = (reg_.MODER bitand compl (3<<(2*pin_))) bitor (e<<(2*pin_));
                return *this;
                }

                II auto
outType         (PINS::OTYPE e)
                {
                if( e == PINS::ODRAIN ) reg_.OTYPER or_eq pinmask_;
                else reg_.OTYPER and_eq compl pinmask_;
                return *this;
                }

                II auto
pull            (PINS::PULL e)
                {
                auto bp = 2*pin_;
                auto bmclr = compl (3<<bp);
                auto bmset = e<<bp;
                reg_.PUPDR = (reg_.PUPDR bitand bmclr) bitor bmset;
                return *this;
                }

                II auto
speed           (PINS::SPEED e)
                {
                auto bp = 2*pin_;
                auto bmclr = compl (3<<bp);
                auto bmset = e<<bp;
                reg_.OSPEEDR = (reg_.OSPEEDR bitand bmclr) bitor bmset;
                return *this;
                }

                II auto
altFunc         (PINS::ALTFUNC e)
                {
                auto& r = reg_.AFR[pin_>7 ? 1 : 0];
                auto bp = 4*(pin_ bitand 7);
                auto bmclr = compl (15<<bp);
                auto bmset = e<<bp;
                r = (r bitand bmclr) bitor bmset;
                mode( PINS::ALTERNATE );
                return *this; 
                }

// irq

                //get rising flag, clear if set
                II bool
isFlagRise      ()
                {
                auto bm =EXTI->RPR1 bitand pinmask_;
                EXTI->RPR1 = bm;
                return bm;
                }

                //get falling flag, clear if set
                II bool
isFlagFall      ()
                {
                auto bm =EXTI->FPR1 bitand pinmask_;
                EXTI->FPR1 = bm;
                return bm;
                }

                //get any flag, clear if set
                II bool
isFlag          ()
                {
                bool r = isFlagRise();
                bool f = isFlagFall();
                return r or f;
                }

                II auto
irqOff          () 
                { 
                EXTI->IMR1 and_eq compl pinmask_;
                return *this; 
                }

                II auto
irqOn           () 
                { 
                RCC->APBENR2 or_eq RCC_APBENR2_SYSCFGEN; //so can read EXTI_LINEx
                //set our port to use this pin irq
                auto& r = EXTI->EXTICR[pin_/4];
                auto bp = (pin_ bitand 3)*8; //0,8,16,24
                auto bmset = port_<<bp;
                auto bmclr = compl (0xFF<<bp);
                r = (r bitand bmclr) bitor bmset;
                isFlag(); //clear flags
                EXTI->IMR1 or_eq pinmask_; 
                return *this; 
                } 

                II auto
irqNoEdges      () 
                { 
                EXTI->FTSR1 and_eq compl pinmask_; 
                EXTI->RTSR1 and_eq compl pinmask_;
                return *this; 
                }

                II auto
irqRising       () 
                { 
                EXTI->FTSR1 and_eq compl pinmask_; 
                EXTI->RTSR1 or_eq pinmask_; 
                return *this; 
                }

                II auto
irqFalling      () 
                { 
                EXTI->RTSR1 and_eq compl pinmask_; 
                EXTI->FTSR1 or_eq pinmask_; 
                return *this; 
                }

                II auto
irqBothEdges    () 
                { 
                EXTI->FTSR1 or_eq pinmask_; 
                EXTI->RTSR1 or_eq pinmask_; 
                return *this; 
                }

                //get which IRQn_Type we belong to
                II IRQn_Type
irqN            ()
                {
                if( pin_ <= 1 ) return EXTI0_1_IRQn;
                if( pin_ <= 3 ) return EXTI2_3_IRQn;
                return EXTI4_15_IRQn;
                }


// read

                II auto
pinVal          () { return reg_.IDR bitand pinmask_; }
                II auto
latVal          () { return reg_.ODR bitand pinmask_; }

                II auto
isHigh          () { return pinVal(); }
                II auto
isLow           () { return not isHigh(); }
                II auto
isOn            () { return ( invert_ == PINS::LOWISON ) ? isLow() : isHigh(); }
                II auto
isOff           () { return not isOn(); }


// write

                II auto
high            () { reg_.BSRR = pinmask_; return *this; }
                II auto
low             () { reg_.BRR = pinmask_; return *this; }
                II auto
on              () { invert_ == PINS::LOWISON ? low() : high(); return *this; }
                II auto
off             () { invert_ == PINS::LOWISON ? high() : low(); return *this; }
                II auto
on              (bool tf) { tf ? on() : off(); return *this; }
                II auto
toggle          () { latVal() ? low() : high(); return *this; }
                II auto
pulse           () { toggle(); toggle(); return *this; }


                //back to reset state- if reconfiguring pin from an unknown state
                II auto
deinit          ()
                {
                mode(PINS::ANALOG).outType(PINS::PUSHPULL).altFunc(PINS::AF0)
                    .speed(PINS::SPEED0).pull(PINS::NOPULL).low();
                //if we are on the sw port, and is a sw pin
                //then we have a different reset state
                if ( port_ == (PINS::SWCLK/16) and (pin_ == (PINS::SWCLK bitand 15)) ) {
                    pull( PINS::PULLDOWN ).mode( PINS::ALTERNATE );
                    }
                if ( port_ == (PINS::SWCLK/16) and (pin_ == (PINS::SWDIO bitand 15)) ) {
                    pull( PINS::PULLUP ).speed( PINS::SPEED3 ).mode( PINS::ALTERNATE );
                    }
                return *this;
                }

};



