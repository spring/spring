/*============================================================================
PROMINENT NOTICE: THIS IS A DERIVATIVE WORK OF THE ORIGINAL SOFTFLOAT CODE
CHANGES:
    Removed processors include
    This file serves as a bridge to the streflop system
Nicolas Brodu, 2006
=============================================================================*/

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
| Include common integer types and flags.
*----------------------------------------------------------------------------*/
#include "../System.h"


namespace streflop {
namespace SoftFloat {

// Use the types from System.h, some could be more "convenient"
typedef int8_t flag;
typedef uint8_t uint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint32_t uint32;
typedef int32_t int32;
typedef uint64_t uint64;
typedef int64_t int64;
// And these are exact by construction
typedef uint8_t bits8;
typedef int8_t sbits8;
typedef uint16_t bits16;
typedef int16_t sbits16;
typedef uint32_t bits32;
typedef int32_t sbits32;
typedef uint64_t bits64;
typedef int64_t sbits64;


// softfloat needs boolean TRUE/FALSE
#undef TRUE
#undef FALSE
enum {
    FALSE = 0,
    TRUE  = 1
};

// Streflop Bridge: Complete the missing defined that were in the processor files
#if __FLOAT_WORD_ORDER == 1234
#ifndef LITTLEENDIAN
#define LITTLEENDIAN
#endif
#elif __FLOAT_WORD_ORDER == 4321
#ifndef BIGENDIAN
#define BIGENDIAN
#endif
#endif

// 64-bit int types are assumed to exist in other parts of streflop
#define BITS64

// How to define a long long 64-bit constant
#define LIT64( a ) a##LL

// From original comment: If a compiler does not support explicit inlining,
// this macro should be defined to be 'static'.
// However, with C++, this has become obsolete
#define INLINE extern inline

}
}
