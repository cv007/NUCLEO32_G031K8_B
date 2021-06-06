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
                int
main            ()
                {
                encoder1Init();
                i32 lastc = 0;
                while( true ) {
                    if( lastc != encoder1.count() ) {
                        lastc = encoder1.count();
                        //uart.print("{@Fgreen}encoder1:{@Fwhite} {d}  {b01}{N}", lastc );
                        uart.print( "{@Fgreen}encoder1: ");
                        if( lastc >= 0 ) uart.print( "{@Fwhite}" ); else uart.print( "{@Forange}" );
                        uart.print( "{d}  {b01}{N}", lastc );
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
                uart.print("{N");
                uart.print("   HFSR: 0x{X08}{N}", *(u32*)0xE000ED2C );
                uart.print("   CFSR: 0x{X08}{N}", *(u32*)0xE000ED28 );
                uart.print("   UFSR: 0x{X08}{N}", *(u32*)0xE000ED2A );

                uart.print(" rccCSR: 0x{X08}{N}", RCC->CSR );
                uart.print("scbICSR: 0x{X08}{N}", SCB->ICSR );
                uart.print("   VTOR: 0x{X08}{N}", SCB->VTOR );
                uart.print("{N}");
                for( auto i = 0; i < 32; i++ ) {
                    uart.print("[{u02}]: 0x{X08}{N}", i, debugRam[i] );
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
                    uart.print("  count: {}{N}", count++ );
                    uart.print("  RTSR1: 0x{X08}  {b}{N}", EXTI->RTSR1 );
                    uart.print("  FTSR1: 0x{X08}  {b}{N}", EXTI->FTSR1 );
                    uart.print("   RPR1: 0x{X08}  {b}{N}", EXTI->RPR1 );
                    uart.print("   FPR1: 0x{X08}  {b}{N}", EXTI->FPR1 );
                    uart.print(" EXTICR: 0x{X08}  {b}{N}", EXTI->EXTICR[0] );
                    uart.print("       : 0x{X08}  {b}{N}", EXTI->EXTICR[1] );
                    uart.print("       : 0x{X08}  {b}{N}", EXTI->EXTICR[2] );
                    uart.print("       : 0x{X08}  {b}{N}", EXTI->EXTICR[3] );
                    uart.print("   IMR1: 0x{X08}  {b}{N}", EXTI->IMR1 );
                    uart.print("   EMR1: 0x{X08}  {b}{N}", EXTI->EMR1 );
                    uart.print("   ISER: 0x{X08}  {b}{N}", NVIC->ISER[0U] );
                    uart.print("   ISPR: 0x{X08}  {b}{N}", NVIC->ISPR[0U] );
                    uart.print("scbICSR: 0x{X08}  {b}{N}", SCB->ICSR );
                    uart.print("ITLINE7: 0x{X08}  {b}{N}", SYSCFG->IT_LINE_SR[7] );
                    uart.print("   VTOR: 0x{X08}{N}", SCB->VTOR );
                    uart.print("   [23]: 0x{X08}{N}", ((u32*)(SCB->VTOR))[23] );

                    //startup.cpp will store scbICSR value first thing in reset function
                    //and the value should be 0, but is 3 after programming (exception irq is active)
                    //(startup.cpp also sets exception handler to reset function address so we get
                    //here to print out the data- otherwise stuck in esception handler)

                    //since we are in an exception in these cases, no irq even though all other
                    //registers indicate everything is setup correctly
                    uart.print("   [01]: 0x{X08}{N}", ((u32*)(SCB->VTOR))[01] );
                    uart.print("{N}");
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
                    uart.print( "{u} ", swv & 1 );
                    }
                uart.print( " delay: {u}{N}", t );
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



