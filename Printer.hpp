//Printer.hpp
#pragma once

#include <cstdio>
#include <cstdarg>

#include "MyStm32.hpp"


/*=============================================================
    Printer
    class to inherit for printf style 'printing'
    via virtual write function which has a signature of-
        bool write(const char)
    write return value is false if there is a problem and
    need to abort
=============================================================*/
class Printer {

//-------------|
    private:
//-------------|

                //set buffer size as needed,
                //will be allocated on the stack
                static constexpr auto bufsiz_{ 128 };

                //returns true if ok, false if a problem
                virtual bool
write           (const char c) = 0;

//-------------|
    public:
//-------------|

                //return value-
                //  < 0 is error
                //  == 0 is nothing to print
                //  >= bufsiz_ is truncated output
                //  < bufsiz_ is complete output

                //blocking on underlying hardware or software buffer,
                int
print           (const char* fmt, ...)
                {
                char buf[bufsiz_]; //using stack, as we are not returning until done
                va_list args;
                va_start( args, fmt );
                auto ret = vsnprintf( buf, bufsiz_, fmt, args );
                va_end( args );
                if( ret <= 0 ) return ret;
                for( auto c : buf ) if( c == 0 or not write( c ) ) break;
                return ret;
                }


};
