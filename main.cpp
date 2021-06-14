#include "MyStm32.hpp"
/*-------------------------------------------------------------
    stm32g031k8 - nucleo32
--------------------------------------------------------------*/

#if 1
#include "Encoder.hpp"
/*-------------------------------------------------------------
    main
        instances available fron headers-
        board
        uart

    incremental encoder hooked up to D11,D12
    print out values when encoder changes
    (see Encoder.hpp)
--------------------------------------------------------------*/
Encoder encoder1{ board.D[11], board.D[12] }; //PB4,PB5 D11,D12

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
                            << clear
                            << FG HOT_PINK << "encoder1: "
                            << FG << (lastc >= 0 ? BLUE_VIOLET : DEEP_SKY_BLUE)
                            << setw(4) << lastc
                            << setfill('0') << bin << " [" << setw(8) << (lastc bitand 0xFF) << "]" << NORMAL << endl;
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
        instances available fron headers-
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
                // led on when pin grounded- works
                // while(1){ board.led.on( sw.isOn() ); }

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
//crude delay
static void delay(volatile u32 n){ while(n--){} }

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



