//Printer.hpp
#pragma once

#include <cstdio>
#include <cstdarg>

#include "MyStm32.hpp"

/*=============================================================
    Printer
    class to inherit for 'printing' via virtual write function
=============================================================*/
class Printer {

//-------------|
    protected:
//-------------|

                virtual void
write           (const char c) = 0;

//-------------|
    public:
//-------------|

                //blocking, using sprintf to keep simple
                int
print           (const char* fmt, ...)
                {
                va_list args;
                va_start( args, fmt );
                auto ret = vsnprintf( buf_, 128, fmt, args );
                va_end( args );
                for( auto c : buf_ ) {
                    if( c == 0 ) break;
                    write(c);
                    }
                return ret;
                }

//-------------|
    private:
//-------------|

                char buf_[128];

};
