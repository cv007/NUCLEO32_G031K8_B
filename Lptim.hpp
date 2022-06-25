#pragma once //Lptim.hpp

#include "MyStm32.hpp"

//RccLptim class to handle Lptim needs
class RccLptim {
    protected:
    auto LSIon(){ RCC->CSR or_eq RCC_CSR_LSION; }
    auto LSIoff(){ RCC->CSR and_eq RCC_CSR_LSION; }
    auto LSIready(){ return RCC->CSR bitand RCC_CSR_LSIRDY; }

    enum LPTIM_CLKSRC { LPTIM_PCLK, LPTIM_LSI, LPTIM_HSI16, LPTIM_LSE };
    //assuming only 2 LPTIM instances, if more then need to add
    auto lptimeClock(LPTIM_TypeDef* t, LPTIM_CLKSRC e){
        auto bmclr = t == LPTIM1 ? RCC_CCIPR_LPTIM1SEL : RCC_CCIPR_LPTIM2SEL;
        auto bmset = t == LPTIM1 ? e<<RCC_CCIPR_LPTIM1SEL_Pos : e<<RCC_CCIPR_LPTIM2SEL_Pos;
        RCC->CCIPR = (RCC->CCIPR bitand bmclr) bitor bmset;
        }
    auto lptimEnable(LPTIM_TypeDef* t){
        auto bm = t == LPTIM1 ? RCC_APBSMENR1_LPTIM1SMEN : RCC_APBSMENR1_LPTIM2SMEN;
        RCC->APBENR1 or_eq bm;
        }
    auto lptimDisable(LPTIM_TypeDef* t){
        auto bm = t == LPTIM1 ? RCC_APBSMENR1_LPTIM1SMEN : RCC_APBSMENR1_LPTIM2SMEN;
        RCC->APBENR1 and_eq compl bm;
        }
    auto lptimReset(LPTIM_TypeDef* t){
        auto bm = t == LPTIM1 ? RCC_APBRSTR1_LPTIM1RST : RCC_APBRSTR1_LPTIM2RST;
        RCC->APBRSTR1 or_eq bm;
        RCC->APBRSTR1 and_eq compl bm;
        }

};

/*=============================================================
    Lptim - low power timer (LPTIM1, LPTIM2 for stm32g031)
=============================================================*/
struct Lptim : RccLptim {

//-------------|
    private:
//-------------|

                //useful private enum values
                enum { ENABLEbm = 1<<0, CNTSTRTbm = 1<<2 };

                //vars
                LPTIM_TypeDef& lptim_;


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

Lptim           (LPTIM_TypeDef* t) : lptim_(*t) {}

                //functions

                //NOTE: functions that write to registers requiring lptim be on or off to do so,
                //      will enable/disable lptim as needed and leave lptim in that state

                auto
reset           (){ lptimReset(&lptim_); return *this; }
                auto
on              (){ lptim_.CR or_eq ENABLEbm; return *this; }
                auto
off             () { lptim_.CR and_eq compl ENABLEbm; return *this; }
                auto
startContinuous (){ on(); lptim_.CR or_eq CNTSTRTbm; return *this;  }

                //LPTIMIRQ is bitmask value, use as-is
                auto
irqClear        (LPTIMIRQ e) { lptim_.ICR = e; return *this; }
                auto
irqOn           (LPTIMIRQ e){ off(); irqClear(e); lptim_.IER or_eq e; return *this; }
                auto
irqOff          (LPTIMIRQ e) { off(); lptim_.IER and_eq compl e; return *this; }
                auto
irqIsFlag       (LPTIMIRQ e) { return lptim_.ISR bitand e; }
                //note- preload not in use, arr value written 'now'
                auto
setReload       (u16 v) { on(); lptim_.ARR = v; return *this; }
                auto //read twice, valid if both the same value
count           () { u16 v; while( v = lptim_.CNT, v != lptim_.CNT ){} return v; }

                auto
clksel          (CLKSRC e)
                {
                if( e == LSI ) LSIon();                           //enable LSI clock
                if( e != EXTIN ) lptimeClock( &lptim_, (RccLptim::LPTIM_CLKSRC)e );  //clock source
                lptimEnable( &lptim_ );                                    //LPTIMx clock enable
                off();                                                  //needed to write to CFGR
                if( e == EXTIN ) lptim_.CFGR or_eq 1;                   //CKSEL, 0=int clock, 1=ext in
                    else lptim_.CFGR and_eq 1;
                return *this;
                }

};






/*=============================================================
    LptimRepeatFunction - low power timer (LPTIM1, LPTIM2)
    internal LSI only, ~32khz
    simple usage to run a function at intervals < ~2sec
=============================================================*/
                //ms -> lsi counts (max 2048ms)
                SCA
operator "" _ms_lptim(u64 ms) -> u16 { return ms > 2048 ? 65535 : ms*32ul-1; }



struct LptimRepeatFunction : Lptim {

//-------------|
    private:
//-------------|

                //vars
                void(*isrFunc_)();    //store isr function to call
                static inline LptimRepeatFunction* instances_[2]; //for isr use, assuming only lptim1 and lptim2

//-------------|
    public:
//-------------|

                static void //static so can put in vector table
isr             () //assuming only lptim1 and lptim2
                { //get instance needed so we can call into it
                LptimRepeatFunction* lptim = irqActive() == LPTIM1_IRQn ? instances_[0] : instances_[1];
                lptim->irqClear( lptim->ARRM );
                if( lptim->isrFunc_ ) lptim->isrFunc_();  //call function, if set
                }


                auto
reinit          (void(*isrfunc)(), u16 arrVal)
                {
                reset();
                isrFunc_ = isrfunc;
                if( not isrfunc ) return;           //no function, so nothing more to do
                    clksel( LSI )
                    .irqOn( ARRM )
                    .setReload( arrVal )
                    .startContinuous();
                }

                void
reinit          (u16 arrVal)                        //reuse previously set function
                {
                reinit( isrFunc_, arrVal );
                }


LptimRepeatFunction(LPTIM_TypeDef* t, void(*isrfunc)(), u16 arrVal)   //(use _ms_lptim to convert ms to arr value)
                : Lptim(t)
                {
                if( t == LPTIM1 ) instances_[0] = this; else instances_[1] = this;
                irqFunction( t == LPTIM1 ? LPTIM1_IRQn : LPTIM2_IRQn, isr );
                reinit( isrfunc, arrVal );          //set irq function, interval
                }

};





/*=============================================================
    LptimPulseCounter - low power timer (LPTIM1, LPTIM2)
    count pulses on PB5 (lptim1) or PB1 (lptim2)
    LPTIM1_IN1 = PB5, AF5
    LPTIM2_IN1 = PB1, AF5
=============================================================*/
enum LptimPulseCounterInstances { LptimPulseCounterPB5, LptimPulseCounterPB1 };

struct LptimPulseCounter : Lptim {

//-------------|
    private:
//-------------|

                //vars
                volatile u16 pulseCountH_;//upper 16bits (CNT is lower 16bits)
                static inline LptimPulseCounter* instances_[2]; //for isr use, assuming only lptim1 and lptim2

                //functions

                static void //static so can put in vector table
isr             () //assuming only lptim1 and lptim2
                { //get instance needed so we can call into it
                LptimPulseCounter* lptim = irqActive() == LPTIM1_IRQn ? instances_[0] : instances_[1];
                lptim->irqClear( lptim->ARRM );
                lptim->pulseCountH_++; //upper 16 bits
                }

//-------------|
    public:
//-------------|


                auto
reinit          ()
                {
                reset()
                    .clksel( EXTIN )
                    .irqOn( ARRM )
                    .setReload( 65535 )
                    .startContinuous();
                }

                auto
count           ()
                {
                while( true ){
                    auto vL = Lptim::count();
                    auto vH = pulseCountH_;
                    if( vL == Lptim::count() ) return (vH<<16) bitor vL;
                    }
                }


LptimPulseCounter(LptimPulseCounterInstances e)
                : Lptim( e == LptimPulseCounterPB5 ? LPTIM1 : LPTIM2 )
                {
                if( e == LptimPulseCounterPB5 ) instances_[0] = this; else instances_[1] = this;
                PIN pin = (e == LptimPulseCounterPB5) ? PB5 : PB1;
                GpioPin(pin).mode(INPUT).pull(PULLDOWN).altFunc(AF5);
                irqFunction( e == LptimPulseCounterPB5 ? LPTIM1_IRQn : LPTIM2_IRQn, isr );
                reinit();
                }



};

