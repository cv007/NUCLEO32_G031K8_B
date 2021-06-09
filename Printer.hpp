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

    no buffers used, all data goes directly out to the
    device write function, if any buffering wanted it
    has to be done in the device class that inherits this


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
                bool optionJL_  { false };  //justify left?

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
                optionJL_ = false;
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

                // << bin|bin0b|oct|oct0|dec|hex|hex0x|Hex|Hex0x
                //     2    3    8   9   10  16   17   18   19
                //          SB       SB           SB   UC  SB+UC
                Printer&
base            (int v)
                {
                optionUC_ = (v >= 18); //18,19
                optionSB_ = (v bitand 1);//3,9,17,19
                if( v >= 16 ) optionB_ = 16; //16,17,18,19-> base 16
                else optionB_ = v bitand 10; //-> base 2,8,10
                return *this;
                }

                // << setfill('char') (default is ' ')
                Printer&
setfill         (char c) { optionFIL_ = c; return *this; }

                // << noshowpos , << showpos
                Printer&
positive        (bool tf) { optionPOS_ = tf; return *this; }

                // << noshowalpha , << showalpha
                Printer&
boolalpha       (bool tf) { optionBA_ = tf; return *this; }

                // << left, << right
                Printer&
justifyleft     (bool tf) { optionJL_ = tf; return *this; }

                // << endl
                Printer&
writeNL         () { writeStr( NL_ ); return *this; }


                // operator<<


                //string
                Printer&
operator<<      (const char* fmt)
                {
                auto w = optionW_;
                int i = w ? __builtin_strlen( fmt ) : 0;
                auto fill = [&]{ while( w-- > i ) write_( optionFIL_ ); };
                if( not optionJL_ ) fill(); //justify right, fill first
                while( *fmt ) write_( *fmt++ );
                if( optionJL_ ) fill();//justify left, fill last
                if( not stickyW_ ) optionW_ = 0;
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
                    (a setw() will clear the sticky)
    setfill(c)      set fill char, default is ' '
    endl            write newline combo as specified in Printer class
    bin             set base to 2
    bin0b           with showbase
    oct             set base to 8
    oct0            with showbase
    dec             set base to 10 (default)
    hex             set base to 16
    hex0x           with showbase
    Hex             uppercase
    Hex0x           uppercase with showbase
    clear           set options to default, clear count, error
    noshowpos       no + for dec
    showpos         show + for dec that are positive
    showalpha       bool is "true" or "false"
    noshowalpha     bool is 1 or 0 (treated as unsigned int)

    setBWF(bin|bin0b|oct|oct0||dec|hex|hex0x,Hex,Hex0x, W, fillchar)
        W is setW sticky version

    setBwF(bin|bin0b|oct|oct0||dec|hex|hex0x,Hex,Hex0x, w, fillchar)
        w is setw - non sticky version

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
bin0b           = 3,    //0b
oct             = 8,
oct0            = 9,    //0
dec             = 10,
hex             = 16,   //lowercase
hex0x           = 17,   //0x, lowercase
Hex             = 18,   //uppercase
Hex0x           = 19 }; //0x, uppercase
                inline Printer&
                operator<<(Printer& p, BASE_ v) { return p.base(v); }

                enum CLEAR_ {
clear           };
                inline Printer&
                operator<<(Printer& p, CLEAR_ v) { (void)v; return p.start(); }


                enum POSITIVE_ {
noshowpos,
showpos         };
                inline Printer&
                operator<<(Printer& p, POSITIVE_ v) { return p.positive( v ); }

                enum ALPHA_ {
noshowalpha,
showalpha       };
                inline Printer&
                operator<<(Printer& p, ALPHA_ v) { return p.boolalpha( v ); }

                enum JUSTIFY_ {
right,
left            };
                inline Printer&
                operator<<(Printer& p, JUSTIFY_ v) { return p.justifyleft( v ); }

                struct SetBWF_ { BASE_ base; int W; char fill; };
                inline SetBWF_
setBWF          (BASE_ base, int W, char fill = ' ')
                {
                return { base, W, fill };
                }
                inline Printer&
                operator<<(Printer& p, SetBWF_ s)
                {
                return p.base(s.base).width(s.W, true).setfill(s.fill);
                }

                struct SetBwF_ { BASE_ base; int w; char fill; };
                inline SetBwF_
setBwF          (BASE_ base, int w, char fill = ' ')
                {
                return { base, w, fill };
                }
                inline Printer&
                operator<<(Printer& p, SetBwF_ s)
                {
                return p.base(s.base).width(s.w).setfill(s.fill);
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
