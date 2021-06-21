#pragma once //Format.hpp

#include "MyStm32.hpp"

/*-------------------------------------------------------------
    Format options - set first 3 as needed
--------------------------------------------------------------*/
#define FMT_DOUBLE  0 //(will also get u64 and float)
#define FMT_U64     0 //u64 support
#define FMT_FLOAT   0 //can have float without u64/double

//double
#if     (defined(FMT_DOUBLE) && FMT_DOUBLE)
#define FMT_DOUBLE_ 1
#define FMT_FLOAT_TYPE_ double
#define FMT_PREMAX_ 14 //precision max (limit is 16)
#else
#define FMT_DOUBLE_ 0
#endif
//float only
#if     (defined(FMT_FLOAT) && FMT_FLOAT) && !FMT_DOUBLE_
#define FMT_FLOAT_  1
#define FMT_FLOAT_TYPE_ float
#define FMT_PREMAX_ 7 //precision max (limit is 9)
#else
#define FMT_FLOAT_  0
#endif
//u64 or double
#if     (defined(FMT_U64) && FMT_U64) || FMT_DOUBLE_
using   u64 = uint64_t;
using   i64 = int64_t;
#define FMT_USE_U64_ 1
#define FMT_U64_    u64 //all integer functions call u64 or i64
#define FMT_I64_    i64 //and in this case are actually u64/i64
#define FMT_BUFMAX_ 66  //64bit bin w/showbase is 66 chars
#else
#define FMT_USE_U64_ 0
#define FMT_U64_    u32 //no u64 support, so call the u32/i32
#define FMT_I64_    i32 //functions instead
#define FMT_BUFMAX_ 34  //32bit bin w/showbase is 34 chars
#endif
/*-------------------------------------------------------------
    Format

    simple class to inherit for cout style 'printing'
    via virtual put function which has a signature of-

        bool put(const char)
        write return value is false if there is an error

    all data goes directly out to the device put function,
    if any buffering wanted it has to be done in the device
    class that inherits this

    documentation-

    examples-

--------------------------------------------------------------*/
class Format {

//change function attributes for functions that return Format&
#define self    [[ gnu::noinline ]] Format
//#define self  Format  //no attributes

//-------------|
    public:
//-------------|

                using u32 = uint32_t;
                using i32 = int32_t;
                using u16 = uint16_t;
                using i16 = int16_t;
                using u8 = uint8_t;
                using i8 = int8_t;

                auto
count           (){ return count_; }

                auto
errors          (){ return errors_; }


// all functions below returns self&


                //set to 1 or 2 chars you want for newline
                //if a set to 0, will get '\0' output
                //(first char in NL_ is always used, second char only if not 0)
                self&
newline         (const char a, const char b = 0)
                { NL_[0] = a; NL_[1] = b; return *this; }

                // reset options,  << reset
                self&
reset           ()
                {
                count_ = 0;
                errors_ = 0;
                optionB_ = 10;
                optionFIL_ = 0;
                optionWMIN_ = 0;
                optionWMAX_ = 0;
            #if FMT_DOUBLE_ || FMT_FLOAT_
                optionPRE_ = 9; //0 is a usable value (no decimal), so cannot use 0
            #endif
                optionPOS_ = false;
                optionSB_ = false;
                optionBA_ = false;
                optionJL_ = false;
                optionUC_ = false;
                return *this;
                }

                // << setw(n) - minumum width, n limited to sane value via OPTIONWMIN_MAX
                self&
width           (const int v)
                { optionWMIN_ = v > OPTIONWMIN_MAX ? OPTIONWMIN_MAX : v; return *this; }

                // << setwmax(40) - maximum width (truncate output)
                self&
widthmax        (const unsigned int v)
                { optionWMAX_ = v; return *this; }

                // << bin|oct|dec|hex
                // base is max of 16, as the hex table is only 0-F
                self&
base            (const int v)
                {
                if( v >= 16 ) optionB_ = 16;
                else optionB_ = v bitand 10;//-> base 2,8,10
                //if 1,4,5 sneaked in, switch resulting 0 to base 10
                if( not optionB_ ) optionB_ = 10;
                return *this;
                }

            #if FMT_DOUBLE_ || FMT_FLOAT_
                // << setprecision(n)
                self&
precision       (const int v)
                { optionPRE_ = v > FMT_PREMAX_ ? FMT_PREMAX_ : v; return *this; }
            #endif

                // << setfill('char') (default value is ' ', unset is also ' ')
                self&
fill            (const char c = ' ')
                { optionFIL_ = c; return *this; }

                // << noshowpos , << showpos , + for dec base values
                self&
positive        (const bool tf)
                { optionPOS_ = tf; return *this; }

                // << noshowalpha , << showalpha , bool "true"/"false or 1 0
                self&
boolalpha       (const bool tf)
                { optionBA_ = tf; return *this; }

                // << left, << right , justify output left/right if min width > output
                self&
justify         (const bool tf)
                { optionJL_ = tf; return *this; }

                // << uppercase, << nouppercase , for hex only (A-F/a-f)
                self&
uppercase       (const bool tf)
                { optionUC_ = tf; return *this; }

                // << showbase, << noshowbase , 0x 0b 0 (for oct if value not 0)
                self&
showbase        (const bool tf)
                { optionSB_ = tf; return *this; }

                // << endl , newline as set in NL_
                self&
newline         ()
                { //if NL_[0] was set to \0, will get it
                put_( NL_[0] );
                if( NL_[1] ) put_( NL_[1] );
                return *this;
                }


                //string
                self&
operator<<      (const char* str)
                {
                auto w = optionWMIN_;
                auto wmax = optionWMAX_;                    //if 0, first --wmax will make it 0xFFFF (effectively no max limit)
                auto fc = optionFIL_ ? optionFIL_ : ' ';    //if 0, is ' '
                u32 i = w ? __builtin_strlen( str ) : 0;    //if w set, need str length
                auto fill = [&]{ while( (w-- > i) and --wmax ) put_( fc ); }; //lambda
                if( not optionJL_ ) fill();                 //justify right, fill first
                while( *str and --wmax ) put_( *str++ );  //srite str
                if( optionJL_ ) fill();                     //justify left, fill last
                optionWMIN_ = 0;                            //setw always cleared after use
                return *this;
                }

                //u64 (or u32 if no u64 support wanted)
                self&
operator<<      (const FMT_U64_ vu) //is u64 or u32
                {
                FMT_U64_ v = vu;
                auto div = optionB_; //2,8,10,16 (already limited to only these values)
                auto w = optionWMIN_;
                auto fc = optionFIL_ ? optionFIL_ : ' ';  //0 is ' '
                auto sb = (div == 8 and vu == 0) ? false : optionSB_; //oct 0 does not need showbase
                u32 i = 0; //buf index
                char buf[w > FMT_BUFMAX_ ? w : FMT_BUFMAX_]; //64bit binary with showbase uses 66 chars, 32bit 34
                if( v == 0 ) buf[i++] = '0'; //if 0, add '0' as below loop will be skipped
                //convert number
                while( v ){
                    auto c = hexTable[v % div];
                    if( c >= 'a' and optionUC_ ) c and_eq compl (1<<5); //to uppercase if hex/uppercase
                    buf[i++] = c;
                    v /= div;
                    }
                //lambda functions
                auto fill = [&](){ while( i < w ) buf[i++] = fc; };
                auto xtras = [&](){
                    if( div == 10 ) {                           //dec
                        if( optionNEG_ ) buf[i++] = '-';        //negative
                        else if( optionPOS_ ) buf[i++] = '+';   //positive and + wanted
                        }
                    else if( sb ){ // 2,8,16, showbase
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
                while( i ) put_( buf[--i] );
                optionNEG_ = false; //always clear after use
                optionWMIN_ = 0; //always clear after use
                return *this;
                }

            #if FMT_DOUBLE_ || FMT_FLOAT_
                //double or float
                self&
operator<<      (const FMT_FLOAT_TYPE_ d)
                {
                FMT_FLOAT_TYPE_ f = d;
                //check for nan/inf
                if( __builtin_isnan(f) ) return operator<<( "nan" );
                if( __builtin_isinf_sign(f) ) return operator<<( "inf" );
                if( f < 0 ){ optionNEG_ = true; f = -f; }
                //save/restore any options we change that should normally remain unchanged
                auto b = optionB_; optionB_ = 10;  //switch to dec
                FMT_U64_ vi = f; //integer part
                //get 1 more decimal precision than we need, so we can round up
                auto i = optionPRE_;
                FMT_U64_ mul = 10;                  //start at 1 more (10 instead of 1)
                while( i-- ) mul *= 10;             //create multiplier from PRE value
                FMT_U64_ vu = (f - vi) * mul;       //fractional part now integer
                if( (vu % 10) >= 5 ) vu += 10;      //round up? (all are away from 0)
                vu /= 10;                           //drop the extra decimal
                if( optionPRE_ == 0 and vu ) vi++;  //precision set to 0, then use vu to round up
                operator<<( vi );                   //write integer
                if( optionPRE_ ){                   //if precision not 0, write decimal
                    operator<<('.');                //decimal point
                    optionWMIN_ = optionPRE_;       //min length from precision
                    auto sp = optionPOS_; optionPOS_ = false; //no +
                    auto fil = optionFIL_; optionFIL_ = '0'; //0 pad
                    operator<<( vu );               //write decimal
                    optionFIL_ = fil;               //restore values
                    optionPOS_ = sp;
                    }
                optionB_ = b;                       //back to original base
                return *this;
                }
            #endif //FMT_DOUBLE_ || FMT_FLOAT_


                // the i64 (or i32) handles the negative numbers, then sends to u64 or u32
                // only negated if < 0 when using dec base


                //i64 (or i32)
                self&
operator<<      (const FMT_I64_ v)
                {
                if( v >= 0 or optionB_ != 10 ) return operator<<( (FMT_U64_)v );
                optionNEG_ = true;
                return operator<<( (FMT_U64_)-v );
                }


            #if FMT_USE_U64_ //then need u32/i32 versions
                self&
operator<<      (const u32 vu) { return operator<<( (u64)vu ); }
                self&
operator<<      (const i32 v) { return operator<<( (i64)v ); }
            #endif

                self&
operator<<      (const i16 v) { return operator<<( (FMT_I64_)v ); }
                self&
operator<<      (const u16 vu) { return operator<<( (FMT_U64_)vu ); }
                self&
operator<<      (const i8 v) { return operator<<( (FMT_I64_)v ); }
                self&
operator<<      (const u8 vu) { return operator<<( (FMT_U64_)vu ); }
                self&
operator<<      (const char v) { put_( v bitand 0xFF ); return *this; }
                self&
operator<<      (const bool v)
                {
                if( optionBA_ ) return operator<<( v ? "true" : "false" );
                return operator<<( (FMT_U64_)v );
                }

//-------------|
    private:
//-------------|

                //parent class has the put function
                virtual bool
put             (const char) = 0;

                //helper put, so we can also inc count for each char written
                //and count errors (whatever an error may be)
                void
put_            (const char c)
                { if( put(c) ) count_++; else errors_++; }


                static constexpr char hexTable[]{ "0123456789abcdef" };
                static constexpr auto OPTIONWMIN_MAX{ 128 }; //maximum value of optionWMIN_

                char NL_[3]     {"\r\n"};   //newline combo, can be changed at runtime
                u16  count_     { 0 };      //number of chars printed
                u16  errors_    { 0 };      //store any errors along the way
                u16  optionWMIN_{ 0 };      //minimum width
                u16  optionWMAX_{ 0 };      //maximum width
                u8   optionB_   { 10 };     //base 2,8,10,16
            #if FMT_DOUBLE_ || FMT_FLOAT_
                u8   optionPRE_ { 9 };      //float/double precision (decimal places)
            #endif
                char optionFIL_ { 0 };      //setfill char (0 is ' ')
                bool optionUC_  { false };  //uppercase/nouppercase
                bool optionSB_  { false };  //showbase/noshowbase
                bool optionNEG_ { false };  //is negative?
                bool optionPOS_ { false };  //showpos/noshowpos
                bool optionBA_  { false };  //boolalpha/noboolalpha
                bool optionJL_  { false };  //(justify) left/right

        #undef self
        #undef FMT_U64_
        #undef FMT_I64_
        #undef FMT_BUFMAX_
        #undef FMT_USE_U64_
        #undef FMT_PREMAX_

};


/*-------------------------------------------------------------
    Format helpers for << put in a fmt namespace

    can bring in namespace if wanted-
        using namespace fmt;

* = non-standard

    setw(w)         set minimum width n (Format class sets a limit to this value)
*   setwmax(n)      set maximum width of output (0 is no max limit)

    setfill(c)      set fill char, default is ' '

    endl            write newline combo as specified in Format class

*   bin             set base to 2
    oct             set base to 8
    dec             set base to 10 (default)
    hex             set base to 16

*   reset           set options to default, clear count, error

    noshowpos       no + for dec
    showpos         show + for dec that are positive

    showalpha       bool is "true" or "false"
    noshowalpha     bool is 1 or 0 (treated as unsigned int)

    left            justify output left
    right           justify output right

    uppercase       hex output uppercase
    nouppercase     hex output lowercase

    setprecision    set decimal precision for float/double

    all options remain set except for setw(), which is cleared
    after use, also <<clear resets all options
--------------------------------------------------------------*/
namespace fmt {

// w/argument - << name(arg)

                struct Setw_fmt { int n; };
                inline Setw_fmt
setw            (int n) { return {n}; }
                inline Format&
                operator<<(Format& p, Setw_fmt s)
                { return p.width(s.n); }

                struct SetwMax_fmt { int n; };
                inline SetwMax_fmt
setwmax         (int n) { return {n}; }
                inline Format&
                operator<<(Format& p, SetwMax_fmt s)
                { return p.widthmax(s.n); }

                struct Setfill_fmt { char c; };
                inline Setfill_fmt
setfill         (char c = ' ') { return {c}; }
                inline Format&
                operator<<(Format& p, Setfill_fmt s)
                { return p.fill(s.c); }

            #if FMT_DOUBLE_ || FMT_FLOAT_
                struct Setprecision_fmt { int n; };
                inline Setprecision_fmt
setprecision    (int n) { return {n}; }
                inline Format&
                operator<<(Format& p, Setprecision_fmt s)
                { return p.precision(s.n); }
            #endif
            #undef FMT_DOUBLE_ //done with these defines
            #undef FMT_FLOAT_

//no argumets -  << name <<

                enum ENDL_fmt {
endl            };
                inline Format&
                operator<<(Format& p, ENDL_fmt e)
                { (void)e; return p.newline(); }

                enum BASE_fmt {
bin             = 2,
oct             = 8,
dec             = 10,
hex             = 16 };
                inline Format&
                operator<<(Format& p, BASE_fmt e)
                { return p.base(e); }

                enum RESET_fmt {
reset           };
                inline Format&
                operator<<(Format& p, RESET_fmt e)
                { (void)e; return p.reset(); }

                enum POSITIVE_fmt {
noshowpos,
showpos         };
                inline Format&
                operator<<(Format& p, POSITIVE_fmt e)
                { return p.positive(e); }

                enum ALPHA_fmt {
noshowalpha,
showalpha       };
                inline Format&
                operator<<(Format& p, ALPHA_fmt e)
                { return p.boolalpha(e); }

                enum JUSTIFY_fmt {
right,
left            };
                inline Format&
                operator<<(Format& p, JUSTIFY_fmt e)
                { return p.justify(e); }

                enum UPPERCASE_fmt {
nouppercase,
uppercase       };
                inline Format&
                operator<<(Format& p, UPPERCASE_fmt e)
                { return p.uppercase(e); }

                enum SHOWBASE_fmt {
noshowbase,
showbase        };
                inline Format&
                operator<<(Format& p, SHOWBASE_fmt e)
                { return p.showbase(e); }


//combo helpers to reduce some verbosity, macros seem to be the
//easiest instead of creating a bunch more of the above
//plus they all use the standard cout properties so these
//also work on the pc when testing (except for the endlc, since
//reset is not standard so would need to be changed)
#define setwf(n,c)      fmt::setw(n) << fmt::setfill(c)
#define hex0x           fmt::hex << fmt::showbase
#define Hex             fmt::hex << fmt::uppercase
#define Hex0x           fmt::hex << fmt::showbase << fmt::uppercase
#define bin0b           fmt::bin << fmt::showbase
#define endlr           fmt::endl << fmt::reset
#define endl2           fmt::endl << fmt::endl
#define endl2r          fmt::endl << fmt::endl << fmt::reset
#define cdup(c,n)       setwf(n,c) << ""

}



/*------------------------------------------------------------------------------
    Null Format device - a black hole
        overrides all the public functions in Format,
        but none of them do anything here, so compiler will optimize away
        all uses

    // Usart debugDev;      //not wanted anymore
    NullFormat debugDev;    //so make it a NullFormat device
------------------------------------------------------------------------------*/
class NullFormat : public Format {

                //Format virtual put, 1 char
                virtual bool //unused, as it is never called
put             (const char c) { (void)c; return 0; }

public:
auto count      (){ return 0; }
auto errors     (){ return 0; }
auto newline    (const char a, const char b = 0) { (void)a; (void)b; return *this; }
auto reset      () { return *this; }
auto width      (const int v) { (void)v; return *this; }
auto widthmax   (const unsigned int v) { (void)v; return *this; }
auto base       (const int v) { (void)v; return *this; }
auto precision  (const int v) { (void)v; return *this; }
auto fill       (const char c = ' ') { (void)c; return *this; }
auto positive   (const bool tf) { (void)tf; return *this; }
auto boolalpha  (const bool tf) { (void)tf; return *this; }
auto justifyleft(const bool tf) { (void)tf; return *this; }
auto uppercase  (const bool tf) { (void)tf; return *this; }
auto showbase   (const bool tf) { (void)tf; return *this; }
auto newline    () { return *this; }

                template<typename T> // << anything
auto operator<< (T t) { return *this; }


};


/*------------------------------------------------------------------------------
    Buffer Format device - like snprintf with some extras

    BufFormat<32> bp;   //32 usable bytes allocated for buffer
                        //33 bytes allocated for buffer (buffer adds 1)

    buffer is always 0 terminated in the class, although possible to manipulate
    the buffer outside as a pointer is available via buf(), size/length will
    0 the last byte so cannot get into problems (the last extra byte allocated
    in the buffer belongs to the buffer so is free to clear it anytime)

    bp << "test" << 123; // bp.buf_ = "test123", 0 terminated
    can use bp in a Format << , which returns its buffer
    dev << bp << 456; // prints "test123456"
    bp.clear() << "hello"; // buf = "hello",  0 terminated
------------------------------------------------------------------------------*/
template<unsigned N>
class BufFormat : public Format {

//-------------|
    public:
//-------------|

BufFormat       (){ buf_[0] = 0; } //0 terminate

                auto
buf             () { return buf_; }
                auto
clear           () { buf_[0] = 0; count_ = 0; return *this; }
                auto //in case someone messed with buffer, 0 terminate first
length          () { buf_[N] = 0; return __builtin_strlen(buf_); }
                auto
size            () { return length(); }
                auto
operator[]      (int n) { return (n >= 0 and (n < (N-1))) ? buf_[n] : '\0'; }

//-------------|
    private:
//-------------|

                //Format virtual put, 1 char
                virtual bool
put             (const char c)
                {
                if( count_ < (N-1) ) {
                    buf_[count_++] = c;
                    buf_[count_] = 0; //0 terminate
                    return true;
                    }
                return false;
                }

                char buf_[N+1]; //we add the space for terminating '\0'
                u16 count_{0};

};
namespace fmt {

                //so can  Format << bufFormat  and get the buffer printed out
                template<unsigned N>
                inline Format&
                operator<<(Format& p, BufFormat<N>& b)
                { return p.operator<<( b.buf() ); }

}


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

using namespace fmt;
