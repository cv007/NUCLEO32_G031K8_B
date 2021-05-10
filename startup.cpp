//startup file for STM32G031K8
//no other includes needed

/*-----------------------------------------------------------------------------
    types and defines
-----------------------------------------------------------------------------*/
using u32 = unsigned int;
using i32 = int;
using u16 = unsigned short;
using i16 = short;
using flashVectorT = struct { u32* stackTop; void(*vfunc[3])(); };

#define IIA [[ gnu::always_inline ]] inline static auto

/*-----------------------------------------------------------------------------
    vars and constants

    .ramvector section added to linker script, the section starts at the
    first location in ram so is aligned properly, _sramvector/_eramvector
    values added so we can init ram vector table in this startup code

    _sramvector is declared as an array as we need in array form to outsmart
    the compiler which will not let us cast _sramvector in a way so we can 
    set the stack/reset values seperately, so just set to a size of 3 so we
    can access the first two, and use the address of the third (_eramvector
    rakes care of finding the end when we init the vector values)
-----------------------------------------------------------------------------*/
//linker script symbols
extern u32 _etext;              //end of text
extern u32 _sdebugram[32];      //ram set aside for debug use
extern u32 _edebugram;
extern u32 _sramvector[3];      //start of ram vector
extern u32 _eramvector;
extern u32 _srelocate;          //data (initialized)
extern u32 _erelocate;
extern u32 _szero;              //bss (zeroed)
extern u32 _ezero;
extern u32 _estack;

//setup linker symbols as u32 pointers (addresses), and nicer names
static constexpr u32* dataFlashStart    { &_etext };
static constexpr auto debugramStart     { &_sdebugram[0] };
static constexpr auto debugramEnd       { &_edebugram };
static constexpr auto ramvectorStart    { &_sramvector[0] };
static constexpr u32* ramvectorEnd      { &_eramvector };
static constexpr u32* dataStart         { &_srelocate };
static constexpr u32* dataEnd           { &_erelocate };
static constexpr u32* bssStart          { &_szero };
static constexpr u32* bssEnd            { &_ezero };
static constexpr u32* stackTop          { &_estack };

//SCB.VTOR (vector table offset), SCB.AIRCR (for swReset)
static volatile u32&  vtor              { *(reinterpret_cast<u32*>(0xE000ED08)) };
static volatile u32&  aircr             { *(reinterpret_cast<u32*>(0xE000ED0C)) };

//for delay functions
static constexpr u32  FCPU_MHZ          {16}; //16MHz at reset
static constexpr u32  CYCLES_PER_LOOP   {4};

/*-----------------------------------------------------------------------------
    function declarations
-----------------------------------------------------------------------------*/
int main();
static void resetFunc();
extern "C" void __libc_init_array();
static void errorFunc();

/*-----------------------------------------------------------------------------
    reset vector block, 4 words -
        stack value, reset handler/nmi/hardfault addresses
    ram will be used for vector table, so only need these 4 (really only need
        reset handler address, but if hardfault takes place between reset and
        setting vtor we will end up in a known location- errorFunc)
    section is KEEP in linker script, 'used' keeps compiler from complaining
-----------------------------------------------------------------------------*/
[[ using gnu : section(".vectors"), used ]]
flashVectorT flashVector{ stackTop, {resetFunc, errorFunc, errorFunc } };


/*-----------------------------------------------------------------------------
    functions
-----------------------------------------------------------------------------*/
                #pragma GCC push_options
                #pragma GCC optimize ("-Os")

                IIA
delayCycles     (volatile u32 n) { while( n -= CYCLES_PER_LOOP, n>CYCLES_PER_LOOP ){} }

                IIA 
delayMS         (u16 ms){ delayCycles(FCPU_MHZ*1000*ms); }

                #pragma GCC pop_options


                IIA //start address, end address, value
setmem          (u32* s, u32* e, u32 v) { while(s < e) *s++ = v; }

                IIA //start address, end address, values
cpymem          (u32* s, u32* e, u32* v) { while(s < e) *s++ = *v++; }

                //software reset
                [[ using gnu : used, noreturn ]] static void
swReset         () 
                {
                asm( "dsb 0xF":::"memory");
                aircr = 0x05FA0004;
                asm( "dsb 0xF":::"memory");
                while( true ){}
                }

                //unhandled interrupt, exception, or return from main
                [[ using gnu : used, noreturn ]] static void
errorFunc       () 
                { 
                //get main stack pointer, get the 8 stack entries which
                //were possibly set by exception (r0-r3,r12,lr,pc,xpsr)
                //and copy to debug ram
                u32* msp;
                asm( "MRS %0, msp" : "=r" (msp) );
                for( auto i = 0; i < 8; i++ ) {
                    debugramStart[i] = msp[i];
                    }
                //and software reset
                swReset();
                }


                IIA
initRam         ()
                {
                //if last debug ram not set to specific value, clear ram
                if( debugramStart[31] != 0x12345678 ) {
                    setmem( debugramStart, debugramEnd, 0 );
                    debugramStart[31] = 0x12345678;
                    }
                //else inc reset count, and leave values as-is
                //(may be exception data stored)
                else debugramStart[30]++;

                //set all ram vectors to default function (errorFunc)
                setmem( &ramvectorStart[2], ramvectorEnd, (u32)errorFunc );
                //set stack/reset in case someone starts using vtor to read these
                ramvectorStart[0] = (u32)stackTop;
                ramvectorStart[1] = (u32)resetFunc;
                //move vectors to ram
                vtor = (u32)ramvectorStart;
                //init data from flash
                cpymem( dataStart, dataEnd, dataFlashStart );
                //clear bss
                setmem( bssStart, bssEnd, 0 );
                }

                [[ using gnu : used, noreturn ]]
                static void
resetFunc       ()
                {
                delayMS(5000);          //time to allow swd hot-plug
                initRam();              //ram vectors, normal data/bss init               
                __libc_init_array();    //libc init, c++ constructors, etc.

                //C++ will not allow using main with pendatic on, so disable pedantic
                #pragma GCC diagnostic push
                #pragma GCC diagnostic ignored "-Wpedantic"
                main();
                #pragma GCC diagnostic pop

                //should not return from main, so treat as an error
                errorFunc();

                }
