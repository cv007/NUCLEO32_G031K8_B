#pragma once //Format.hpp

#include "MyStm32.hpp"

//==================================================================
//==================================================================
// FMT namespace
//      Print class
//      PrintNull class
//      operator<< code
//==================================================================
namespace FMT {

//enum values which will be used to choose a particular overloaded
// print function, where the enum value is either the only value
// needed (2 or more enums) or only needed to choose an overloaded
// function but not otherwise used (single value enums)
enum FMT_JUSTIFY    { right, left, internal };                  //if width padding, justify left, right, or internal to a number
enum FMT_BASE       { bin = 2 , oct = 8, dec = 10, hex = 16 };  //number base
enum FMT_ENDL       { endl };                                   //newline
enum FMT_SHOWBASE   { noshowbase, showbase };                   //0x (hex), 0b (bin), 0 (oct)
enum FMT_UPPERCASE  { nouppercase, uppercase };                 //hex uppercase or lowercase
enum FMT_SHOWALPHA  { noshowalpha, showalpha };                 //bin 1|0, "true"|"false"
enum FMT_SHOWPOS    { noshowpos, showpos };                     //+
enum FMT_RESET      { reset };                                  //all options reset
enum FMT_COUNTCLR   { countclr };                               //clear char out count
enum FMT_ENDL2      { endl2 };                                  //endl x2

//==================================================================
// FMT::Print class
//==================================================================
class Print {

        //constant values
        enum { PRECISION_MAX = 9 }; //max float precision, limited to 9 by use of 32bit integers in calculations

//----------
  public:
//----------

        //string
        // str only, str + padding, padding + str
        auto&
print   (const char* str)
        {
        auto pad = width_ - (int)__builtin_strlen(str); //padding size (will be used if >0)
        width_ = 0;                                     //always reset after use
        isNeg_ = false;                                 //clear for the other 2 functions (since they both will end up here)
        auto strwr = [&]{ while( *str ) write_( *str++ ); }; //function to write the string
        if( pad <= 0 or just_ == left ) strwr();        //print str first
        if( pad > 0 ){                                  //need to deal with padding
            while( pad-- > 0 ) write_( fill_ );         //print any needed padding
            if( just_ == right ) strwr();               //and print str if was not done already
            }
        return *this;
        }

        //unsigned int (32bits), prints as string (to above string function)
        auto&
print   (const u32 v)
        {
        static constexpr auto BUFSZ{ 32+2+1 };          //0bx...x-> 32+2 digits max (+1 for 0 termination)
        char buf[BUFSZ];
        u8 idx = BUFSZ;                                 //start past end, so pre-decrement bufidx
        auto insert = [&](char c){ buf[--idx] = c; };   //function to insert c to buf (idx decrementing)
        auto a = uppercase_ ? 'A' : 'a';
        auto u8toa = [&](u8 c){ return c<10 ? c+'0' : c-10+a; };
        insert( 0 );                                    //0 terminate
        auto u = v;                                     //make copy to use (v is const)
        auto w = just_ == internal ? width_ : 0;        //use width_ here if internal justify
        if( w ) width_ = 0;                             //if internal, clear width_ so not used when value printed
        while( insert(u8toa(u % base_)), w--, u /= base_ ){} //add printable digits to buf
        while( w-- > 0 ) insert( fill_ );               //add internal fill, if any
        switch( base_ ){                                //any other things to add
            case dec: if( isNeg_ ) insert('-'); else if( pos_ ) insert('+');    break;
            case bin: if( showbase_ ){ insert('b'); insert ('0'); }             break;
            case oct: if( showbase_ and v ) insert('0');                        break;
            case hex: if( showbase_ ){ insert('x'); insert('0'); }              break;
            }
        return print( &buf[idx] );                      //call string version of print
        }


        //float, prints as string (to above string function)
        auto&
print   (const float cf)
        {
        //check for nan/inf
        if( __builtin_isnan(cf) ) return print( "nan" );
        if( __builtin_isinf_sign(cf) ) return print( "inf" );
        //we are limited by choice to using 32bit integers, so check for limits we can handle
        //raw float value of 0x4F7FFFFF is 4294967040.0, and the next value we will exceed a 32bit integer
        if( (cf > 4294967040.0) or (cf < -4294967040.0) ) return print( "ovf" );

        //values used to get fractional part into integer, based on precision 1-9
        static constexpr u32 mulTbl[PRECISION_MAX+1]
             { 1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000 };
        static constexpr auto BUFSZ{ 22 };          //10.10\0
        char str[BUFSZ];                            //fill top to bottom (high to low)
        auto idx = BUFSZ;                           //top+1, since pre-decrementing
        auto insert = [&](char c){ str[--idx] = c; }; //pre-decrement idx so is always pointing to start

        auto pre = precision_;                      //copy
        auto f = cf;                                //copy
        if( f < 0.0 ){ isNeg_ = true; f = -f; }     //make positive if needed
        auto fi = (u32)f;                           //fi = integer part
        insert(0);                                  //0 terminate string

        //deal with fractional part if precision is not 0
        if( pre ){
            auto pw = mulTbl[pre];                  //table mul value to get desired fractional part moved up
            f = (f - fi) * pw;                      //f = desired fractional part moved up to integer
            auto i = (u32)f;                        //i = integer part of desired fraction
            f -= i;                                 //f now contains the remaining/unused fraction

            if( (f >= 0.5) and (++i >= pw) ){       //if need to round up- inc i, check for overflow (the table value)
                i = 0;                              //i overflow, set i to 0
                fi++;                               //propogate into (original) integer part
                }
            while( insert((i % 10) + '0'), i /= 10, --pre ){} //add each fractional digit
            insert('.');                            //add dp
            }

        //deal with integer part
        while( insert((fi % 10) + '0'), fi /= 10 ){}//add each integer digit
        if( isNeg_ ) insert('-');                   //if neg, now add '-'
        else if( pos_ ) insert('+');                //if positive and pos wanted, add '+'

        return print( &str[idx] );                  //call string version of print
        }

        //reset all options to default (except newline), clear count
        auto&
print   (FMT_RESET e)
        {
        (void)e;
        just_ = left;
        showbase_ = noshowbase;
        uppercase_ = nouppercase;
        alpha_ = noshowalpha;
        pos_ = noshowpos;
        isNeg_ = false;
        count_ = 0;
        width_ = 0;
        fill_ = ' ';
        base_ = dec;
        precision_ = PRECISION_MAX;
        return *this;
        }

        //all other printing overloaded print functions
        auto& print     (const i32 n)       { u32 nu = n; if( n < 0 ){ isNeg_ = true; nu = -nu; } return print( nu ); }
        auto& print     (const int n)       { u32 nu = n; if( n < 0 ){ isNeg_ = true; nu = -nu; } return print( nu ); }
        auto& print     (const i16 n)       { return print( (i32)n ); }
        auto& print     (const u16 n)       { return print( (u32)n ); }
        auto& print     (const char c)      { write_( c ); return *this;}
        auto& print     (const bool tf)     { return alpha_ ? print( tf ? "true" : "false" ) : print( tf ? '1' : '0'); }

        //non-printing overloaded print functions deduced via enum
        auto& print     (FMT_BASE e)        { base_ = e; return *this;}
        auto& print     (FMT_SHOWBASE e)    { showbase_ = e; return *this;}
        auto& print     (FMT_UPPERCASE e)   { uppercase_ = e; return *this;}
        auto& print     (FMT_SHOWALPHA e)   { alpha_ = e; return *this;}
        auto& print     (FMT_SHOWPOS e)     { pos_ = e; return *this;}
        auto& print     (FMT_JUSTIFY e)     { just_ = e; return *this;}
        auto& print     (FMT_ENDL e)        { (void)e; return print( nl_ ); }
        auto& print     (FMT_ENDL2 e)       { (void)e; return print( nl_), print( nl_ ); }
        auto& print     (FMT_COUNTCLR e)    { (void)e; count_ = 0; return *this;}

        //non-printing functions needing a non-enum value, typically called by operator<< code
        auto& width     (int v)             { width_ = v; return *this; } //setw
        auto& fill      (int v)             { fill_ = v; return *this; } //setfill
        auto& precision (int v)             { precision_ = v > PRECISION_MAX ? PRECISION_MAX : v; return *this; } //setprecision

        //setup newline char(s) (up to 2)
        auto& newline   (const char* str)   { nl_[0] = str[0]; nl_[1] = str[1]; return *this; } //setnewline

        //return current char count (the only public function that does not return *this)
        int   count     ()                  { return count_; }

//----------
  private:
//----------

        //a helper write so we can keep a count of chars written (successfully)
        //(if any write fails as defined by the parent class (returns false), the failure
        // is only reflected in the count and not used any further)
        void write_  (const char c)     { if( write(c) ) count_++; }

        virtual
        bool write  (const char) = 0; //parent class creates this function

        char            nl_[3]      { '\n', '\0', '\0' };
        FMT_JUSTIFY     just_       { left };
        FMT_SHOWBASE    showbase_   { noshowbase };
        FMT_UPPERCASE   uppercase_  { nouppercase };
        FMT_SHOWALPHA   alpha_      { noshowalpha };
        FMT_SHOWPOS     pos_        { noshowpos };
        bool            isNeg_      { false };  //inform other functions a number was originally negative
        int             width_      { 0 };
        char            fill_       { ' ' };
        int             count_      { 0 };
        u8              precision_  { PRECISION_MAX };
        FMT_BASE        base_       { dec };

};

//==================================================================
// FMT::PrintNull class (no  output, optimizes away all uses)
//==================================================================
class PrintNull {}; //just a class type, nothing inside

//==================================================================
// operator <<
//==================================================================
                //for PrintNull- do nothing, and
                // return the same PrintNull reference passed in
                // (nothing done, no code produced when optimized)
                template<typename T> PrintNull& //anything passed in
operator <<     (PrintNull& p, T t){ (void)t; return p; }


                //for Print-
                //use structs to contain a value or values and make it unique
                // so values can be distinguished from values to print,
                // also allows more arguments to be passed into operator <<

                //everything else pass to print() and let the Print class sort it out
                template<typename T> Print&
operator <<     (Print& p, T t) { return p.print( t ); }

                //the functions which require a value will return
                //a struct with a value or values which then gets used by <<
                //all functions called in Print are public, so no need to 'friend' these

// << setw(10)
struct Setw                     { int n; };
inline Setw     setw            (int n)              { return {n}; }
inline Print&   operator<<      (Print& p, Setw s)   { return p.width(s.n); }

// << setfill(' ')
struct Setf                     { char c; };
inline Setf     setfill         (char c)             { return {c}; }
inline Print&   operator<<      (Print& p, Setf s)   { return p.fill(s.c); }

// << setprecision(4)
struct Setp                     { int n; };
inline Setp     setprecision    (int n)              { return {n}; }
inline Print&   operator<<      (Print& p, Setp s)   { return p.precision(s.n); }

// << cdup('=', 40)
struct Setdup                   { char c; int n; };
inline Setdup   cdup            (char c, int n)      { return {c,n}; }
inline Print&   operator<<      (Print& p, Setdup s) { p.width(s.n); return p.print(""); }

// << setwf(8,'0')
struct Setwf                    { int n; char c; };
inline Setwf    setwf           (int n, char c)      { return {n,c}; }
inline Print&   operator<<      (Print& p, Setwf s)  { p.width(s.n); return p.fill(s.c); }

// << hexpad(8) << 0x1a  -->> 0000001a
struct Padh                     { int n; };
inline Padh     hexpad          (int n)              { return {n}; }
inline Print&   operator<<      (Print& p, Padh s)   { return p << hex << nouppercase << noshowbase << internal << setwf(s.n,'0'); }

// << Hexpad(8) << 0x1a  -->> 0000001A
struct PadH                     { int n; };
inline PadH     Hexpad          (int n)              { return {n}; }
inline Print&   operator<<      (Print& p, PadH s)   { return p << hex << uppercase << noshowbase << internal << setwf(s.n,'0'); }

// << hex0xpad(8) << 0x1a  -->> 0x0000001a
struct Padh0x                   { int n; };
inline Padh0x   hex0xpad        (int n)              { return {n}; }
inline Print&   operator<<      (Print& p, Padh0x s) { return p << hex << nouppercase << showbase << internal << setwf(s.n,'0'); }

// << Hex0xpad(8) << 0x1a  -->> 0x0000001A
struct PadH0x                   { int n; };
inline PadH0x   Hex0xpad        (int n)              { return {n}; }
inline Print&   operator<<      (Print& p, PadH0x s) { return p << hex << uppercase << showbase << internal << setwf(s.n,'0'); }

// << decpad(8) << 123  -->> 00000123
struct PadD                     { int n; };
inline PadD     decpad          (int n)              { return {n}; }
inline Print&   operator<<      (Print& p, PadD s)   { return p << dec << internal << setwf(s.n,'0'); }

// << binpad(8) << 123  -->> 01111011
struct PadB                     { int n; };
inline PadB     binpad          (int n)              { return {n}; }
inline Print&   operator<<      (Print& p, PadB s)   { return p << bin << noshowbase << internal << setwf(s.n,'0'); }

// << bin0bpad(8) << 123  -->> 0b01111011
struct PadB0b                   { int n; };
inline PadB0b   bin0bpad        (int n)              { return {n}; }
inline Print&   operator<<      (Print& p, PadB0b s) { return p << bin << showbase << internal << setwf(s.n,'0'); }

} // FMT namespace end
//==================================================================
//==================================================================


/*-------------------------------------------------------------
    ansi codes

    colors need FG or BG preceeding
    << CLS << FG BLUE << BG WHITE << "fg blue, bg white"
    << ITALIC << FG RGB(200,100,50) << "italic rgb(200,100,50)"
--------------------------------------------------------------*/
#define ANSI_CSI            "\033["

#define FG                  ANSI_CSI "38;2;"
#define BG                  ANSI_CSI "48;2;"
#define RGB(r,g,b)          #r";"#g";"#b"m"


#define ANSI_CLS             ANSI_CSI "2J"
#define ANSI_HOME            ANSI_CSI "1;1H"
#define ANSI_RESET           ANSI_CLS ANSI_HOME ANSI_NORMAL
#define ANSI_ITALIC          ANSI_CSI "3m"
#define ANSI_NORMAL          ANSI_CSI "0m"
#define ANSI_UNDERLINE       ANSI_CSI "4m"

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
#define GHFMT_WHITE              RGB(248,248,255)
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

