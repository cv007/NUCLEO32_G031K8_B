//Pvd.hpp
#pragma once

#include "MyStm32.hpp"

//PWR

struct Pvd {

                enum { EXTIbm = (1<<16), PVDObm = (1<<1) };

                enum
VHIGH           { VH21,VH22,VH25,VH26,VH27,VH29,VH30,VHIN };
                enum
VLOW            { VL20,VL22,VL24,VL25,VL26,VL28,VL29 };
                enum
IRQTYPE         { NONE, IRQ_CLIMBING, IRQ_DROPPING, IRQ_BOTH, EVT_CLIMBING, EVT_DROPPING, EVT_BOTH };

                //read/clear/return state
                auto
isClimbing      ()
                {
                auto bm = EXTI->RPR1 bitand EXTIbm;
                EXTI->RPR1 = bm; //clear if set, harmless if not
                return bm;
                }
                auto
isDropping      ()
                {
                auto bm = EXTI->FPR1 bitand EXTIbm;
                EXTI->FPR1 = bm;
                return bm;
                }

                auto
disable         ()
                {
                PWR->CR2 = 0;
                RCC->APBENR2 or_eq RCC_APBENR2_SYSCFGEN;
                EXTI->IMR1 and_eq compl EXTIbm;
                EXTI->EMR1 and_eq compl EXTIbm;
                EXTI->RTSR1 and_eq compl EXTIbm;
                EXTI->FTSR1 and_eq compl EXTIbm;
                }
                auto
enable          (VHIGH hi, VLOW lo, IRQTYPE t, void(*isr)() = nullptr)
                {
                disable();
                if( int(lo) > int(hi) ) lo = VLOW(hi); //lo <= hi
                PWR->CR2 = (hi<<4) bitor (lo<<1) bitor 1;  //PVDE bit0, =1
                if( t == IRQ_DROPPING or t == IRQ_BOTH or t == EVT_DROPPING or t == EVT_BOTH ) EXTI->RTSR1 or_eq EXTIbm; // = rising irq/evt
                if( t == IRQ_CLIMBING or t == IRQ_BOTH or t == EVT_CLIMBING or t == EVT_BOTH  ) EXTI->FTSR1 or_eq EXTIbm; // = falling irq/evt
                if( t == EVT_DROPPING or t == EVT_BOTH ) EXTI->EMR1 or_eq EXTIbm; // EVT
                if( t == NONE ) return;
                isClimbing(); //clear irq flags
                isDropping();
                if( t == EVT_DROPPING or t == EVT_CLIMBING or t == EVT_BOTH ){
                    EXTI->EMR1 or_eq EXTIbm;
                    return;
                    }
                if( not isr ) return;
                irqFunction( PVD_IRQn, isr );
                EXTI->IMR1 or_eq EXTIbm;
                }
                auto
isVlow          (){ return PWR->SR2 bitand PVDObm; }
                auto
isVhigh         (){ return not isVlow(); }


};
