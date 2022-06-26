#include "MyStm32.hpp"
/*-------------------------------------------------------------
    stm32g031k8 - nucleo32
--------------------------------------------------------------*/
#if 0
                int
main            ()
                {
                u32 count = 0;
                    uart
                        << FG GREEN "Hello World: "
                        << Hex0x << setwf(8,'0') << count++
                        << endl;
                    uart2
                        << FG GREEN "Hello World: "
                        << Hex0x << setwf(8,'0') << count++
                        << endl;
                    }
                    delayMS(10);
                }
#endif

#if 0
#include "Encoder.hpp"
/*-------------------------------------------------------------
    main
        instances available fron headers-
        board
        uart

    incremental encoder hooked up to D11,D12 (PB4,PB5)
    print out values when encoder changes
    (see Encoder.hpp)
--------------------------------------------------------------*/
Encoder encoder1{ board.D[11], board.D[12] };

                static void
encoder1Init    ()
                {
                //lambda function which can call the instance
                //(encoder1 not static, so cannot place encode1.isr address
                // directly in the ram vector)
                irqFunction( EXTI4_15_IRQn, [](){ encoder1.isr(); } );
                }

                int
main            ()
                {
                // uart << endl;

                // uart << Hex0x << setwf(15,' ') << "RCC APBENR1: " << setwf(8,'0') << RCC->APBENR1;
                // RCC->APBENR1 or_eq (1<<28); //PWREN
                // uart << " -> " << setwf(8,'0') << RCC->APBENR1 << endl;

                // uart << Hex0x << setwf(15,' ') << "PWR CR1: " << setwf(8,'0') << PWR->CR1;
                // PWR->CR1 or_eq (1<<8);  //DBP disable rtc domain write protection
                // uart << " -> " << setwf(8,'0') << PWR->CR1 << endl;

                // uart << Hex0x << setwf(15,' ') << "RCC BDCR: " << setwf(8,'0') << RCC->BDCR;
                // RCC->BDCR = 1; //LSEON
                // uart << " -> " << setwf(8,'0') << RCC->BDCR; //LSEON should be set
                // while( (RCC->BDCR bitand 2) == 0 ){} //LSERDY
                // uart << " -> " << setwf(8,'0') << RCC->BDCR << endl2r; //LSERDY is set

/*

  RCC APBENR1: 0x00020000 -> 0x10020000
      PWR CR1: 0x00000208 -> 0x00000308
     RCC BDCR: 0x00000003 -> 0x00000003 -> 0x00000003

*/
// while(1){}


                encoder1Init();

                i32 lastc = 0;
                while( true ) {
                    if( lastc != encoder1.count() ) {
                        lastc = encoder1.count();
                        uart
                            << FG DARK_ORANGE << "encoder1: "
                            << FG << (lastc >= 0 ? BLUE_VIOLET : DEEP_SKY_BLUE)
                            << setw(4) << lastc
                            << " [" << bin << setwf(8,'0') << (lastc bitand 0xFF) << "]" ANSI_NORMAL << endlr;
                        board.led.toggle();
                        delayMS(5);
                        board.led.toggle();
                        }
                    }

                }

#endif


//troubleshoot exception
#if 0
/*-------------------------------------------------------------
    main
        instances available fron headers-
        board
        uart
--------------------------------------------------------------*/
extern u32 _sdebugram;
u32* debugRam{ &_sdebugram };

                int
main            ()
                {
                delayMS( 15000 );
                uart
                    << setfill('0') << showbase << hex << setw(10)
                    << endl
                    << setw(10) <<    "HFSR: " << *(u32*)0xE000ED2C << endl
                    << setw(10) <<    "CFSR: " << *(u32*)0xE000ED28 << endl
                    << setw(10) <<    "UFSR: " << *(u32*)0xE000ED2A << endl
                    << setw(10) <<  "rccCSR: " << RCC->CSR << endl
                    << setw(10) << "scbICSR: " << SCB->ICSR << endl
                    << setw(10) <<    "VTOR: " << SCB->VTOR << endl << clear;
                for( u32 i = 0; i < 32; i++ ) {
                    uart
                        << setw(5) << "[" setw(2) << i << "]: "
                        << setw(10) << debugRam[i] << endl << clear;
                    }
                while( true ){
                    board.led.toggle();
                    delayMS(200);
                    }
                }

#endif


//troubleshoot why no irq
#if 0
/*-------------------------------------------------------------
    main
        instances available from headers-
        board
        uart
--------------------------------------------------------------*/
//D4 (PA10) on Nucleo32
auto sw = GpioPin(board.D[4],LOWISON)
                .mode(INPUT)
                .pull(PULLUP)
                .irqMode(FALLING);

                int
main            ()
                {
                //set irq function for sw irq
                irqFunction(
                    EXTI4_15_IRQn,
                    [](){
                        sw.isFallFlag(); //reading also clears flag
                        //stay here, show we are in isr
                        while(1){
                            board.led.toggle();
                            delayMS(100);
                            }
                    }
                );

                auto count = 0;
                while( true ){
                    delayMS( 5000 );
                    uart
                        << setw(10) << "count: " << count++ << endl
                        << hex << showbase << uppercase << setfil('0')
                        << setw(10) << "RTSR1: " << EXTI->RTSR1 << endl
                        << setw(10) << "FTSR1: " << EXTI->FTSR1 << endl
                        << setw(10) << "RPR1: " << EXTI->RPR1 << endl
                        << setw(10) << "FPR1: " << EXTI->FPR1 << endl
                        << setw(10) << "EXTICR: " << EXTI->EXTICR[0] << endl
                        << setw(10) << ": " << EXTI->EXTICR[1] << endl
                        << setw(10) << ": " << EXTI->EXTICR[2] << endl
                        << setw(10) << ": " << EXTI->EXTICR[3] << endl
                        << setw(10) << "IMR1: " << EXTI->IMR1 << endl
                        << setw(10) << "EMR1: " << EXTI->EMR1 << endl
                        << setw(10) << "ISER: " << NVIC->ISER[0U] << endl
                        << setw(10) << "ISPR: " << NVIC->ISPR[0U] << endl
                        << setw(10) << "scbICSR: " << SCB->ICSR << endl
                        << setw(10) << "ITLINE7: " << SYSCFG->IT_LINE_SR[7] << endl
                        << setw(10) << "VTOR: " << SCB->VTOR << endl
                        << setw(10) << "[23]: " << ((u32*)(SCB->VTOR))[23] << endl

                    //startup.cpp will store scbICSR value first thing in reset function
                    //and the value should be 0, but is 3 after programming (exception irq is active)
                    //(startup.cpp also sets exception handler to reset function address so we get
                    //here to print out the data- otherwise stuck in esception handler)

                    //since we are in an exception in these cases, no irq even though all other
                    //registers indicate everything is setup correctly

                        << "[01]: " << ((u32*)(SCB->VTOR))[01] << endl << clear;
                    }

                }

#endif



#if 0
/*-------------------------------------------------------------
    main
--------------------------------------------------------------*/
//arrays of pins
GpioPin switches[]{ //nucleo board labels D9-D12
    { GpioPin(board.D[12],LOWISON).mode(INPUT).pull(PULLUP) },
    { GpioPin(board.D[11],LOWISON).mode(INPUT).pull(PULLUP) },
    { GpioPin(board.D[10],LOWISON).mode(INPUT).pull(PULLUP) },
    { GpioPin(board.D[9],LOWISON).mode(INPUT).pull(PULLUP) }
};

                bool
switchValues    (u32& swv) //return true if switches changed
                {
                static u32 last;
                swv = 0;
                for( auto& s : switches ) {
                    swv <<= 1;
                    if( s.isOn() ) swv++;
                    }
                if( last != swv ) {
                    last = swv;
                    return true;
                    }
                return false;
                }

                void
printInfo       (u32 swv, u32 t)
                {
                uart
                    << "switches: " << setfill('0') << bin
                    << setw(4) << swv
                    << " delay: " << dec << t << endl;
                }

                int
main            ()
                {
                u32 swState = 0;
                const u32 tbase = 100;

                while( true ) {
                    //get all switch states
                    bool changed = switchValues(swState);
                    //calculate delay value
                    auto t = tbase + swState*200;
                    //print info if switch state changes
                    if( changed ) printInfo( swState, t );
                    //delay and toggle led
                    delayMS( t );
                    board.led.toggle();
                    }

                }

#endif

#if 1
/*-------------------------------------------------------------
    main
--------------------------------------------------------------*/
#include "MyStm32.hpp"
#include "Lptim.hpp"

//count pulses on PB1 ( D[3] )
LptimExtCounter lptimCounter{ LptimExtCounterPB1 };

//need something to generate pulses (board does not provide
//connections to uart2 or led, so will do this instead)
//toggle pin in lptim isr function, connect D[2] (this pin)  to D[3] (lptim counter pb1)
GpioPin pulseGen{ GpioPin(board.D[2]).mode(OUTPUT).off() };


volatile u32 lptimIrqCount; //count lptim irq's, for fun

//blink sos in morse code, 'dit' times are random range of values
LptimRepeatDo lptim {
                    LPTIM1, //which timer (pulse counter is using LPTIM2)
                    []{     //lambda function, could move this to a named function also
                        static constexpr bool sos[]{
                            1,0,1,0,1,0, 0,0,0,
                            1,1,1,0,1,1,1,0,1,1,1,0, 0,0,0,
                            1,0,1,0,1,0,
                            0,0,0,0,0,0,0 };
                        static auto sosIdx = 0u;
                        pulseGen.pulse(); //for lptimCounter clock source
                        board.led.on( sos[sosIdx] );
                        if( ++sosIdx >= arraySize(sos) ){
                            sosIdx = 0;
                            lptim.reinit( random16(80_ms_lptim, 200_ms_lptim) );
                            }
                        lptimIrqCount++;
                        }, //end lambda
                    100_ms_lptim, //start with 100ms
                    };



                int
main            ()
                {
                while( true ) {
                    //             random32 (hex)  irq count (dec) pulse counts
                    //Hello World [00000000][         0][         0]
                    uart    << FG ROYAL_BLUE "Hello World ["  FG LIGHT_GREEN
                            << Hexpad(8) << random32()
                            << FG ROYAL_BLUE "]["
                            << FG ORANGE << setwf(10, ' ') << dec << lptimIrqCount
                            << FG ROYAL_BLUE "]["
                            << FG YELLOW << setw(10) << lptimCounter.count()
                            << FG ROYAL_BLUE "]" << endl;
                    delayMS( 10 );
                    }

                }

#endif
