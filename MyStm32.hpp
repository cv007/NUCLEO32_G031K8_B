//MyAvr.hpp
#pragma once
/*-------------------------------------------------------------
    using - stm32g031k8 - nucleo32
--------------------------------------------------------------*/
#include "stm32g031xx.h" //manufacturer header
#include <cstdint>

using u8    = uint8_t;
using i8    = int8_t;
using u16   = uint16_t;
using i16   = int16_t;
using u32   = uint32_t;
using i32   = int32_t;
using u64   = uint64_t;
using i64   = int64_t;

#define SCA     static constexpr auto
#define II      [[ gnu::always_inline ]] inline
#define NOP     asm("nop")

/*--------------------------------------------------------------
    system vars - global access
--------------------------------------------------------------*/
struct System {

                //default out of reset, HSI16
                static inline u8 cpuMHz{ 16 };
};

/*--------------------------------------------------------------
    mcu include
--------------------------------------------------------------*/
#include "stm32g031k8.hpp" //our own mcu specific header

/*--------------------------------------------------------------
    utilities (optionally bring in to global namespace)
--------------------------------------------------------------*/
#include "Util.hpp"
using namespace UTIL;

/*--------------------------------------------------------------
    common includes and inline vars
--------------------------------------------------------------*/
#include "Boards.hpp"
inline Boards::Nucleo32g031 board;      //everyone can access

inline u8 uartBuffer[64];               //create a buffer for uart
inline Uart uart{ board.uart, 1000000, uartBuffer, 64 }; //everyone can access

using namespace PINS;                   //bring into global namespace
using namespace FMT;
