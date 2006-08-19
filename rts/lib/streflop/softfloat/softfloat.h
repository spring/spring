/*============================================================================
PROMINENT NOTICE: THIS IS A DERIVATIVE WORK OF THE ORIGINAL SOFTFLOAT CODE
CHANGES:
    Comment out FLOAT128
    Removed all signed char => char
    Inserted this file is a namespace
    Added variable to control the sending of real system traps
    Protect this header by a #define
    pack the fields of floatx80, just in case (should be useless)
Nicolas Brodu, 2006
=============================================================================*/
#ifndef SOFTFLOAT_H
#define SOFTFLOAT_H

namespace streflop {
namespace SoftFloat {

// Control which of the softfloat exceptions will send real system traps
// Uses streflop FE_XXX flags, see the softfloat-specialize file
extern int float_exception_realtraps;

/*============================================================================

This C header file is part of the SoftFloat IEC/IEEE Floating-point Arithmetic
Package, Release 2b.

Written by John R. Hauser.  This work was made possible in part by the
International Computer Science Institute, located at Suite 600, 1947 Center
Street, Berkeley, California 94704.  Funding was partially provided by the
National Science Foundation under grant MIP-9311980.  The original version
of this code was written as part of a project to build a fixed-point vector
processor in collaboration with the University of California at Berkeley,
overseen by Profs. Nelson Morgan and John Wawrzynek.  More information
is available through the Web page `http://www.cs.berkeley.edu/~jhauser/
arithmetic/SoftFloat.html'.

THIS SOFTWARE IS DISTRIBUTED AS IS, FOR FREE.  Although reasonable effort has
been made to avoid it, THIS SOFTWARE MAY CONTAIN FAULTS THAT WILL AT TIMES
RESULT IN INCORRECT BEHAVIOR.  USE OF THIS SOFTWARE IS RESTRICTED TO PERSONS
AND ORGANIZATIONS WHO CAN AND WILL TAKE FULL RESPONSIBILITY FOR ALL LOSSES,
COSTS, OR OTHER PROBLEMS THEY INCUR DUE TO THE SOFTWARE, AND WHO FURTHERMORE
EFFECTIVELY INDEMNIFY JOHN HAUSER AND THE INTERNATIONAL COMPUTER SCIENCE
INSTITUTE (possibly via similar legal warning) AGAINST ALL LOSSES, COSTS, OR
OTHER PROBLEMS INCURRED BY THEIR CUSTOMERS AND CLIENTS DUE TO THE SOFTWARE.

Derivative works are acceptable, even for commercial purposes, so long as
(1) the source code for the derivative work includes prominent notice that
the work is derivative, and (2) the source code includes prominent notice with
these four paragraphs for those parts of this code that are retained.

=============================================================================*/

/*----------------------------------------------------------------------------
| The macro `FLOATX80' must be defined to enable the extended double-precision
| floating-point format `floatx80'.  If this macro is not defined, the
| `floatx80' type will not be defined, and none of the functions that either
| input or output the `floatx80' type will be defined.  The same applies to
| the `FLOAT128' macro and the quadruple-precision format `float128'.
*----------------------------------------------------------------------------*/
#define FLOATX80
//#define FLOAT128

/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point types.
*----------------------------------------------------------------------------*/
typedef unsigned int float32;
typedef unsigned long long float64;
#ifdef FLOATX80
typedef struct {
    unsigned long long low;
    unsigned short high
#ifdef __GNUC__
    // Should be useless, since it's aligned at 64 already
     __attribute__ ((__packed__));
#endif
    ;
} floatx80;
#endif
#ifdef FLOAT128
typedef struct {
    unsigned long long low, high;
} float128;
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point underflow tininess-detection mode.
*----------------------------------------------------------------------------*/
extern char float_detect_tininess;
enum {
    float_tininess_after_rounding  = 0,
    float_tininess_before_rounding = 1
};

/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point rounding mode.
*----------------------------------------------------------------------------*/
extern char float_rounding_mode;
enum {
    float_round_nearest_even = 0,
    float_round_down         = 1,
    float_round_up           = 2,
    float_round_to_zero      = 3
};

/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point exception flags.
*----------------------------------------------------------------------------*/
extern char float_exception_flags;
enum {
    float_flag_invalid   =  1,
    float_flag_divbyzero =  4,
    float_flag_overflow  =  8,
    float_flag_underflow = 16,
    float_flag_inexact   = 32
};

/*----------------------------------------------------------------------------
| Routine to raise any or all of the software IEC/IEEE floating-point
| exception flags.
*----------------------------------------------------------------------------*/
void float_raise( char );

/*----------------------------------------------------------------------------
| Software IEC/IEEE integer-to-floating-point conversion routines.
*----------------------------------------------------------------------------*/
float32 int32_to_float32( int );
float64 int32_to_float64( int );
#ifdef FLOATX80
floatx80 int32_to_floatx80( int );
#endif
#ifdef FLOAT128
float128 int32_to_float128( int );
#endif
float32 int64_to_float32( long long );
float64 int64_to_float64( long long );
#ifdef FLOATX80
floatx80 int64_to_floatx80( long long );
#endif
#ifdef FLOAT128
float128 int64_to_float128( long long );
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE single-precision conversion routines.
*----------------------------------------------------------------------------*/
int float32_to_int32( float32 );
int float32_to_int32_round_to_zero( float32 );
long long float32_to_int64( float32 );
long long float32_to_int64_round_to_zero( float32 );
float64 float32_to_float64( float32 );
#ifdef FLOATX80
floatx80 float32_to_floatx80( float32 );
#endif
#ifdef FLOAT128
float128 float32_to_float128( float32 );
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE single-precision operations.
*----------------------------------------------------------------------------*/
float32 float32_round_to_int( float32 );
float32 float32_add( float32, float32 );
float32 float32_sub( float32, float32 );
float32 float32_mul( float32, float32 );
float32 float32_div( float32, float32 );
float32 float32_rem( float32, float32 );
float32 float32_sqrt( float32 );
char float32_eq( float32, float32 );
char float32_le( float32, float32 );
char float32_lt( float32, float32 );
char float32_eq_signaling( float32, float32 );
char float32_le_quiet( float32, float32 );
char float32_lt_quiet( float32, float32 );
char float32_is_signaling_nan( float32 );

/*----------------------------------------------------------------------------
| Software IEC/IEEE double-precision conversion routines.
*----------------------------------------------------------------------------*/
int float64_to_int32( float64 );
int float64_to_int32_round_to_zero( float64 );
long long float64_to_int64( float64 );
long long float64_to_int64_round_to_zero( float64 );
float32 float64_to_float32( float64 );
#ifdef FLOATX80
floatx80 float64_to_floatx80( float64 );
#endif
#ifdef FLOAT128
float128 float64_to_float128( float64 );
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE double-precision operations.
*----------------------------------------------------------------------------*/
float64 float64_round_to_int( float64 );
float64 float64_add( float64, float64 );
float64 float64_sub( float64, float64 );
float64 float64_mul( float64, float64 );
float64 float64_div( float64, float64 );
float64 float64_rem( float64, float64 );
float64 float64_sqrt( float64 );
char float64_eq( float64, float64 );
char float64_le( float64, float64 );
char float64_lt( float64, float64 );
char float64_eq_signaling( float64, float64 );
char float64_le_quiet( float64, float64 );
char float64_lt_quiet( float64, float64 );
char float64_is_signaling_nan( float64 );

#ifdef FLOATX80

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision conversion routines.
*----------------------------------------------------------------------------*/
int floatx80_to_int32( floatx80 );
int floatx80_to_int32_round_to_zero( floatx80 );
long long floatx80_to_int64( floatx80 );
long long floatx80_to_int64_round_to_zero( floatx80 );
float32 floatx80_to_float32( floatx80 );
float64 floatx80_to_float64( floatx80 );
#ifdef FLOAT128
float128 floatx80_to_float128( floatx80 );
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision rounding precision.  Valid
| values are 32, 64, and 80.
*----------------------------------------------------------------------------*/
extern char floatx80_rounding_precision;

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision operations.
*----------------------------------------------------------------------------*/
floatx80 floatx80_round_to_int( floatx80 );
floatx80 floatx80_add( floatx80, floatx80 );
floatx80 floatx80_sub( floatx80, floatx80 );
floatx80 floatx80_mul( floatx80, floatx80 );
floatx80 floatx80_div( floatx80, floatx80 );
floatx80 floatx80_rem( floatx80, floatx80 );
floatx80 floatx80_sqrt( floatx80 );
char floatx80_eq( floatx80, floatx80 );
char floatx80_le( floatx80, floatx80 );
char floatx80_lt( floatx80, floatx80 );
char floatx80_eq_signaling( floatx80, floatx80 );
char floatx80_le_quiet( floatx80, floatx80 );
char floatx80_lt_quiet( floatx80, floatx80 );
char floatx80_is_signaling_nan( floatx80 );

#endif

#ifdef FLOAT128

/*----------------------------------------------------------------------------
| Software IEC/IEEE quadruple-precision conversion routines.
*----------------------------------------------------------------------------*/
int float128_to_int32( float128 );
int float128_to_int32_round_to_zero( float128 );
long long float128_to_int64( float128 );
long long float128_to_int64_round_to_zero( float128 );
float32 float128_to_float32( float128 );
float64 float128_to_float64( float128 );
#ifdef FLOATX80
floatx80 float128_to_floatx80( float128 );
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE quadruple-precision operations.
*----------------------------------------------------------------------------*/
float128 float128_round_to_int( float128 );
float128 float128_add( float128, float128 );
float128 float128_sub( float128, float128 );
float128 float128_mul( float128, float128 );
float128 float128_div( float128, float128 );
float128 float128_rem( float128, float128 );
float128 float128_sqrt( float128 );
char float128_eq( float128, float128 );
char float128_le( float128, float128 );
char float128_lt( float128, float128 );
char float128_eq_signaling( float128, float128 );
char float128_le_quiet( float128, float128 );
char float128_lt_quiet( float128, float128 );
char float128_is_signaling_nan( float128 );

#endif

// Close namespaces
}
}

#endif
