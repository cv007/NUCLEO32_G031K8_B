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
    (.ramvector section added to linker script, the section starts at the
    first location in ram so is aligned properly, _sramvector/_eramvector
    values added so we can init ram vector table in this startup code)
-----------------------------------------------------------------------------*/
//linker script symbols
extern u32 _etext;          //end of text
extern u32 _sramvector;     //start of ram vector
extern u32 _eramvector;
extern u32 _srelocate;      //data (initialized)
extern u32 _erelocate;
extern u32 _szero;          //bss (zeroed)
extern u32 _ezero;
extern u32 _estack;

//setup linker symbols as u32 pointers (addresses), and nicer names
static constexpr u32* dataFlashStart    { &_etext };
static constexpr u32* ramvectorStart    { &_sramvector };
static constexpr u32* ramvectorEnd      { &_eramvector };
static constexpr u32* dataStart         { &_srelocate };
static constexpr u32* dataEnd           { &_erelocate };
static constexpr u32* bssStart          { &_szero };
static constexpr u32* bssEnd            { &_ezero };
static constexpr u32* stackTop          { &_estack };

//SCB.VTOR (vector table offset)
static volatile u32&  vtor              { *(reinterpret_cast<u32*>(0xE000ED08)) };

//for delay functions
static constexpr u32  FCPU              {16000000}; //16MHz at reset
static constexpr u32  CYCLES_PER_LOOP   {4};

/*-----------------------------------------------------------------------------
    function declarations
-----------------------------------------------------------------------------*/
int main();
static void resetFunc();
extern "C" void __libc_init_array();


/*-----------------------------------------------------------------------------
    reset vector block, 4 words -
        stack value, reset handler/nmi/hardfault addresses
    ram will be used for vector table, so only need these 4 (really only need
        reset handler address, but if hardfault takes place between reset and
        setting vtor we will end up in a known location- errorFunc)
    section is KEEP in linker script, 'used' keeps compiler from complaining
-----------------------------------------------------------------------------*/
[[ using gnu : section(".vectors"), used ]]
flashVectorT flashVector{ stackTop, {resetFunc, resetFunc, resetFunc} };


/*-----------------------------------------------------------------------------
    functions
-----------------------------------------------------------------------------*/
                #pragma GCC push_options
                #pragma GCC optimize ("-Os")

                IIA
delayCycles     (volatile i32 n) { while(n -= CYCLES_PER_LOOP, n>0){} }

                IIA 
delayMS         (u16 ms){ delayCycles(FCPU/1000*ms-1); }

                #pragma GCC pop_options

                IIA //start address, end address, value
setmem          (u32* s, u32* e, u32 v) { while(s < e) *s++ = v; }

                IIA //start address, end address, values
cpymem          (u32* s, u32* e, u32* v) { while(s < e) *s++ = *v++; }

                //unhandled interrupt, or return from main
                //do something here to debug, or to recover when no longer
                //debugging (system reset, or something)
                [[ using gnu : used, noreturn ]]
                static void
errorFunc       () { while(true); }

                IIA
initVectors     ()
                {
                //set all ram vectors to default function (errorFunc)
                //(stack and reset also set, should be harmless)
                setmem( ramvectorStart, ramvectorEnd, (u32)errorFunc );
                //move vectors to ram
                vtor = (u32)ramvectorStart;
                }

                IIA
initRam         ()
                {
                //init data from flash
                cpymem( dataStart, dataEnd, dataFlashStart );
                //clear bss
                setmem( bssStart, bssEnd, 0 );
                }

                [[ using gnu : used, noreturn ]]
                static void
resetFunc       ()
                {
                delayMS(5000);  //time to allow swd hot-plug
                initVectors();  //setup ram vectors
                initRam();      //normal data/bss init

                //libc init, c++ constructors, etc.
                __libc_init_array();

                //C++ will not allow using main with pendatic on, so disable pedantic
                #pragma GCC diagnostic push
                #pragma GCC diagnostic ignored "-Wpedantic"
                main();
                #pragma GCC diagnostic pop

                //should not return from main, so treat as an error
                errorFunc();
                }
