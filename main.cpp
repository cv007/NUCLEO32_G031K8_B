#include "MyStm32.hpp"
/*-------------------------------------------------------------
    stm32g031k8 - nucleo32
--------------------------------------------------------------*/

#if 1
/*-------------------------------------------------------------
    main
--------------------------------------------------------------*/
//D12 on Nucleo
auto sw = GpioPin(board.D[12],LOWISON).mode(INPUT).pull(PULLUP);

                int
main            ()
                {
                //board.led is config locked, try to change
                board.led.mode(INPUT); 
                //does not change (led keeps working)
                //.lock() must be working

                auto n = 0;
                while( true ){
                    //toggle rate depends on switch
                    delayMS( sw.isOn() ? 100 : 500 );
                    board.led.toggle();
                    uart.print("count: %u\r\n", n++);
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
                uart.print("switches: ");
                for( auto i = 0; i < arraySize(switches); i++, swv >>= 1 ) {
                    uart.print( "%u ", swv & 1 );
                    }
                uart.print( " delay: %u\r\n", t );
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



