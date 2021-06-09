//Printer.hpp
#pragma once

#include <cstdarg>

#include "MyStm32.hpp"


/*-------------------------------------------------------------
    Printer

    simple class to inherit for cout style 'printing'
    via virtual write function which has a signature of-
        bool write(const char)
        write return value is false if there is an error

    not a big fan of cout style as its gets verbose, but
    it does make the creation of the Printer class more
    straight forward

    documentation-

    examples-

--------------------------------------------------------------*/
class Printer {

//-------------|
    private:
//-------------|

                //parent class has the write function
                virtual bool write(const char) = 0;

                static constexpr char hexTable[]{ "0123456789abcdef" };
                SCA OPTIONW_MAX{ 128 }; //maximum value of optionW_

                char NL_[3]     {"\r\n"};   //newline combo, can be change at runtime
                int  count_     { 0 };      //number of chars printed
                bool error_     { false };  //store any errors along the way
                char optionW_   { 0 };      //minimum width
                int  optionB_   { 10 };     //base 2,8,10,16
                bool optionUC_  { false };  //uppercase?
                bool optionSB_  { false };  //showbase? 0x 0 0b
                char optionFIL_ { ' ' };    //setfill char
                bool optionNEG_ { false };  //is negative?
                bool stickyW_   { false };  //keep optionW_ after use?
                bool optionPOS_ { false };  //show + for positive dec
                bool optionBA_  { false };  //bool alpha? "true"/"false", 1,0

                //helper write, so we can also inc count for each char written
                void
write_          (char c)
                { if( write(c) ) count_++; else error_ = true; }

                virtual void
writeStr        (const char* str)
                { while( *str ) write_( *str++ ); }

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
                stickyW_ = false;
                optionBA_ = false;
                return *this;
                }

                // << setw(n) - n limited to OPTIONW_MAX
                Printer&
width           (int v, bool sticky = false)
                {
                optionW_ = v > OPTIONW_MAX ? OPTIONW_MAX : v;
                stickyW_ = sticky;
                return *this;
                }

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

                // << noshowpos , << showpos
                Printer&
positive        (bool tf) { optionPOS_ = tf; return *this; }

                // << noshowalpha , << showalpha
                Printer&
boolalpha       (bool tf) { optionBA_ = tf; return *this; }
                // << endl
                Printer&
writeNL         () { writeStr( NL_ ); return *this; }


                // operator<<


                //string
                Printer&
operator<<      (const char* fmt)
                {
                //add any fill before string
                auto w = optionW_;
                int i = w ? __builtin_strlen( fmt ) : 0;
                while( w-- > i ) write_( optionFIL_ );
                while( *fmt ) write_( *fmt++ );
                if( not stickyW_ ) optionW_ = 0;
                return *this;
//                 //add any fill before string
//                 auto w = optionW_;
//                 auto i = 0;
//                 const char* str = fmt;
//                 if( w ){ //setw in effect, figure out string length
//                     while( *str++ ) i++;
//                     while( w-- > i ) write_( optionFIL_ );
//                     }
//                 while( *fmt ) write_( *fmt++ );
//                 if( not stickyW_ ) optionW_ = 0;
//                 return *this;
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
                    else if( div == 10 and optionPOS_ ) buf[i++] = '+'; //dec is positive and + wanted
                    if( optionSB_ ){ //showbase
                        if( optionB_ == 16 ){ buf[i++] = 'x'; buf[i++] = '0'; }
                        if( optionB_ == 8 ){ buf[i++] = '0'; }
                        if( optionB_ == 2 ){ buf[i++] = 'b'; buf[i++] = '0'; }
                        }
                };
                if( optionFIL_ == '0' ){ fill(); xtras(); } else { xtras(); fill(); }
                while( --i >= 0 ) write_( buf[i] );
                optionNEG_ = false;
                if( not stickyW_ ) optionW_ = 0;
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

                //bool
                Printer&
operator<<      (bool v)
                {
                if( optionBA_ ) return operator<<( v ? "true" : "false" );
                return operator<<( (u32)v );
                }

};


/*-------------------------------------------------------------
    Printer helpers for <<

    setw(n)         set minimum width n (max value is specified in Printer class)
    setW(n)         a 'sticky' version of setw (value remains after use)
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
    noshowpos       no + for dec
    showpos         show + for dec that are positive
    showalpha       bool is "true" or "false"
    noshowalpha     bool is 1 or 0 (treated as unsigned int)
    set(bin|oct|dec|hex, showbase|noshowbase, uppercase|nouppercase, fillchar)

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

                struct SetW_ { int n; };
                inline SetW_
setW            (int n) { return { n }; }
                inline Printer&
                operator<<(Printer& p, SetW_ w)
                {
                return p.width(w.n, true); //sticky width = true
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

                enum POSITIVE_ {
noshowpos,
showpos         };
                inline Printer&
                operator<<(Printer& p, POSITIVE_ v)
                {
                return p.positive( v );
                }

                enum ALPHA_ {
noshowalpha,
showalpha       };
                inline Printer&
                operator<<(Printer& p, ALPHA_ v)
                {
                return p.boolalpha( v );
                }

                struct Set_ { BASE_ base; SHOWBASE_ show; UPPERCASE_ uc; char fill; };
                inline Set_
set             (BASE_ base, SHOWBASE_ sbase = showbase, UPPERCASE_ uc = uppercase, char fill = ' ')
                {
                return { base, sbase, uc, fill };
                }
                inline Printer&
                operator<<(Printer& p, Set_ s)
                {
                return p.base(s.base).showbase(s.show).uppercase(s.uc).setfill(s.fill);
                }

}

/*-------------------------------------------------------------
    Printer helpers for ansi codes

    colors need fg or bg preceeding
    << fg << BLUE << bg << WHITE
--------------------------------------------------------------*/
namespace prn {

        SCA fg        {"\033[38;2;"};
        SCA bg        {"\033[48;2;"};
        SCA BLACK     {"0;0;0m"};
        SCA RED       {"255;0;0m"};
        SCA GREEN     {"0;255;0m"};
        SCA YELLOW    {"255;255;0m"};
        SCA BLUE      {"0;0;255m"};
        SCA MAGENTA   {"255;0;255m"};
        SCA CYAN      {"0;135;215m"};
        SCA WHITE     {"255;255;255m"};
        SCA ORANGE    {"255;99;71m"};
        SCA PURPLE    {"143;0;211m"};
        SCA CLS       {"\033[2J"};
        SCA HOME      {"\033[1;1H"};
        SCA RESET     {"\033[2J\033[1;1H\033[0m"};
        SCA ITALIC    {"\033[3m"};
        SCA NORMAL    {"\033[0m"};
        SCA UNDERLINE {"\033[4m"};
}


//comment to prevent bringing in prn namespace to global namespace
using namespace prn;
