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

/*=============================================================
    GpioPort class
=============================================================*/
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

/*=============================================================
    GpioPin class
=============================================================*/
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

                //properities
                II GpioPin&
mode            (PINS::MODE e)
                {
                reg_.MODER = (reg_.MODER bitand compl (3<<(2*pin_))) bitor (e<<(2*pin_));
                return *this;
                }

                II GpioPin&
outType         (PINS::OTYPE e)
                {
                if( e == PINS::ODRAIN ) reg_.OTYPER or_eq pinmask_;
                else reg_.OTYPER and_eq compl pinmask_;
                return *this;
                }

                II GpioPin&
pull            (PINS::PULL e)
                {
                reg_.PUPDR = (reg_.PUPDR bitand compl (3<<(2*pin_))) bitor (e<<(2*pin_));
                return *this;
                }

                II GpioPin&
speed           (PINS::SPEED e)
                {
                reg_.OSPEEDR = (reg_.OSPEEDR bitand compl (3<<(2*pin_))) bitor (e<<(2*pin_));
                return *this;
                }

                II GpioPin&
altFunc         (PINS::ALTFUNC e)
                {
                auto& ptr = reg_.AFR[pin_>7 ? 1 : 0];
                auto bm = compl (15<<(4*(pin_ bitand 7)));
                auto bv = e<<(4*(pin_ bitand 7));
                ptr = (ptr bitand bm) bitor bv;
                mode( PINS::ALTERNATE );
                return *this;
                }

                II GpioPin&
lock            () { GpioPort::lock(pinmask_); return *this; }

                //back to reset state- if reconfuring pin from an unknown state
                II GpioPin&
deinit          ()
                {
                mode(PINS::ANALOG).outType(PINS::PUSHPULL).altFunc(PINS::AF0)
                    .speed(PINS::SPEED0).pull(PINS::NOPULL);
                low();
                //if we are on the sw port, and is a sw pin
                //then we have a different reset state
                if ( port_ == (PINS::SWCLK/16) and (pin_ == (PINS::SWCLK bitand 15)) ) {
                    mode( PINS::ALTERNATE ).pull( PINS::PULLDOWN );
                    }
                if ( port_ == (PINS::SWCLK/16) and (pin_ == (PINS::SWDIO bitand 15)) ) {
                    mode( PINS::ALTERNATE ).pull( PINS::PULLUP ).speed( PINS::SPEED3 );
                    }
                return *this;
                }

                //read
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

                //write
                II GpioPin&
high            () { reg_.BSRR = pinmask_; return *this; }
                II GpioPin&
low             () { reg_.BRR = pinmask_; return *this; }

                II GpioPin&
on              () { invert_ == PINS::LOWISON ? low() : high(); return *this; }
                II GpioPin&
off             () { invert_ == PINS::LOWISON ? high() : low(); return *this; }
                II GpioPin&
on              (bool tf) { tf ? on() : off(); return *this; }
                II GpioPin&
toggle          () { latVal() ? low() : high(); return *this; }
                II GpioPin&
pulse           () { toggle(); toggle(); return *this; }

};



