#pragma once //Lptim.hpp

#include "MyStm32.hpp"

                //ms -> lsi counts (max 2048ms)
                SCA
operator "" _ms_lptim(u64 ms) -> u16 { return ms > 2048 ? 65535 : ms*32ul-1; }

/*=============================================================
    LptimRepeatFunction - low power timer (LPTIM1, LPTIM2)
    internal LSI only, ~32khz
    simple usage to run a function at intervals < ~2sec
=============================================================*/
template<u32 Base_> //Base_
struct LptimRepeatFunction {

                static_assert( Base_ == LPTIM1_BASE or Base_ == LPTIM2_BASE,
                               "invalid Base_ template parameter, use LPTIM1_BASE or LPTIM2_BASE"
                               " for this mcu" );

//-------------|
    private:
//-------------|

                enum
CLKSRC          {
                PCLK, LSI, HSI16, //LSE
                };

                enum //bitmasks
LPTIMIRQ        {
                CMPM = 1<<0, ARRM = 1<<1, EXTTRIG = 1<<2, CMPOK = 1<<3, ARROK = 1<<4, UP = 1<<5, DOWN = 1<<6, ALL = 0x7F
                };

                //LPTIMx specific rcc register layout, for lptim use only
                SCA N{ Base_ == LPTIM2_BASE };                      //0=lptim1, 1=lptim2
                struct RegRcc {
                    u32 unused1[11];                                //0x00-0x2B
                    u32 :31-N; u32 LPTIMRST :1; u32 :N;             //APBRSTR1, 0x2C-0x2F
                    u32 unused2[3];                                 //0x30-0x3B
                    u32 :31-N; u32 LPTIMEN :1; u32 :N;              //APBENR1, 0x3C-0x3F
                    u32 unused3[5];                                 //0x40-0x53
                    u32 :18+N*2; u32 LPTIMSEL :2; u32 :12-N*2;      //CCIPR, 0x54-0x57
                    u32 unused4[2];                                 //0x58-0x5F
                    u32 LSION :1; u32 :31;                          //CSR, 0x60
                };

                //useful enums/constants
                enum { ENABLEbm = 1<<0, CNTSTRTbm = 1<<2 };
                SCA IRQn{ Base_ == LPTIM1_BASE ? LPTIM1_IRQn : LPTIM2_IRQn };   //irq vector number

                //vars
                static inline LPTIM_TypeDef&    reg_{ *(LPTIM_TypeDef*)Base_ }; //lptim registers
                static inline volatile RegRcc&  regRcc_{ *(RegRcc*)RCC_BASE };  //rcc registers

                static inline void(*isrFunc_)();                                //store isr function to call


                //functions

                //NOTE: functions that write to registers requiring lptim be on or off to do so,
                //      will enable/disable lptim as needed and leave lptim in that state

                static auto
isr             ()
                {
                irqClear( ARRM );
                if( isrFunc_ ) isrFunc_();  //call function, if set
                }

                static auto
clksel          (CLKSRC e)
                {
                if( e == LSI ) regRcc_.LSION = 1; //enable LSI clock
                regRcc_.LPTIMSEL = e;       //clock source
                regRcc_.LPTIMEN = 1;        //LPTIMx clock enable
                }

                static auto
reset           (){ regRcc_.LPTIMRST = 1; regRcc_.LPTIMRST = 0; }
                static auto
on              (){ reg_.CR or_eq ENABLEbm; }
                static auto
off             (){ reg_.CR and_eq compl ENABLEbm; }
                static auto
startContinuous (){ on(); reg_.CR or_eq CNTSTRTbm; }
                static auto

                //LPTIMIRQ is bitmask value, use as-is
irqClear        (LPTIMIRQ e) { reg_.ICR = e; }
                static auto
irqOn           (LPTIMIRQ e){ off(); irqClear(e); reg_.IER or_eq e; }
                static auto
irqOff          (LPTIMIRQ e) { off(); reg_.IER and_eq compl e; }
                static auto
irqIsFlag       (LPTIMIRQ e) { return reg_.ISR bitand e; }
                //note- preload not in use, arr value written 'now'
                static void
setInterval     (u16 v) { on(); reg_.ARR = v; }

//-------------|
    public:
//-------------|

                static void
reinit          (void(*isrfunc)(), u16 arrVal)
                {
                reset();                        //reset lptim via rcc
                //lptim registers now in reset state
                isrFunc_ = isrfunc;
                if( not isrfunc ) return;       //no function, so nothing more to do
                irqOn( ARRM );                  //lptim will be off after set
                setInterval( arrVal );          //lptim is now on
                startContinuous();              //start counter continuous mode
                }

                static void
reinit          (u16 arrVal)                    //reuse previously set function
                {
                reinit( isrFunc_, arrVal );
                }

LptimRepeatFunction(void(*isrfunc)(), u16 arrVal) //(use _ms_lptim to convert ms to arr value)
                {
                clksel( LSI );                  //enable clock src in rcc
                irqFunction( IRQn, isr );       //setup irq vector
                reinit( isrfunc, arrVal );      //set irq function, interval
                }


};

using Lptim1RepeatFunction = LptimRepeatFunction<LPTIM1_BASE>;
using Lptim2RepeatFunction = LptimRepeatFunction<LPTIM2_BASE>;







/*=============================================================
    LptimPulseCounter - low power timer (LPTIM1, LPTIM2)
    count pulses on PB5 (lptim1) or PB1 (lptim2)
=============================================================*/
template<u32 Base_, PIN Pin_> //Base_
struct LptimPulseCounter {

                static_assert( Base_ == LPTIM1_BASE or Base_ == LPTIM2_BASE,
                               "invalid Base_ template parameter, use LPTIM1_BASE or LPTIM2_BASE"
                               " for this mcu" );

                static_assert( (Base_ == LPTIM1_BASE and Pin_ == PB5) or (Base_ == LPTIM2_BASE and Pin_ == PB1),
                               "invalid pin for LPTIM instance" );

//-------------|
    private:
//-------------|

                enum //bitmasks
LPTIMIRQ        {
                CMPM = 1<<0, ARRM = 1<<1, EXTTRIG = 1<<2, CMPOK = 1<<3, ARROK = 1<<4, UP = 1<<5, DOWN = 1<<6, ALL = 0x7F
                };

                //LPTIMx specific rcc register layout, for lptim use only
                SCA N{ Base_ == LPTIM2_BASE };                      //0=lptim1, 1=lptim2
                struct RegRcc {
                    u32 unused1[11];                                //0x00-0x2B
                    u32 :31-N; u32 LPTIMRST :1; u32 :N;             //APBRSTR1, 0x2C-0x2F
                    u32 unused2[3];                                 //0x30-0x3B
                    u32 :31-N; u32 LPTIMEN :1; u32 :N;              //APBENR1, 0x3C-0x3F
                    u32 unused3[5];                                 //0x40-0x53
                    u32 :18+N*2; u32 LPTIMSEL :2; u32 :12-N*2;      //CCIPR, 0x54-0x57
                    u32 unused4[2];                                 //0x58-0x5F
                    u32 LSION :1; u32 :31;                          //CSR, 0x60
                };

                //useful enums/constants
                enum { ENABLEbm = 1<<0, CNTSTRTbm = 1<<2 };
                SCA IRQn{ Base_ == LPTIM1_BASE ? LPTIM1_IRQn : LPTIM2_IRQn };   //irq vector number

                //vars
                static inline LPTIM_TypeDef&    reg_{ *(LPTIM_TypeDef*)Base_ }; //lptim registers
                static inline volatile RegRcc&  regRcc_{ *(RegRcc*)RCC_BASE };  //rcc registers
                static inline volatile u16      pulseCountH_;                   //upper 16bits (CNT lower 16bits)

                //functions

                //NOTE: functions that write to registers requiring lptim be on or off to do so,
                //      will enable/disable lptim as needed and leave lptim in that state

                static auto
isr             ()
                {
                irqClear( ARRM );
                pulseCountH_++; //upper 16 bits
                }

                static auto
reset           (){ regRcc_.LPTIMRST = 1; regRcc_.LPTIMRST = 0; }
                static auto
on              (){ reg_.CR or_eq ENABLEbm; }
                static auto
off             (){ reg_.CR and_eq compl ENABLEbm; }
                static auto
startContinuous (){ on(); reg_.CR or_eq CNTSTRTbm; }

                //LPTIMIRQ is bitmask value, use as-is
                static auto
irqClear        (LPTIMIRQ e) { reg_.ICR = e; }
                static auto
irqOn           (LPTIMIRQ e){ off(); irqClear(e); reg_.IER or_eq e; }

                //note- preload not in use, arr value written 'now'
                static void
setInterval     (u16 v) { on(); reg_.ARR = v; }

//-------------|
    public:
//-------------|

                static void
reinit          ()
                {
                reset();                        //reset lptim via rcc
                //lptim is off
                reg_.CFGR = 1;                  //CKSEL = LPTIM ext IN1
                irqOn( ARRM );                  //lptim will be off after set
                setInterval( 65535 );           //lptim is now on
                startContinuous();
                }

                static auto
count           () //we have to read CNT twice (which we also already need to do)
                {
                while( true ){
                    auto vL = reg_.CNT;
                    auto vH = pulseCountH_;
                    if( vL == reg_.CNT ) return (vH<<16) bitor vL;
                    }
                }


LptimPulseCounter()
                {
                regRcc_.LPTIMEN = 1;            //LPTIMx clock enable
                irqFunction( IRQn, isr );       //setup irq vector
                reinit();
                GpioPin(Pin_).mode(INPUT).pull(PULLDOWN).altFunc(AF5);
                }


};
//LPTIM1_IN1 = PB5, AF5
//LPTIM2_IN1 = PB1, AF5
using Lptim1PulseCounter = LptimPulseCounter<LPTIM1_BASE, PB5>;
using Lptim2PulseCounter = LptimPulseCounter<LPTIM2_BASE, PB1>;

