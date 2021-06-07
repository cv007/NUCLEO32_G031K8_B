//Printer.hpp
#pragma once

#include <cstdarg>

#include "MyStm32.hpp"


/*-------------------------------------------------------------
    MarkupHelper

    makes 16bit hash codes from strings
    cannot be placed in Markup class as it is then considered
    an incomplete definition if used in the Markup class, so
    create another class
-------------------------------------------------------------*/
class MarkupHelper {

//-------------|
    protected:
//-------------|

                //simple hash to produce 16bit value from string
                //the 16bits should prevent collisions but should check
                //by running the markupColors through some script/app to verify
                static constexpr u16
MKhash          (const char* str)
                {
                u16 hash = 0;
                while( *str ){ hash = hash * 33 + *str; str++; }
                return hash;
                }

};


/*-------------------------------------------------------------
    Markup

    ansi code markup in print strings

    all inside "{}"
    all start with @ to indicate @nsi decoding

    F is foreground
    B is background

    //cls+home+normal, foreground green
    dev << "{@reset}{@Fgreen}ok" << endl;
    //background white, foreground cyan
    dev << "{@Bwhite}{@Fcyan}ok" << endl;
    //cls
    dev << "{@cls}";
    etc.

    Printer class inherits Markup
-------------------------------------------------------------*/
class Markup : protected MarkupHelper {

//-------------|
    private:
//-------------|

                //Printer
                virtual void writeStr(const char*) = 0;

                using
markupCodeT     = struct {
                const u16 hash;
                const char* str;
                };

                static constexpr markupCodeT
markupCodes     [] {
                // name->hash           string to write
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

//-------------|
    protected:
//-------------|

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

/*-------------------------------------------------------------
    Printer

    simple class to inherit for cout style 'printing'
    via virtual write function which has a signature of-
        bool write(const char)
        write return value is false if there is an error

    not a big fan of cout style, but it does make the creation
    of the Printer class more straight forward as the compiler
    can take on some of the work and eliminate the need for
    more type options



    optional formatting is string contained in {}

    {t0n}
        t = types listed below
        0 = optional '0' (leading 0's on)
        n = optional '1'-'9' (max number of chars to print)
    {N}, print newline, which can set set with .setNewline
    {@ansi}, markup codes for ansi- see Markup class

                                0,n optional  default value for n
    'x' = hex lowercase         {x0n}           8
    'X' = hex uppercase         {X0n}           8
    'b' = binary (0,1)          {b0n}           8  (n 1-4 -> 8,16,24,36)

    all options are reset after each use of <<



    examples-
        //simple string,
        dev << "Hello World" << endl; //-> "Hello World!\r\n"
        dev << "Hello World{N}"; //-> "Hello World!\r\n" (newline code embedded in string)

        int v = -123;

        //print v, no leading 0's, no max limit of chars
        //also reset count/error via .start()
        dev.start() << v; //-> "-123"

        //print v in unsigned, no leading 0's, no max limit of chars
        dev << (u32)v << endl; //-> "4294967173\r\n"

        v = 254;
        //print using d and b
        //v w/leading 0's, max chars 3 (000-999)
        //v binary w/leading 0's, 1 byte max (n 1-4 will convert to 8,16,24,32)
        dev << "d: {03}" << v <<  b: {b01}" << v << endl; //-> "d: 254  b: 11111110\r\n"

        v = 23456;
        //print using X and x
        //X leading 0's, max chars 4 (0000-FFFF)
        //x leading 0's, max chars 2 (00-FF)
        dev << "X: 0x{X04}" << v << "  x: 0x{x02}" << v << endl; //-> "X: 0x5BA0  x: 0xa0\r\n"

        char ch = 'a';
        //if want to a char to be treated as an int, promote it with + or cast it
        dev << "c: " << ch << "  x: 0x{x01}" << +ch << "  d: " << +ch << "  b: 0b{b01}" << +ch << endl;
        //-> "c: a  x: 0x61  d: 97  b: 0b1100001"

--------------------------------------------------------------*/
class Printer : protected Markup {

//-------------|
    private:
//-------------|

                //parent class has the write function
                virtual bool write(const char) = 0;

                static constexpr char hexTable[]{ "0123456789abcdef" };

                char NL_[3]     { "\r\n" }; //change default as needed, can change at runtime
                int  count_     { 0 };      //return number of chars printed
                bool error_     { false };  //store any errors along the way
                char optionT_   { 0 };      //specified type ('x','X','b')
                bool optionLZ_  { false };  //leading 0's (set by '0')
                char optionW_   { 0 };      //number of chars to print ('1'-'9')
                bool markup_    { true };   //ansi markup enable

                //helper write, so we can also inc count
                void
write_          (char c)
                { if( write(c) ) count_++; else error_ = true; }

                virtual void
writeStr        (const char* str)
                { while( *str ) write( *str++ ); }

                void
writeNL         () { writeStr( NL_ ); }


                //parse options inside {}
                void
parseOptions    (const char* fmt){
                //{{ - escape '{' if want to print '{'
                if( *fmt == '{' ){ write_(*fmt++); return; }
                //{} - empty
                if( *fmt == '}' ){ fmt++; return; }
                //{N[N...]} newline as many as there are N's
                if( *fmt == 'N' ){
                    while( *fmt++ == 'N' ) writeNL();
                    return;
                    }
                if( *fmt == '@' ){
                    markup( ++fmt );
                    return;
                    }
                //{x|X|b
                if( *fmt == 'x' or *fmt == 'X' or *fmt == 'b' ) optionT_ = *fmt++;
                //{x|X|b 0 1-9
                if( *fmt == '0' ){ optionLZ_ = true; fmt++; }
                if( *fmt >= '1' and (*fmt <= '9') ) optionW_ = *fmt - '0';
                }

//-------------|
    public:
//-------------|

                // {x|X|b[0][1-9]}
                // {N[N...]} = newline (x number of N's)

                auto
count           (){ return count_; }
                auto
error           (){ return error_; }

                void
resetOptions    ()
                {
                optionT_ = 0;
                optionLZ_ = false;
                optionW_ = 0;
                }

                //reset count/error
                // dev.start << 123;
                // dev.count() returns 3
                Printer&
start           () { count_ = 0; error_ = 0; return *this; }

                //set to 1 or 2 chars you want for {N}ewline
                //NL_[2] already 0, cannot change so no need to set again
                void
setNewline      (const char a, const char b = 0) { NL_[0] = a; NL_[1] = b; }

                void
markupOn        (){ markup_ = true; }
                void //ignore {@ansi} markup
markupOff       (){ markup_ = false; }


                // operator<<


                //string
                Printer&
operator<<      (const char* fmt)
                {
                while( true ) {
                    while( *fmt != '{' and *fmt ) write_( *fmt++ );
                    if( not *fmt ) break;
                    //{
                    parseOptions( ++fmt );
                    //}
                    while( *fmt++ != '}' ){}
                    }
                return *this;
                }

                //unsigned int, also hex and binary
                Printer&
operator<<      (u32 vu)
                {
                auto div = (optionT_ == 0) ? 10 : (optionT_ == 'b' ) ? 2 : 16;
                auto w = optionW_ ? optionW_ : (optionT_ = 0) ? 10 : (optionT_ == 'b') ? 32 : 8;
                if( optionT_ == 'b' and optionW_ >= 1 and optionW_ <= 4 ) w *= 8;
                char ucbm = (optionT_ == 'X') ? compl (1<<5) : 0; //uppercase bitmask
                auto i = 0;
                char buf[w];
                bool not0 = optionLZ_;

                while( i < w ){
                    auto c = hexTable[vu % div];
                    if( c > '9' and ucbm ) c and_eq ucbm; //to uppercase if needed
                    vu /= div;
                    buf[i++] = c;
                    if( not optionLZ_ and vu == 0 ) break;
                    }
                while( --i >= 0 ) {
                    if( buf[i] ) not0 = true;
                    if( i == 0 or not0 ) write_( buf[i] );
                    }
                resetOptions();
                return *this;
                }

                //signed int (take care of - , then pass to unsigned)
                Printer&
operator<<      (i32 v)
                {
                u32 vu = v < 0 ? -v : v;
                if( v < 0 and optionT_ == 0 ) write_( '-' );
                return operator<<( vu );
                }

                //unsigned char
                Printer&
operator<<      (u8 vu) { write_( vu bitand 0xFF ); return *this; }
                //char
                Printer&
operator<<      (i8 v) { write_( v bitand 0xFF ); return *this; }


};
#define endl "{N}"


