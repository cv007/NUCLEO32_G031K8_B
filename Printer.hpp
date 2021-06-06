//Printer.hpp
#pragma once

#include <cstdarg>

#include "MyStm32.hpp"


/*=============================================================
    MarkupHelper

    makes 16bit hash codes from strings
    needs to be defined before use so cannot place this in
    Markup class, so is another class
=============================================================*/
class MarkupHelper {

public:
                //simple hash to produce 16bit value from string
                //the 16bits should prevent collisions but should check
                //by running the markupColors through some script/app to verify
                static constexpr u16
MKhash          (const char* str)
                {
                u16 hash = 0;
                while ( *str ){ hash = hash * 33 + *str; str++; }
                return hash;
                }

};


/*=============================================================
    Markup

    ansi codes created in print strings

    all inside {}
    all start with @
    F is foreground color
    B is background color

    dev.print( "{@Fgreen}ok{N}" ); //foreground green
    dev.print( "{@Bwhite}ok{N}" ); //background white
    dev.print( "{@cls}" ); //cls
    etc.

    Printer class inherits Markup
=============================================================*/
class Markup : MarkupHelper {

                //Printer
                virtual void writeStr(const char*) = 0;

                using
markupCodeT     = struct {
                const u16 hash;
                const char* str;
                };

                static constexpr markupCodeT
markupCodes     [] {
                { MKhash("black"),      "0;0;0m" },
                { MKhash("red"),        "255;0;0m" },
                { MKhash("green"),      "0;255;0m" },
                { MKhash("yellow"),     "255;255;0m" },
                { MKhash("blue"),       "0;0;255m" },
                { MKhash("magenta"),    "255;0;255m" },
                { MKhash("cyan"),       "0;135;215m" },
                { MKhash("white"),      "255;255;255m" },
                { MKhash("orange"),     "255;99;71m" },
                { MKhash("purple"),     "143;0;211m" },
                //control
                { MKhash("cls"),        "2J" },
                { MKhash("home"),       "1;1H" },
                { MKhash("reset"),      "2J\033[1;1H\033[0m" }, //cls+home+normal
                //attributes
                { MKhash("italic"),     "3m" },
                { MKhash("normal"),     "0m" },
                { MKhash("underline"),  "4m" },
                };

protected:

                auto
markup          (const char* fmt)
                {
                bool fg = *fmt == 'F';
                bool bg = *fmt == 'B';
                if( fg or bg ) fmt++;
                u16 hash = 0;
                while ( *fmt and *fmt != '}' ) hash = hash * 33 + *fmt++;
                for( auto& c : markupCodes ){
                    if( c.hash != hash ) continue;
                    writeStr( "\033[" );
                    if( fg ) writeStr( "38;2;" );
                    if( bg ) writeStr( "48;2;" );
                    writeStr( c.str );
                    break;
                    }
                }

};


/*=============================================================
    Printer

    class to inherit for printf style 'printing'
    via virtual write function which has a signature of-
        bool write(const char)
    write return value is false if there is an error

    a vfprintf replacement is used, using a different style

    .print return value is count of chars printed, and if
    any errors the count is negative


    format is string contained in {}
    there will either be pairs of-
        fmt, data (const char*, int)
        or
        fmt, str (const char*, const char*)
    or
    fmt (const char*)


    {t0n}
        t = types listed below
        0 = optional '0' (leading 0's on)
        n = optional '1'-'9' (max number of chars to print)
    {N}, print newline, which can set set with .setNewline
    {@ansi}, markup codes for ansi- see Markup class

                                0,n optional  default value for n
    'd' = signed integer        {d0n}           10
    'u' = unsigned integer      {u0n}           10
    'x' = hex lowercase         {x0n}           8
    'X' = hex uppercase         {X0n}           8
    'b' = binary (0,1)          {b0n}           32  (n 1-4 -> 8,16,24,36)
    'c' = character (byte)      {c}  (no other options)

    examples-
        int v = -123;

        //print v using default 'd', {} defaults to 'd'
        //no leading 0's, no max limit of chars
        dev.print( "{}", v ); //-> "-123"

        //print v using 'u'
        //no leading 0's, no max limit of chars
        dev.print( "{u}{N}", v ); //-> "4294967173\r\n"

        v = 254;
        //print using d and b
        //d leading 0's, max chars 3 (000-999)
        //b leading 0's, 1 byte max (n 1-4 will convert to 8,16,24,32)
        dev.print( "d: {d03}  b: {b01}{N}", v ); //-> "d: 254  b: 11111110\r\n"

        v = 23456;
        //print using X and x
        //X leading 0's, max chars 4 (0000-FFFF)
        //x leading 0's, max chars 2 (00-FF)
        dev.print( "X: 0x{X04}  x: 0x{x02}{N}", v ); //-> "X: 0x5BA0  x: 0xa0\r\n"

        char ch = 'a';
        dev.print( "c: {c}  x: 0x{x}  d: {d}  b: 0b{b}{N}", ch );
        //-> "c: a  x: 0x61  d: 97  b: 0b1100001"

        //no need to specify type for a string,
        //just need a {} placeholder for the string
        dev.print( "Hello {}", "World", "{}{N}", "!" ); //-> "Hello World!\r\n"

    {}
=============================================================*/
class Printer : Markup {

                //parent class has the write function
                virtual bool write(const char) = 0;

                static constexpr char hexTable[]{ "0123456789abcdef" };

                char NL_[3]     { "\r\n" }; //newline string, can change via setNewline
                int  count_     { 0 };      //return number of chars printed
                bool error_     { false };  //store any errors along the way
                char optionT_   { 'd' };    //specified type ('d','x','X','u','b','c')
                bool optionLZ_  { false };  //leading 0's (set by '0')
                char optionW_   { 8 };      //max number of chars to print ('1'-'9')
                bool markup_    { true };   //ansi markup enabled

                //helper write, so we can also inc count_
                void
write_          (char c)
                { if( write(c) ) count_++; else error_ = true; }

                void
writeStr        (const char* str)
                { while( *str ) write( *str++ ); }

                void
writeNL         () { writeStr( NL_ ); }

                //formatting function
                void
writeV          (int v)
                {
                auto w = 32;                //max width (assuming 'b')
                bool uc = false;            //uppercase? (for 'x', 'X')
                unsigned vu = (unsigned)v;  //use unsigned value
                switch( optionT_ ) {
                    case 'c':
                        write_( vu bitand 0xFF );
                        break;
                    case 'b': {
                        //1-4 -> 8,16,24,32
                        optionW_ = optionW_ <= 4 ? optionW_*8 : 32;
                        bool not0 = optionLZ_;
                        while( w-- ){
                            auto c = vu bitand 0x80000000 ? '1' : '0';
                            if( c == '1' ) not0 = true;
                            if( (w < optionW_) and not0 ) write_( c );
                            vu <<= 1;
                            }
                        break;
                        }
                    case 'd':
                        if( v < 0 ){ vu = (unsigned)-v; write_('-'); }
                        [[ fallthrough ]];
                    case 'u': {
                        w = 10;
                        auto dv = 1000000000ul;
                        bool not0 = optionLZ_;
                        while( w-- ){
                            auto c = vu/dv;
                            if( c != 0 or w == 0 ) not0 = true;
                            vu -= c*dv;
                            if( (w < optionW_) and not0 ) write_( c + '0' );
                            dv /= 10;
                            }
                        break;
                        }
                    case 'X':
                        uc = true;
                        [[ fallthrough ]];
                    case 'x': {
                        w = 8;
                        bool not0 = optionLZ_;
                        while( w-- ){
                            auto c = hexTable[(vu >> 28) bitand 0xF];
                            if( c != '0' or w == 0 ) not0 = true;
                            if( c >= 'a' and uc ) c and_eq compl (1<<5);
                            if( (w < optionW_) and not0 ) write_( c );
                            vu <<= 4;
                            }
                        break;
                        }

                    }
                }

                //parse options inside {}
                bool //found option(s), need to print the value
parseOptions    (const char* fmt){
                //{{
                if( *fmt == '{' ){ write_(*fmt++); return false; }
                //{}
                if( *fmt == '}' ){ fmt++; return false; }
                //{N..}
                if( *fmt == 'N' ){
                    while( *fmt++ == 'N' ) writeNL();
                    return false;
                    }
                //{@markup}
                if( *fmt == '@' ){
                    if( markup_ ) markup( ++fmt );
                    return false;
                    }
                //{d|x|X|u|b|c
                if( *fmt < '0' or (*fmt > '9') ) optionT_ = *fmt++;
                //set default max widths if not later specified
                //(b is taken care of in writeV)
                optionW_ = (optionT_ == 'd' or optionT_ == 'u' ? 10 : 8);
                //{d|x|X|u|b|c 0 1-9
                if( *fmt == '0' ){ optionLZ_ = true; fmt++; }
                if( *fmt >= '1' and (*fmt <= '9') ) optionW_ = *fmt - '0';
                return true;
                }

                //final call to print with no more fmt/data
                //return the count, which is the number of chars output
                //if any errors, return the negative count of chars output
                int
print           ()
                {
                auto ret = error_ ? -count_ : count_;
                count_ = 0; //0 for next use
                error_ = false;
                return ret;
                }

public:
                // {[d|x|X|u|b|c][0][1-9]} - default is 'd'
                // {N[N...]} = newline (x number of N's)
                // {@ansi}

                template<typename...Ts>
                int
print           (const char* fmt, int v, Ts...ts )
                {
                bool written = false;
                optionT_ = 'd'; //each print fmt/data assumes 'd'
                optionLZ_ = false; //and no leading 0's
                while( true ) {
                    while( *fmt != '{' and *fmt ) write_( *fmt++ );
                    if( not *fmt ) break;
                    //{
                    if( parseOptions(++fmt) ) {
                        writeV( v );
                        written = true;
                        }
                    //}
                    while( *fmt++ != '}' ){}
                    }
                if( not written ) writeV( v );
                return print( ts... );
                }

                //string w/no data ( can still use {N}, {@ansi} )
                int
print           (const char* fmt)
                {
                while( true ) {
                    while( *fmt != '{' and *fmt ) write_( *fmt++ );
                    if( not *fmt ) break;
                    //{
                    parseOptions( ++fmt );
                    //}
                    while( *fmt++ != '}' ){}
                    }
                return print();
                }

                //string, string
                template<typename...Ts>
                int
print           (const char* fmt, const char* str, Ts...ts)
                {
                bool written = false;
                while( true ) {
                    while( *fmt != '{' and *fmt ) write_( *fmt++ );
                    if( not *fmt ) break;
                    //{
                    if( parseOptions(++fmt) ) {
                        writeStr( str );
                        written = true;
                        }
                    //}
                    while( *fmt++ != '}' ){}
                    }
                if( not written ) writeStr( str );
                return print( ts... );
                }

                //set to 1 or 2 chars you want for {N}ewline
                void
setNewline      (const char a, const char b = 0)
                {
                NL_[0] = a;
                NL_[1] = b;
                //NL_[2] already 0, cannot change so need to set again
                }

                void
markupOn        (){ markup_ = true; }

                //ignore {@ansi} markup
                void
markupOff       (){ markup_ = false; }

};





#if 0 //original snprintf based version, was about 1.5K > above version without markup feature
#include <cstdio>
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
#endif


