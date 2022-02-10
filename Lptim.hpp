#pragma once //Lptim.hpp

#include "MyStm32.hpp"

/*=============================================================
    Lptim - low power timer (LPTIM1, LPTIM2)
=============================================================*/
template<u32 Base_> //Base_
struct Lptim {

                static_assert( Base_ == LPTIM1_BASE or Base_ == LPTIM2_BASE,
                               "invalid Base_ template parameter, use LPTIM1_BASE or LPTIM2_BASE"
                               " for this mcu" );

//-------------|
    private:
//-------------|

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

                //useful private enum values
                enum { ENABLEbm = 1<<0, CNTSTRTbm = 1<<2 };

                //vars
                static inline LPTIM_TypeDef&    reg_{ *(LPTIM_TypeDef*)Base_ }; //lptim registers
                static inline volatile RegRcc&  regRcc_{ *(RegRcc*)RCC_BASE };  //rcc registers

                #define self Lptim<Base_>() //a *this equivalent

//-------------|
    public:
//-------------|

                enum
CLKSRC          {
                PCLK, LSI, HSI16, LSE, EXTIN
                };

                enum //bitmasks
LPTIMIRQ        {
                CMPM = 1<<0, ARRM = 1<<1, EXTTRIG = 1<<2, CMPOK = 1<<3, ARROK = 1<<4,
                UP = 1<<5, DOWN = 1<<6, ALL = 0x7F
                };

                SCA IRQn{ Base_ == LPTIM1_BASE ? LPTIM1_IRQn : LPTIM2_IRQn };   //irq vector number

                //functions

                //NOTE: functions that write to registers requiring lptim be on or off to do so,
                //      will enable/disable lptim as needed and leave lptim in that state

                static auto
clksel          (CLKSRC e)
                {
                if( e == LSI ) regRcc_.LSION = 1;       //enable LSI clock
                if( e != EXTIN ) regRcc_.LPTIMSEL = e;  //clock source
                regRcc_.LPTIMEN = 1;                    //LPTIMx clock enable
                off(); //needed to write to CFGR
                if( e == EXTIN ) reg_.CFGR or_eq 1;     //CKSEL, 0=int clock, 1=ext in
                    else reg_.CFGR and_eq 1;
                return self;
                }


                static auto
reset           (){ regRcc_.LPTIMRST = 1; regRcc_.LPTIMRST = 0; return self; }
                static auto
on              (){ reg_.CR or_eq ENABLEbm; return self; }
                static auto
off             (){ reg_.CR and_eq compl ENABLEbm; return self; }
                static auto
startContinuous (){ on(); reg_.CR or_eq CNTSTRTbm; return self; }

                //LPTIMIRQ is bitmask value, use as-is
                static auto
irqClear        (LPTIMIRQ e) { reg_.ICR = e; return self; }
                static auto
irqOn           (LPTIMIRQ e){ off(); irqClear(e); reg_.IER or_eq e; return self; }
                static auto
irqOff          (LPTIMIRQ e) { off(); reg_.IER and_eq compl e; return self; }
                static auto
irqIsFlag       (LPTIMIRQ e) { return reg_.ISR bitand e; return self; }
                //note- preload not in use, arr value written 'now'
                static auto
setReload       (u16 v) { on(); reg_.ARR = v; return self; }
                static auto //read twice, valid if both the same value
count           () { u16 v; while( v = reg_.CNT, v != reg_.CNT ){} return v; }

                #undef self
};






/*=============================================================
    LptimRepeatFunction - low power timer (LPTIM1, LPTIM2)
    internal LSI only, ~32khz
    simple usage to run a function at intervals < ~2sec
=============================================================*/
                //ms -> lsi counts (max 2048ms)
                SCA
operator "" _ms_lptim(u64 ms) -> u16 { return ms > 2048 ? 65535 : ms*32ul-1; }


template<u32 Base_>
struct LptimRepeatFunction {

//-------------|
    private:
//-------------|

                static inline void(*isrFunc_)();    //store isr function to call
                static inline Lptim<Base_> lptim;   //'base class'

                //functions
                static auto
isr             ()
                {
                lptim.irqClear( lptim.ARRM );
                if( isrFunc_ ) isrFunc_();  //call function, if set
                }

//-------------|
    public:
//-------------|

                static void
reinit          (void(*isrfunc)(), u16 arrVal)
                {
                lptim.reset();
                isrFunc_ = isrfunc;
                if( not isrfunc ) return;           //no function, so nothing more to do
                lptim.clksel( lptim.LSI )
                     .irqOn( lptim.ARRM )
                     .setReload( arrVal )
                     .startContinuous();
                }

                static void
reinit          (u16 arrVal)                        //reuse previously set function
                {
                reinit( isrFunc_, arrVal );
                }

LptimRepeatFunction(void(*isrfunc)(), u16 arrVal)   //(use _ms_lptim to convert ms to arr value)
                {
                irqFunction( lptim.IRQn, isr );     //setup irq vector
                reinit( isrfunc, arrVal );          //set irq function, interval
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

                static_assert( (Base_ == LPTIM1_BASE and Pin_ == PB5) or (Base_ == LPTIM2_BASE and Pin_ == PB1),
                               "invalid pin for LPTIM instance" );

//-------------|
    private:
//-------------|


                //vars
                static inline Lptim<Base_> lptim;       //'base class'
                static inline volatile u16 pulseCountH_;//upper 16bits (CNT is lower 16bits)

                //functions
                static auto
isr             ()
                {
                lptim.irqClear( lptim.ARRM );
                pulseCountH_++; //upper 16 bits
                }

//-------------|
    public:
//-------------|

                static void
reinit          ()
                {
                GpioPin(Pin_).mode(INPUT).pull(PULLDOWN).altFunc(AF5);
                lptim.reset()
                     .clksel( lptim.EXTIN )
                     .irqOn( lptim.ARRM )
                     .setReload( 65535 )
                     .startContinuous();
                }

                static auto
count           ()
                {
                while( true ){
                    auto vL = lptim.count();
                    auto vH = pulseCountH_;
                    if( vL == lptim.count() ) return (vH<<16) bitor vL;
                    }
                }


LptimPulseCounter()
                {
                irqFunction( lptim.IRQn, isr );         //setup irq vector
                reinit();
                }


};
//LPTIM1_IN1 = PB5, AF5
//LPTIM2_IN1 = PB1, AF5
using Lptim1PulseCounter = LptimPulseCounter<LPTIM1_BASE, PB5>;
using Lptim2PulseCounter = LptimPulseCounter<LPTIM2_BASE, PB1>;

