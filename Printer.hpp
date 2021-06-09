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
    of the Printer class more straight forward

    documentation-

    examples-

--------------------------------------------------------------*/
class Printer : protected Markup {

//-------------|
    private:
//-------------|

                //parent class has the write function
                virtual bool write(const char) = 0;

                static constexpr char hexTable[]{ "0123456789abcdef" };
                static constexpr auto OPTIONW_MAX{ 128 }; //maximum value of optionW_

                char NL_[3]     {"\r\n"};   //newline combo, can be change at runtime
                int  count_     { 0 };      //number of chars printed
                bool error_     { false };  //store any errors along the way
                char optionW_   { 0 };      //minimum width
                int  optionB_   { 10 };     //base 2,8,10,16
                bool optionUC_  { false };  //uppercase?
                bool optionSB_  { false };  //showbase? 0x 0 0b
                char optionFIL_ { ' ' };    //setfill char
                bool markup_    { true };   //markup enabled?
                bool optionNEG_ { false };  //is negative?

                //helper write, so we can also inc count for each char written
                void
write_          (char c)
                { if( write(c) ) count_++; else error_ = true; }

                virtual void
writeStr        (const char* str)
                { while( *str ) write_( *str++ ); }

                //parse options inside {}
                // {N[N...]} is newline combo in use
                // {@ansi} is ansi codes in Markup class
                void
parseOptions    (const char* fmt){
                if( *fmt == '{' ) { write_(*fmt++); return; } //{{ - escaped '{'
                if( *fmt == '}' ) { fmt++; return; } //{} - empty
                if( *fmt == 'N' ) { //{N[N...]} newline as many as there are N's
                    while( *fmt++ == 'N' ) writeNL();
                    return;
                    }
                if( *fmt == '@' ) markup( ++fmt ); //@ansi
                }

//-------------|
    public:
//-------------|

                auto
count           (){ return count_; }
                auto
error           (){ return error_; }


                // everything below returns Printer&


                //set to 1 or 2 chars you want for {N}ewline
                //NL_[2] already 0, cannot change so no need to set
                Printer&
setNewline      (const char a, const char b = 0) { NL_[0] = a; NL_[1] = b; return *this; }
                Printer&
markupOn        (){ markup_ = true; return *this; }
                Printer& //ignore {@ansi} markup
markupOff       (){ markup_ = false; return *this; }


                // << options


                // reset options,  << clear
                Printer&
start           ()
                {
                count_ = 0;
                error_ = 0;
                optionUC_ = false;
                optionB_ = 10;
                optionFIL_ = ' ';
                optionW_ = 0;
                optionSB_ = false;
                return *this;
                }

                // << setw(n) - n limited to OPTIONW_MAX
                Printer&
width           (int v) { optionW_ = v > OPTIONW_MAX ? OPTIONW_MAX : v; return *this; }

                // << bin|oct|dec|hex
                Printer&
base            (int v)
                {
                optionB_ = (v == 16 or v == 8 or v == 2) ? v : 10;
                return *this;
                }

                // << showbase , << noshowbase
                Printer&
showbase        (bool tf) { optionSB_ = tf; return *this; }

                // << setfill('char') (default is ' ')
                Printer&
setfill         (char c) { optionFIL_ = c; return *this; }

                // << nouppercase, << uppercase
                Printer&
uppercase       (bool tf) { optionUC_ = tf; return *this; }

                // << endl
                Printer&
writeNL         () { writeStr( NL_ ); return *this; }


                // operator<<


                //string
                Printer&
operator<<      (const char* fmt)
                {
                //filler before string if width set, but need to know string length
                //first (without markup) as we are not using a buffer
                auto w = optionW_;
                auto i = 0;
                const char* str = fmt;
                if( w ){ //setw in effect
                    while( true ) {
                        while( *str != '{' and *str ){ i++; str++; }
                        while( *str != '}' and *str ){ str++; }
                        if( *str == 0 ) break;
                        }
                    while( w-- > i ) write_( optionFIL_ );
                    }
                while( true ) {
                    while( *fmt != '{' and *fmt ) write_( *fmt++ );
                    if( not *fmt ) break;
                    parseOptions( ++fmt ); // {
                    while( *fmt++ != '}' ){} // }
                    }
                optionW_ = 0;
                return *this;
                }

                //unsigned int
                Printer&
operator<<      (u32 v)
                {
                auto div = optionB_; //2,8,10,16
                auto w = optionW_;
                char ucbm = optionUC_ ? compl (1<<5) : 0; //uppercase bitmask
                auto i = 0;
                char buf[w?w:32];
                while( v ){
                    auto c = hexTable[v % div];
                    if( c > '9' and ucbm ) c and_eq ucbm; //to uppercase
                    v /= div;
                    buf[i++] = c;
                    }
                if( i == 0 ) buf[i++] = '0';
                //add -, 0b, 0x, 0, now if optionFIL_ is not '0'
                //else add fill first
                auto fill = [&](){ while( i < w ) buf[i++] = optionFIL_; };
                auto xtras = [&](){
                    if( optionNEG_ ) buf[i++] = '-'; //negative dec
                    if( optionSB_ ){ //showbase
                        if( optionB_ == 16 ){ buf[i++] = 'x'; buf[i++] = '0'; }
                        if( optionB_ == 8 ){ buf[i++] = '0'; }
                        if( optionB_ == 2 ){ buf[i++] = 'b'; buf[i++] = '0'; }
                        }
                };
                if( optionFIL_ == '0' ){ fill(); xtras(); } else { xtras(); fill(); }
                while( --i >= 0 ) write_( buf[i] );
                optionNEG_ = false;
                optionW_ = 0;
                return *this;
                }

                //signed int
                Printer&
operator<<      (i32 v)
                {
                u32 vu = v < 0 ? -v : v;
                if( v < 0 and optionB_ == 10 ) optionNEG_ = true;
                return operator<<( vu );
                }

                //unsigned char
                Printer&
operator<<      (u8 v)
                {
                write_( v bitand 0xFF );
                return *this;
                }
                //char
                Printer&
operator<<      (i8 v) { write_( v bitand 0xFF ); return *this; }

};


/*-------------------------------------------------------------
    Printer helpers for <<

    setw(n)         set minimum width n (max value is specified in Printer class)
    setfill(c)      set fill char, default is ' '
    endl            write specified newline combo in effect
    bin             set base to 2
    oct             set base to 8
    dec             set base to 10 (default)
    hex             set base to 16
    clear           set options to default, clear count, error
    noshowbase      do not show base headers
    showbase        show base headers- 0x, 0, 0b
    nouppercase     'a'-'f'
    uppercase       'A'-'F'

    all options remain set except for setw(), which is cleared
    after use, also <<clear resets all options
--------------------------------------------------------------*/
namespace prn {

                struct Setw_ { int n; };
                inline Setw_
setw            (int n) { return { n }; }
                inline Printer&
                operator<<(Printer& p, Setw_ w)
                {
                return p.width(w.n);
                }

                struct Setfill_ { char c; };
                inline Setfill_
setfill         (char c) { return { c }; }
                inline Printer&
                operator<<(Printer& p, Setfill_ c)
                {
                return p.setfill( c.c );
                }

                enum ENDL_ {
endl            };
                inline Printer&
                operator<<(Printer& p, ENDL_ v) { (void)v; return p.writeNL(); }

                enum BASE_ {
bin             = 2,
oct             = 8,
dec             = 10,
hex             = 16 };
                inline Printer&
                operator<<(Printer& p, BASE_ v) { return p.base(v); }

                enum CLEAR_ {
clear           };
                inline Printer&
                operator<<(Printer& p, CLEAR_ v) { (void)v; return p.start(); }

                enum SHOWBASE_ {
noshowbase,
showbase        };
                inline Printer&
                operator<<(Printer& p, SHOWBASE_ v)
                {
                return p.showbase( v );
                }

                enum UPPERCASE_ {
nouppercase,
uppercase       };
                inline Printer&
                operator<<(Printer& p, UPPERCASE_ v)
                {
                return p.uppercase( v );
                }

}

using namespace prn;
