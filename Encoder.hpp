#pragma once // Encoder.hpp

#include "MyStm32.hpp"

/*-----------------------------------------------------------------------------
    Encoder

    alternating use of pin irq from encoder outputs, reading pin state in the
    'opposite' interrupt so pin is no longer bouncing at that time

    when both pin states are 0 then is on the second change interrupt where
    a count can be incremented/decremented depending in which interrupt this
    state occurs

    stm32 can only use unique pin numbers for interrupts, so cannot use
    A0 and B0 for example, or if more than one encoder in use all pins
    used have to be unique pin numbers

    (can) create inline Encoder instances at end of file-

        inline Encoder encoder1{PB4, PB5}; //EXTI4_15_IRQn
        inline Encoder encoder2{PB9, PA8}; //EXTI4_15_IRQn
        inline Encoder encoder3{PB0, PB2}; //EXTI0_1_IRQn,EXTI2_3_IRQn

    and add the encoder calls to the EXTI interrupt functions somehere-

    irqFunction(
        EXTI0_1_IRQn,
        [](){ encoder3.isr(); }
        );
    irqFunction(
        EXTI2_3_IRQn,
        [](){ encoder3.isr(); }
        );
    irqFunction(
        EXTI4_15_IRQn,
        [](){ encoder1.isr(); encoder2.isr(); }
        );

    cannot put the .isr() function directly in the vector table as an
    encode instance is required (encode class not static), so will need to
    create a function that calls .isr(), and in the above these functions are
    lambda functions which cal be used to dispatch any calls needed to
    service that interrupt

-----------------------------------------------------------------------------*/
class Encoder {

//-------------|
    public:
//-------------|

Encoder         (PINS::PIN pina, PINS::PIN pinb)
                : pinA_( GpioPin(pina)
                        .mode(PINS::INPUT)
                        .pull(PINS::PULLUP)
                        .irqBothEdges() ),
                  pinB_( GpioPin(pinb)
                        .mode(PINS::INPUT)
                        .pull(PINS::PULLUP)
                        .irqBothEdges() )
                {
                irqSwap();
                }

                //read the current count (count remains unchanged)
                //(absolute position, up to a point)
                //0=no change, -val=CCW, +val=CW
                auto
count           ()
                {
                //count_ read is atomic on 32bit mcu, InterruptLock noe needed
                return count_;
                }

                //read/consume the current count (count zeroed)
                //0=no change, -val=CCW, +val=CW
                auto
read            ()
                {
                InterruptLock lock; //writing/clearing, so don't want to miss a step
                auto ret = count_;
                count_ = 0;
                return ret;
                }

                //reset count to 0
                auto
reset           () { read(); }

                //public access
                auto
isr             ()
                {
                if( isIrqA_ ) { //A irq
                    if( not pinA_.isFlag() ) return; //our pin is not flagged
                    B_ = pinB_.isOn(); //get state of other pin
                    if( not A_ and not B_ and (count_ != COUNTMAX) ) count_++;
                    }
                else { //B irq
                    if( not pinB_.isFlag() ) return;
                    A_ = pinA_.isOn();
                    if( not A_ and not B_ and (count_ != COUNTMIN) ) count_--;
                    }
                irqSwap();
                }

//-------------|
    private:
//-------------|

                //irqMode() will clear pin flag
                auto
irqSwap         () -> void
                {
                if( isIrqA_ ) { //A->B
                    pinA_.irqOff();
                    pinB_.irqOn();
                    isIrqA_ = false;
                    }
                else { //B->A
                    pinB_.irqOff();
                    pinA_.irqOn();
                    isIrqA_ = true;
                    }
                }

                GpioPin pinA_;
                GpioPin pinB_;
                volatile bool A_{1},B_{1};  //pin state measured in opposite isr
                volatile bool isIrqA_;      //1=a, 0=b
                volatile i32 count_;        //encoder count
                //set min/max limit values to match count_ type/size
                SCA COUNTMIN{ (i32)0x80000000 };
                SCA COUNTMAX{ (i32)0x7FFFFFFF };

};


//created as seperate instances
// inline Encoder encoder1{ PB4, PB5 };
// inline Encoder encoder2{ PB9, PA8 };
// inline Encoder encoder3{ PB0, PB2 };

//or array of instances
// inline Encoder encoder[]{
//     { PB4, PB5 },
//     { PB9, PA8 },
//     { PB0, PB2 }
// };

//can create the init here to call the encoder isr from the proper ram vector interrupt
// inline auto encoder1Init(){
//     irqFunction(
//         EXTI4_15_IRQn,
//         [](){ encoder1.isr(); }
//     );
// }
