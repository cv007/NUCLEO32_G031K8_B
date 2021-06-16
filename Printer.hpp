//Printer.hpp
#pragma once

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
//match to this class name
//if class name changes, change this to match
class Printer;
using PrinterT = Printer;

class Printer {

//-------------|
    public:
//-------------|

//change all PrinterT functions to noinline
#define PrinterT [[ gnu::noinline ]] PrinterT

                using u32 = uint32_t;
                using i32 = int32_t;


                auto
count           (){ return count_; }

                auto
errors          (){ return errors_; }


                // everything below returns PrinterT&


                //set to 1 or 2 chars you want for {N}ewline
                //NL_[2] already 0, cannot change so no need to set
                PrinterT&
newline         (const char a, const char b = 0) { NL_[0] = a; NL_[1] = b; return *this; }


                // reset options,  << clear
                PrinterT&
clear           ()
                {
                count_ = 0;
                errors_ = 0;
                optionB_ = 0;
                optionFIL_ = 0;
                optionWMIN_ = 0;
                optionWMAX_ = 0;
                optionSB_ = false;
                optionBA_ = false;
                optionJL_ = false;
                optionUC_ = false;
                return *this;
                }

                // << setw(n) - n limited to OPTIONWMIN_MAX
                PrinterT&
width           (int v)
                {
                optionWMIN_ = v > OPTIONWMIN_MAX ? OPTIONWMIN_MAX : v;
                return *this;
                }

                // << setwmax(40) - maximum width
                PrinterT&
widthmax        (unsigned int v)
                {
                optionWMAX_ = v;
                return *this;
                }

                // << bin|oct|dec|hex
                // base is max of 16, as the hex table is only 0-F
                PrinterT&
base            (int v)
                {
                if( v >= 16 ) optionB_ = 16;
                else optionB_ = v bitand 10;//-> base 2,8,10
                //if 1,4,5 sneaked in, resulting 0 is also base 10
                return *this;
                }

                // << setfill('char') (default is ' ')
                PrinterT&
fill            (char c) { optionFIL_ = c; return *this; }

                // << noshowpos , << showpos
                PrinterT&
positive        (bool tf) { optionPOS_ = tf; return *this; }

                // << noshowalpha , << showalpha
                PrinterT&
boolalpha       (bool tf) { optionBA_ = tf; return *this; }

                // << left, << right
                PrinterT&
justifyleft     (bool tf) { optionJL_ = tf; return *this; }

                // << uppercase, << nouppercase
                PrinterT&
uppercase       (bool tf) { optionUC_ = tf; return *this; }

                // << showbase, << noshowbase
                PrinterT&
showbase        (bool tf) { optionSB_ = tf; return *this; }

                // << endl
                PrinterT&
newline         () { writeStr( NL_ ); return *this; }


                //string
                PrinterT&
operator<<      (const char* str)
                {
                auto w = optionWMIN_;
                auto wmax = optionWMAX_; //if 0, first --wmax will make it 0xFFFFFFFF
                auto fc = optionFIL_ ? optionFIL_ : ' ';
                u32 i = w ? __builtin_strlen( str ) : 0;
                auto fill = [&]{ while( (w-- > i) and --wmax ) write_( fc ); };
                if( not optionJL_ ) fill(); //justify right, fill first
                while( *str and --wmax ) write_( *str++ );
                if( optionJL_ ) fill(); //justify left, fill last
                optionWMIN_ = 0;
                return *this;
                }

                //unsigned int
                PrinterT&
operator<<      (u32 vu)
                {
                auto div = optionB_ ? optionB_ : 10; //2,8,10,16 (0 is 10)
                auto w = optionWMIN_;
                auto fc = optionFIL_ ? optionFIL_ : ' ';  //0 is ' '
                u32 i = 0;
                char buf[w > 34 ? w : 34]; //32bit binary with showbase uses 34 chars
                if( div == 10 and optionNEG_ ) vu = -vu;
                while( vu ){
                    auto c = hexTable[vu % div];
                    if( c >= 'a' and optionUC_ ) c and_eq compl (1<<5); //to uppercase
                    buf[i++] = c;
                    vu /= div;
                    }
                if( i == 0 ) buf[i++] = '0'; //if vu was 0, insert a '0' as while loop was skipped
                auto fill = [&](){ while( i < w ) buf[i++] = fc; };
                auto xtras = [&](){
                    if( div == 10 ) {
                        if( optionNEG_ ) buf[i++] = '-'; //negative dec
                        else if( optionPOS_ ) buf[i++] = '+'; //dec is positive and + wanted
                        }
                    else if( optionSB_ ){ // 2,8,16, showbase
                        if( div == 16 ) buf[i++] = 'x';
                        else if( div == 2 ) buf[i++] = 'b';
                        buf[i++] = '0';
                        }
                };
                //add -, 0b, 0x, 0, now if optionFIL_ is not '0'
                //else add fill first
                //(remember this is a reverse buffer)
                if( fc == '0' ){ fill(); xtras(); } else { xtras(); fill(); }
                //i is 1 past our last char, i will be at least 1
                while( i ) write_( buf[--i] );
                optionNEG_ = false;
                optionWMIN_ = 0;
                return *this;
                }

                //signed int
                PrinterT&
operator<<      (i32 v)
                {
                if( v < 0 ) optionNEG_ = true;
                return operator<<( (u32)v );
                }

                //signed short
                PrinterT&
operator<<      (i16 v)
                {
                return operator<<( (i32)v );
                }

                //unsigned short
                PrinterT&
operator<<      (u16 vu)
                {
                return operator<<( (u32)vu );
                }

                //char
                PrinterT&
operator<<      (u8 vu) { return operator<<( (u32)vu ); }

                //char
                PrinterT&
operator<<      (char v) { write_( v bitand 0xFF ); return *this; }

                //bool
                PrinterT&
operator<<      (bool v)
                {
                if( optionBA_ ) return operator<<( v ? "true" : "false" );
                return operator<<( (u32)v );
                }

//-------------|
    private:
//-------------|

                //parent class has the write function
                virtual bool write(const char) = 0;

                static constexpr char hexTable[]{ "0123456789abcdef" };
                static constexpr auto OPTIONWMIN_MAX{ 128 }; //maximum value of optionWMIN_

                char NL_[3]     {"\r\n"};   //newline combo, can be changed at runtime
                u32  count_     { 0 };      //number of chars printed
                u32  errors_    { 0 };      //store any errors along the way
                u32  optionWMIN_{ 0 };      //minimum width
                u32  optionWMAX_{ 0 };      //maximum width
                u32  optionB_   { 0 };      //base 2,8,10,16 (0 is also base 10)
                char optionFIL_ { 0 };      //setfill char (0 is ' ')
                bool optionUC_  { false };  //uppercase/nouppercase
                bool optionSB_  { false };  //showbase/noshowbase
                bool optionNEG_ { false };  //is negative?
                bool optionPOS_ { false };  //showpos/noshowpos
                bool optionBA_  { false };  //boolalpha/noboolalpha
                bool optionJL_  { false };  //(justify) left/right


                //helper write, so we can also inc count for each char written
                void
write_          (char c)
                { if( write(c) ) count_++; else errors_++; }

                void
writeStr        (const char* str)
                { while( *str ) write_( *str++ ); }

#undef PrinterT

};


/*------------------------------------------------------------------------------
    Null Printer device - a black hole
------------------------------------------------------------------------------*/
class NullPrinter : public Printer {

                //Printer virtual write, 1 char
                virtual bool
write           (const char c){ (void)c; return 0; }

};


/*------------------------------------------------------------------------------
    Buffer Printer device - like snprintf

    BufPrinter<32> bp;  //32 bytes allocated for buffer, includes 0 terminator
                        //so only 31 useful

    bp << "test" << 123; // buf = "test123" 0 terminated
    can use bp in a Printer <<
    dev << bp << 456; // prints "test123456"
    bp.clear() << "hello"; // buf = "hello" 0 terminated
------------------------------------------------------------------------------*/
template<unsigned N>
class BufPrinter : public Printer {

//-------------|
    public:
//-------------|

BufPrinter      (){ buf_[0] = 0; }

                auto&
buf             (){ return buf_; }

                auto
clear           (){ buf_[0] = 0; count_ = 0; return *this; }

//-------------|
    private:
//-------------|

                //Printer virtual write, 1 char
                virtual bool
write           (const char c)
                {
                if( count_ < (N-1) ) {
                    buf_[count_++] = c;
                    buf_[count_] = 0;
                    return true;
                    }
                return false;
                }

                char buf_[N?N:1]; //at least 1, so is terminated
                u16 count_{0};

};

namespace prn { //so can << bufprinter  and get the buffer printed out

                template<unsigned N>
                inline PrinterT&
                operator<<(PrinterT& p, BufPrinter<N>& b)
                { p.operator<<( b.buf() ); return p; }

}




/*-------------------------------------------------------------
    Printer helpers for <<

    setw(w)         set minimum width n (Printer class sets a limit to this value)
*   setwmax(n)      set maximum width of output (0 is no max limit)

    setfill(c)      set fill char, default is ' '

    endl            write newline combo as specified in Printer class

*   bin             set base to 2
    oct             set base to 8
    dec             set base to 10 (default)
    hex             set base to 16

*   clear           set options to default, clear count, error

    noshowpos       no + for dec
    showpos         show + for dec that are positive

    showalpha       bool is "true" or "false"
    noshowalpha     bool is 1 or 0 (treated as unsigned int)

    left            justify output left
    right           justify output right

    uppercase       hex output uppercase
    nouppercase     hex output lowercase

    showbase        showbase for base 16,8,2 - 0x 0 0b
    noshowbase      do not show base


    all options remain set except for setw(), which is cleared
    after use, also <<clear resets all options
--------------------------------------------------------------*/
namespace prn {

//w/arguments -  << name() <<

                struct Setw_prn { int n; };
                inline Setw_prn
setw            (int n) { return {n}; }
                inline PrinterT&
                operator<<(PrinterT& p, Setw_prn s)
                { return p.width(s.n); }

                struct SetwMax_prn { int n; };
                inline SetwMax_prn
setwmax         (int n) { return {n}; }
                inline PrinterT&
                operator<<(PrinterT& p, SetwMax_prn s)
                { return p.widthmax(s.n); }

                struct Setfill_prn { char c; };
                inline Setfill_prn
setfill         (char c = ' ') { return {c}; }
                inline PrinterT&
                operator<<(PrinterT& p, Setfill_prn s)
                { return p.fill(s.c); }

                enum ENDL_prn {
endl            };
                inline PrinterT&
                operator<<(PrinterT& p, ENDL_prn e)
                { (void)e; return p.newline(); }

//no argumets -  << name <<

                enum BASE_prn {
bin             = 2,
oct             = 8,
dec             = 10,
hex             = 16 };
                inline PrinterT&
                operator<<(PrinterT& p, BASE_prn e)
                { return p.base(e); }

                enum CLEAR_prn {
clear           };
                inline PrinterT&
                operator<<(PrinterT& p, CLEAR_prn e)
                { (void)e; return p.clear(); }


                enum POSITIVE_prn {
noshowpos,
showpos         };
                inline PrinterT&
                operator<<(PrinterT& p, POSITIVE_prn e)
                { return p.positive(e); }

                enum ALPHA_prn {
noshowalpha,
showalpha       };
                inline PrinterT&
                operator<<(PrinterT& p, ALPHA_prn e)
                { return p.boolalpha(e); }

                enum JUSTIFY_prn {
right,
left            };
                inline PrinterT&
                operator<<(PrinterT& p, JUSTIFY_prn e)
                { return p.justifyleft(e); }

                enum UPPERCASE_prn {
nouppercase,
uppercase       };
                inline PrinterT&
                operator<<(PrinterT& p, UPPERCASE_prn e)
                { return p.uppercase(e); }

                enum SHOWBASE_prn {
noshowbase,
showbase        };
                inline PrinterT&
                operator<<(PrinterT& p, SHOWBASE_prn e)
                { return p.showbase(e); }

}


/*-------------------------------------------------------------
    Printer helpers for ansi codes

    seems defines are currently the bext way to do these
    strings in an efficient way, as they have the advantage
    of being string literals

    colors need FG or BG preceeding
    << CLS << FG BLUE << BG WHITE << "fg blue, bg white"
    << ITALIC << FG RGB(200,100,50) << "italic rgb(200,100,50)"
--------------------------------------------------------------*/
#define CSI             "\033["

#define FG              CSI "38;2;"
#define BG              CSI "48;2;"
#define RGB(r,g,b)      #r";"#g";"#b"m"

#define CLS             CSI "2J"
#define HOME            CSI "1;1H"
#define RESET           CLS HOME NORMAL
#define ITALIC          CSI "3m"
#define NORMAL          CSI "0m"
#define UNDERLINE       CSI "4m"

//SVG colors
#define ALICE_BLUE               RGB(240,248,255)
#define ANTIQUE_WHITE            RGB(250,235,215)
#define AQUA                     RGB(0,255,255)
#define AQUAMARINE               RGB(127,255,212)
#define AZURE                    RGB(240,255,255)
#define BEIGE                    RGB(245,245,220)
#define BISQUE                   RGB(255,228,196)
#define BLACK                    RGB(0,0,0)
#define BLANCHED_ALMOND          RGB(255,235,205)
#define BLUE                     RGB(0,0,255)
#define BLUE_VIOLET              RGB(138,43,226)
#define BROWN                    RGB(165,42,42)
#define BURLY_WOOD               RGB(222,184,135)
#define CADET_BLUE               RGB(95,158,160)
#define CHARTREUSE               RGB(127,255,0)
#define CHOCOLATE                RGB(210,105,30)
#define CORAL                    RGB(255,127,80)
#define CORNFLOWER_BLUE          RGB(100,149,237)
#define CORNSILK                 RGB(255,248,220)
#define CRIMSON                  RGB(220,20,60)
#define CYAN                     RGB(0,255,255)
#define DARK_BLUE                RGB(0,0,139)
#define DARK_CYAN                RGB(0,139,139)
#define DARK_GOLDEN_ROD          RGB(184,134,11)
#define DARK_GRAY                RGB(169,169,169)
#define DARK_GREEN               RGB(0,100,0)
#define DARK_KHAKI               RGB(189,183,107)
#define DARK_MAGENTA             RGB(139,0,139)
#define DARK_OLIVE_GREEN         RGB(85,107,47)
#define DARK_ORANGE              RGB(255,140,0)
#define DARK_ORCHID              RGB(153,50,204)
#define DARK_RED                 RGB(139,0,0)
#define DARK_SALMON              RGB(233,150,122)
#define DARK_SEA_GREEN           RGB(143,188,143)
#define DARK_SLATE_BLUE          RGB(72,61,139)
#define DARK_SLATE_GRAY          RGB(47,79,79)
#define DARK_TURQUOISE           RGB(0,206,209)
#define DARK_VIOLET              RGB(148,0,211)
#define DEEP_PINK                RGB(255,20,147)
#define DEEP_SKY_BLUE            RGB(0,191,255)
#define DIM_GRAY                 RGB(105,105,105)
#define DODGER_BLUE              RGB(30,144,255)
#define FIRE_BRICK               RGB(178,34,34)
#define FLORAL_WHITE             RGB(255,250,240)
#define FOREST_GREEN             RGB(34,139,34)
#define FUCHSIA                  RGB(255,0,255)
#define GAINSBORO                RGB(220,220,220)
#define GHOST_WHITE              RGB(248,248,255)
#define GOLD                     RGB(255,215,0)
#define GOLDEN_ROD               RGB(218,165,32)
#define GRAY                     RGB(128,128,128)
#define GREEN                    RGB(0,128,0)
#define GREEN_YELLOW             RGB(173,255,47)
#define HONEY_DEW                RGB(240,255,240)
#define HOT_PINK                 RGB(255,105,180)
#define INDIAN_RED               RGB(205,92,92)
#define INDIGO                   RGB(75,0,130)
#define IVORY                    RGB(255,255,240)
#define KHAKI                    RGB(240,230,140)
#define LAVENDER                 RGB(230,230,250)
#define LAVENDER_BLUSH           RGB(255,240,245)
#define LAWN_GREEN               RGB(124,252,0)
#define LEMON_CHIFFON            RGB(255,250,205)
#define LIGHT_BLUE               RGB(173,216,230)
#define LIGHT_CORAL              RGB(240,128,128)
#define LIGHT_CYAN               RGB(224,255,255)
#define LIGHT_GOLDENROD_YELLOW   RGB(250,250,210)
#define LIGHT_GRAY               RGB(211,211,211)
#define LIGHT_GREEN              RGB(144,238,144)
#define LIGHT_PINK               RGB(255,182,193)
#define LIGHT_SALMON             RGB(255,160,122)
#define LIGHT_SEA_GREEN          RGB(32,178,170)
#define LIGHT_SKY_BLUE           RGB(135,206,250)
#define LIGHT_SLATE_GRAY         RGB(119,136,153)
#define LIGHT_STEEL_BLUE         RGB(176,196,222)
#define LIGHT_YELLOW             RGB(255,255,224)
#define LIME                     RGB(0,255,0)
#define LIME_GREEN               RGB(50,205,50)
#define LINEN                    RGB(250,240,230)
#define MAGENTA                  RGB(255,0,255)
#define MAROON                   RGB(128,0,0)
#define MEDIUM_AQUAMARINE        RGB(102,205,170)
#define MEDIUM_BLUE              RGB(0,0,205)
#define MEDIUM_ORCHID            RGB(186,85,211)
#define MEDIUM_PURPLE            RGB(147,112,219)
#define MEDIUM_SEA_GREEN         RGB(60,179,113)
#define MEDIUM_SLATE_BLUE        RGB(123,104,238)
#define MEDIUM_SPRING_GREEN      RGB(0,250,154)
#define MEDIUM_TURQUOISE         RGB(72,209,204)
#define MEDIUM_VIOLET_RED        RGB(199,21,133)
#define MIDNIGHT_BLUE            RGB(25,25,112)
#define MINT_CREAM               RGB(245,255,250)
#define MISTY_ROSE               RGB(255,228,225)
#define MOCCASIN                 RGB(255,228,181)
#define NAVAJO_WHITE             RGB(255,222,173)
#define NAVY                     RGB(0,0,128)
#define OLD_LACE                 RGB(253,245,230)
#define OLIVE                    RGB(128,128,0)
#define OLIVE_DRAB               RGB(107,142,35)
#define ORANGE                   RGB(255,165,0)
#define ORANGE_RED               RGB(255,69,0)
#define ORCHID                   RGB(218,112,214)
#define PALE_GOLDENROD           RGB(238,232,170)
#define PALE_GREEN               RGB(152,251,152)
#define PALE_TURQUOISE           RGB(175,238,238)
#define PALE_VIOLET_RED          RGB(219,112,147)
#define PAPAYA_WHIP              RGB(255,239,213)
#define PEACH_PUFF               RGB(255,218,185)
#define PERU                     RGB(205,133,63)
#define PINK                     RGB(255,192,203)
#define PLUM                     RGB(221,160,221)
#define POWDER_BLUE              RGB(176,224,230)
#define PURPLE                   RGB(128,0,128)
#define REBECCA_PURPLE           RGB(102,51,153)
#define RED                      RGB(255,0,0)
#define ROSY_BROWN               RGB(188,143,143)
#define ROYAL_BLUE               RGB(65,105,225)
#define SADDLE_BROWN             RGB(139,69,19)
#define SALMON                   RGB(250,128,114)
#define SANDY_BROWN              RGB(244,164,96)
#define SEA_GREEN                RGB(46,139,87)
#define SEA_SHELL                RGB(255,245,238)
#define SIENNA                   RGB(160,82,45)
#define SILVER                   RGB(192,192,192)
#define SKY_BLUE                 RGB(135,206,235)
#define SLATE_BLUE               RGB(106,90,205)
#define SLATE_GRAY               RGB(112,128,144)
#define SNOW                     RGB(255,250,250)
#define SPRING_GREEN             RGB(0,255,127)
#define STEEL_BLUE               RGB(70,130,180)
#define TAN                      RGB(210,180,140)
#define TEAL                     RGB(0,128,128)
#define THISTLE                  RGB(216,191,216)
#define TOMATO                   RGB(255,99,71)
#define TURQUOISE                RGB(64,224,208)
#define VIOLET                   RGB(238,130,238)
#define WHEAT                    RGB(245,222,179)
#define WHITE                    RGB(255,255,255)
#define WHITE_SMOKE              RGB(245,245,245)
#define YELLOW                   RGB(255,255,0)
#define YELLOW_GREEN             RGB(154,205,50)



//comment to prevent bringing in prn namespace to global namespace
using namespace prn;
