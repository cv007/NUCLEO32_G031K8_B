#include "MyStm32.hpp"
/*-------------------------------------------------------------
    stm32g031k8 - nucleo32
--------------------------------------------------------------*/
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


#if 1
/*-------------------------------------------------------------
    main
--------------------------------------------------------------*/
#include "MyStm32.hpp"
#include "Lptim.hpp"

//count pulses on PB1 ( D[3] )
LptimExtCounter lptimCounter{ Lptim2_PB1 };

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
                    uart    << FG ROYAL_BLUE "random32() ["  FG LIGHT_GREEN
                            << Hexpad(8) << random32()
                            << FG ROYAL_BLUE "] lptimIrqCount ["
                            << FG ORANGE << setwf(10, ' ') << dec << lptimIrqCount
                            << FG ROYAL_BLUE "] lptimCounter.count() ["
                            << FG YELLOW << setw(10) << lptimCounter.count()
                            << FG ROYAL_BLUE "]" << endl;
                    delayMS( 10 );
                    }

                }

#endif
