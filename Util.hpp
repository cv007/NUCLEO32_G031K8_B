//Util.hpp
#pragma once 

#include "MyStm32.hpp"

//things that need to be outside of UTIL namespace
extern void* _sstack; //used in random16
using vectorFuncT = void(*)();
//linker symbol, vector table start, LPUART1_IRQn is last irq
extern vectorFuncT _sramvector[16+LPUART1_IRQn+1]; 


/*-----------------------------------------------------------------------------
    UTIL namespace

    notes:

        a home for odds and ends
        using namespace UTIL; will bring them all in to current namespace
        or can access via UTIL::
-----------------------------------------------------------------------------*/
namespace UTIL {

/*-----------------------------------------------------------------------------
    irqFunction() - set interrupt function in ram vector table
    function addresses already have bit0 set
    table offset [16] is for peripheral 0, so using [16+n]
    optionally enable the nvic irq for the function (default), or if want
        to enable on your own, use 'false' for the third argument

    irqDelete() - set to default interrupt handler, disable NVIC irq

    no other options for enable/disable, but can do on your own
    these simply provide a function entry into the table, or remove it
-----------------------------------------------------------------------------*/
                inline auto
irqFunction     (IRQn_Type n, vectorFuncT f, bool enable = true)
                {
                _sramvector[16+n] = f;
                if( enable ) NVIC_EnableIRQ(n); //it will check for >= 0
                }

                inline auto
irqDelete       (IRQn_Type n)
                {
                extern void errorFunc(); //default interrupt handler
                NVIC_DisableIRQ(n);
                _sramvector[16+n] = errorFunc;
                }

/*-----------------------------------------------------------------------------
    use to temporarily disable interrupts for various reasons
    atomic access, etc.
    constructor saves PRIMASK bit 0, then interrupts disabled
    at end of scope deconstructor restores PRIMASK

    void myfunc(int v){
        InterruptLock lock; //instance name unimportant
        shared_var = v;     //interupts off
    }
    deconstructor called at end of scope
    interrupts now restored to previous value
-----------------------------------------------------------------------------*/
class InterruptLock {

//-------------|
    public:
//-------------|

InterruptLock   () 
                : status( __get_PRIMASK() ) 
                { __disable_irq(); }

~InterruptLock  () { __set_PRIMASK(status); }

//-------------|
    private:
//-------------|

                u32 status;

};


/*-----------------------------------------------------------------------------
    get size of an array
    u32 a[16];
    for( auto i = 0; i < arraySize(a); i++ ){}
-----------------------------------------------------------------------------*/
                template<typename T, unsigned N> 
                SCA
arraySize       (T (&v)[N]) { return N; }


/*-----------------------------------------------------------------------------
    get a random 16 bit number, also a version with min/max

    poly values and idea from-
https://www.maximintegrated.com/en/design/technical-documents/app-notes/4/4400.html
-----------------------------------------------------------------------------*/
                inline u16
random16        ()
                {
                auto shift = [] (u32& v, u32 mask) { //local function
                    auto bit0 = v bitand 1;
                    v >>= 1;
                    return bit0 ? v xor_eq mask : v;
                    };
                #define POLY_MASK32 0xB4BCD35C
                #define POLY_MASK31 0x7A5BC2E3
                static u32 lfsr32, lfsr31;
                if( lfsr32 == 0 ) { //init on first use
                    u32* pRam = (u32*)&_sstack;
                    lfsr32 = pRam[0] bitor 1; //cannot be 0, so set a bit
                    lfsr31 = pRam[1] bitor 1; //  to make sure
                    }
                shift(lfsr32, POLY_MASK32); //this one done 2x
                return shift(lfsr32, POLY_MASK32) xor shift(lfsr31, POLY_MASK31);
                #undef POLY_MASK32
                #undef POLY_MASK31
                }

                inline u32
random32        () { return ((u32)random16()<<16) + random16(); }

                inline u64
random64        () { return ((u64)random32()<<32) + random32(); }

                //rand() % (max_number + 1 - minimum_number) + minimum_number

                inline u16
random16        (u16 min, u16 max)
                { return random16() % (max + 1 - min) + min; }

                inline u32
random32        (u32 min, u32 max)
                { return random32() % (max + 1 - min) + min; }

                inline u64
random64        (u32 min, u32 max)
                { return random64() % (max + 1 - min) + min; }

/*-----------------------------------------------------------------------------
    swap two vars of the same type
-----------------------------------------------------------------------------*/
                template <typename T> 
                SCA
swap            (T& a, T& b) { T c(a); a=b; b=c; }


/*-----------------------------------------------------------------------------
    shuffle an array of type T[N], randomly
-----------------------------------------------------------------------------*/
                template<typename T, int N> 
                SCA
shuffle         (T (&arr)[N])
                {
                for( auto i = N-1; i > 0; i-- ) {
                    u8 r = random16(0,i);
                    swap( arr[i], arr[r] );
                    }
                }


/*-----------------------------------------------------------------------------
    simple blocking delays
-----------------------------------------------------------------------------*/
                #pragma GCC push_options
                #pragma GCC optimize ("-Os")

                //simple blocking inline delays
                #define CYCLES_PER_LOOP 4
                II static void 
delayCycles     (volatile i32 n){ while(n -= CYCLES_PER_LOOP, n > 0){} }
                II static void 
delayUS         (u32 us){ delayCycles(System::fcpuMHz*us); }
                II static void 
delayMS         (u16 ms){ delayUS( ms*1000 ); }

                #pragma GCC pop_options
                #undef CYCLES_PER_LOOP

}
