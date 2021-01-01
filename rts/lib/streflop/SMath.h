/*
	streflop: STandalone REproducible FLOating-Point
	Nicolas Brodu, 2006
	Code released according to the GNU Lesser General Public License

	Heavily relies on GNU Libm, itself depending on netlib fplibm, GNU MP,
	and IBM MP lib.
	Uses SoftFloat too.

	Please read the history and copyright information in the documentation
	provided with the source code
*/

// Included by the main streflop include file
// module broken apart for logical code separation
#ifndef STREFLOP_MATH_H
#define STREFLOP_MATH_H

// just in case, should already be included
#include "streflop.h"

// Names from the libm conversion
namespace streflop_libm {
using streflop::Simple;
using streflop::Double;
#ifdef Extended
using streflop::Extended;
#endif

	extern Simple __ieee754_sqrtf(Simple x);
	extern Simple __cbrtf(Simple x);
	extern Simple __ieee754_hypotf(Simple x, Simple y);
	extern Simple __ieee754_expf(Simple x);
	extern Simple __ieee754_logf(Simple x);
	extern Simple __ieee754_log2f(Simple x);
	extern Simple __ieee754_exp2f(Simple x);
	extern Simple __ieee754_log10f(Simple x);
	extern Simple __ieee754_powf(Simple x, Simple y);
	extern Simple __sinf(Simple x);
	extern Simple __cosf(Simple x);
	extern Simple __tanhf(Simple x);
	extern Simple __tanf(Simple x);
	extern Simple __ieee754_acosf(Simple x);
	extern Simple __ieee754_asinf(Simple x);
	extern Simple __atanf(Simple x);
	extern Simple __ieee754_atan2f(Simple x, Simple y);
	extern Simple __ieee754_coshf(Simple x);
	extern Simple __ieee754_sinhf(Simple x);
	extern Simple __ieee754_acoshf(Simple x);
	extern Simple __asinhf(Simple x);
	extern Simple __ieee754_atanhf(Simple x);
	extern Simple __fabsf(Simple x);
	extern Simple __floorf(Simple x);
	extern Simple __ceilf(Simple x);
	extern Simple __truncf(Simple x);
	extern Simple __ieee754_fmodf(Simple x, Simple y);
	extern Simple __ieee754_remainderf(Simple x, Simple y);
	extern Simple __remquof(Simple x, Simple y, int *quo);
	extern Simple __rintf(Simple x);
	extern long int __lrintf(Simple x);
	extern long long int __llrintf(Simple x);
	extern Simple __roundf(Simple x);
	extern long int __lroundf(Simple x);
	extern long long int __llroundf(Simple x);
	extern Simple __nearbyintf(Simple x);
	extern Simple __frexpf(Simple x, int *exp);
	extern Simple __ldexpf(Simple value, int exp);
	extern Simple __logbf(Simple x);
	extern int __ilogbf(Simple x);
	extern Simple __copysignf(Simple x);
	extern int __signbitf(Simple x);
	extern Simple __nextafterf(Simple x, Simple y);
	extern Simple __expm1f(Simple x);
	extern Simple __log1pf(Simple x);
	extern Simple __erff(Simple x);
	extern Simple __ieee754_j0f(Simple x);
	extern Simple __ieee754_j1f(Simple x);
	extern Simple __ieee754_jnf(int n, Simple x);
	extern Simple __ieee754_y0f(Simple x);
	extern Simple __ieee754_y1f(Simple x);
	extern Simple __ieee754_ynf(int n, Simple x);
	extern Simple __scalbnf(Simple x, int n);
	extern Simple __scalblnf(Simple x, long int n);
	extern int __fpclassifyf(Simple x);
	extern int __isnanf(Simple x);
	extern int __isinff(Simple x);
	extern Double __ieee754_sqrt(Double x);
	extern Double __cbrt(Double x);
	extern Double __ieee754_hypot(Double x, Double y);
	extern Double __ieee754_exp(Double x);
	extern Double __ieee754_log(Double x);
	extern Double __ieee754_log2(Double x);
	extern Double __ieee754_exp2(Double x);
	extern Double __ieee754_log10(Double x);
	extern Double __ieee754_pow(Double x, Double y);
	extern Double __sin(Double x);
	extern Double __cos(Double x);
	extern Double tan(Double x);
	extern Double __ieee754_acos(Double x);
	extern Double __ieee754_asin(Double x);
	extern Double atan(Double x);
	extern Double __ieee754_atan2(Double x, Double y);
	extern Double __ieee754_cosh(Double x);
	extern Double __ieee754_sinh(Double x);
	extern Double __tanh(Double x);
	extern Double __ieee754_acosh(Double x);
	extern Double __asinh(Double x);
	extern Double __ieee754_atanh(Double x);
	extern Double __fabs(Double x);
	extern Double __floor(Double x);
	extern Double __ceil(Double x);
	extern Double __trunc(Double x);
	extern Double __ieee754_fmod(Double x, Double y);
	extern Double __ieee754_remainder(Double x, Double y);
	extern Double __remquo(Double x, Double y, int *quo);
	extern Double __rint(Double x);
	extern long int __lrint(Double x);
	extern long long int __llrint(Double x);
	extern Double __round(Double x);
	extern long int __lround(Double x);
	extern long long int __llround(Double x);
	extern Double __nearbyint(Double x);
	extern Double __frexp(Double x, int *exp);
	extern Double __ldexp(Double value, int exp);
	extern Double __logb(Double x);
	extern int __ilogb(Double x);
	extern Double __copysign(Double x);
	extern int __signbit(Double x);
	extern Double __nextafter(Double x, Double y);
	extern Double __expm1(Double x);
	extern Double __log1p(Double x);
	extern Double __erf(Double x);
	extern Double __ieee754_j0(Double x);
	extern Double __ieee754_j1(Double x);
	extern Double __ieee754_jn(int n, Double x);
	extern Double __ieee754_y0(Double x);
	extern Double __ieee754_y1(Double x);
	extern Double __ieee754_yn(int n, Double x);
	extern Double __scalbn(Double x, int n);
	extern Double __scalbln(Double x, long int n);
	extern int __fpclassify(Double x);
	extern int __isnanl(Double x);
	extern int __isinf(Double x);
#ifdef Extended
	extern Extended __ieee754_sqrtl(Extended x);
	extern Extended __cbrtl(Extended x);
	extern Extended __ieee754_hypotl(Extended x, Extended y);
	extern Extended __ieee754_expl(Extended x);
	extern Extended __ieee754_logl(Extended x);
	extern Extended __sinl(Extended x);
	extern Extended __cosl(Extended x);
	extern Extended __tanl(Extended x);
	extern Extended __ieee754_asinl(Extended x);
	extern Extended __atanl(Extended x);
	extern Extended __ieee754_atan2l(Extended x, Extended y);
	extern Extended __ieee754_coshl(Extended x);
	extern Extended __ieee754_sinhl(Extended x);
	extern Extended __tanhl(Extended x);
	extern Extended __ieee754_acoshl(Extended x);
	extern Extended __asinhl(Extended x);
	extern Extended __ieee754_atanhl(Extended x);
	extern Extended __fabsl(Extended x);
	extern Extended __floorl(Extended x);
	extern Extended __ceill(Extended x);
	extern Extended __truncl(Extended x);
	extern Extended __ieee754_fmodl(Extended x, Extended y);
	extern Extended __ieee754_remainderl(Extended x, Extended y);
	extern Extended __remquol(Extended x, Extended y, int *quo);
	extern Extended __rintl(Extended x);
	extern long int __lrintl(Extended x);
	extern long long int __llrintl(Extended x);
	extern Extended __roundl(Extended x);
	extern long int __lroundl(Extended x);
	extern long long int __llroundl(Extended x);
	extern Extended __nearbyintl(Extended x);
	extern Extended __frexpl(Extended x, int *exp);
	extern Extended __ldexpl(Extended value, int exp);
	extern Extended __logbl(Extended x);
	extern int __ilogbl(Extended x);
	extern Extended __copysignl(Extended x);
	extern int __signbitl(Extended x);
	extern Extended __nextafterl(Extended x, Extended y);
	extern Extended __expm1l(Extended x);
	extern Extended __log1pl(Extended x);
	extern Extended __erfl(Extended x);
	extern Extended __ieee754_j0l(Extended x);
	extern Extended __ieee754_j1l(Extended x);
	extern Extended __ieee754_jnl(int n, Extended x);
	extern Extended __ieee754_y0l(Extended x);
	extern Extended __ieee754_y1l(Extended x);
	extern Extended __ieee754_ynl(int n, Extended x);
	extern Extended __scalbnl(Extended x, int n);
	extern Extended __scalblnl(Extended x, long int n);
	extern int __fpclassifyl(Extended x);
	extern int __isnanl(Extended x);
	extern int __isinfl(Extended x);
#endif // Extended
}


// Wrappers in our own namespace
namespace streflop {

// Stolen from math.h. All floating-point numbers can be put in one of these categories.
enum
{
	STREFLOP_FP_NAN = 0,
	STREFLOP_FP_INFINITE = 1,
	STREFLOP_FP_ZERO = 2,
	STREFLOP_FP_SUBNORMAL = 3,
	STREFLOP_FP_NORMAL = 4
};


// Simple and double are present in all configurations

	inline Simple sqrt(Simple x) {return streflop_libm::__ieee754_sqrtf(x);}
	inline Simple cbrt(Simple x) {return streflop_libm::__cbrtf(x);}
	inline Simple hypot(Simple x, Simple y) {return streflop_libm::__ieee754_hypotf(x,y);}

	inline Simple exp(Simple x) {return streflop_libm::__ieee754_expf(x);}
	inline Simple log(Simple x) {return streflop_libm::__ieee754_logf(x);}
	inline Simple log2(Simple x) {return streflop_libm::__ieee754_log2f(x);}
	inline Simple exp2(Simple x) {return streflop_libm::__ieee754_exp2f(x);}
	inline Simple log10(Simple x) {return streflop_libm::__ieee754_log10f(x);}
	inline Simple pow(Simple x, Simple y) {return streflop_libm::__ieee754_powf(x,y);}

	inline Simple sin(Simple x) {return streflop_libm::__sinf(x);}
	inline Simple cos(Simple x) {return streflop_libm::__cosf(x);}
	inline Simple tan(Simple x) {return streflop_libm::__tanf(x);}
	inline Simple acos(Simple x) {return streflop_libm::__ieee754_acosf(x);}
	inline Simple asin(Simple x) {return streflop_libm::__ieee754_asinf(x);}
	inline Simple atan(Simple x) {return streflop_libm::__atanf(x);}
	inline Simple atan2(Simple x, Simple y) {return streflop_libm::__ieee754_atan2f(x,y);}

	inline Simple cosh(Simple x) {return streflop_libm::__ieee754_coshf(x);}
	inline Simple sinh(Simple x) {return streflop_libm::__ieee754_sinhf(x);}
	inline Simple tanh(Simple x) {return streflop_libm::__tanhf(x);}
	inline Simple acosh(Simple x) {return streflop_libm::__ieee754_acoshf(x);}
	inline Simple asinh(Simple x) {return streflop_libm::__asinhf(x);}
	inline Simple atanh(Simple x) {return streflop_libm::__ieee754_atanhf(x);}

	inline Simple fabs(Simple x) {return streflop_libm::__fabsf(x);}
	inline Simple floor(Simple x) {return streflop_libm::__floorf(x);}
	inline Simple ceil(Simple x) {return streflop_libm::__ceilf(x);}
	inline Simple trunc(Simple x) {return streflop_libm::__truncf(x);}
	inline Simple fmod(Simple x, Simple y) {return streflop_libm::__ieee754_fmodf(x,y);}
	inline Simple remainder(Simple x, Simple y) {return streflop_libm::__ieee754_remainderf(x,y);}
	inline Simple remquo(Simple x, Simple y, int *quo) {return streflop_libm::__remquof(x,y,quo);}
	inline Simple rint(Simple x) {return streflop_libm::__rintf(x);}
	inline long int lrint(Simple x) {return streflop_libm::__lrintf(x);}
	inline long long int llrint(Simple x) {return streflop_libm::__llrintf(x);}
	inline Simple round(Simple x) {return streflop_libm::__roundf(x);}
	inline long int lround(Simple x) {return streflop_libm::__lroundf(x);}
	inline long long int llround(Simple x) {return streflop_libm::__llroundf(x);}
	inline Simple nearbyint(Simple x) {return streflop_libm::__nearbyintf(x);}

	inline Simple frexp(Simple x, int *exp) {return streflop_libm::__frexpf(x,exp);}
	inline Simple ldexp(Simple value, int exp) {return streflop_libm::__ldexpf(value,exp);}
	inline Simple logb(Simple x) {return streflop_libm::__logbf(x);}
	inline int ilogb(Simple x) {return streflop_libm::__ilogbf(x);}
	inline Simple copysign(Simple x) {return streflop_libm::__copysignf(x);}
#undef signbit
	inline int signbit (Simple x) {return streflop_libm::__signbitf(x);}
	inline Simple nextafter(Simple x, Simple y) {return streflop_libm::__nextafterf(x,y);}

	inline Simple expm1(Simple x) {return streflop_libm::__expm1f(x);}
	inline Simple log1p(Simple x) {return streflop_libm::__log1pf(x);}
	inline Simple erf(Simple x) {return streflop_libm::__erff(x);}
	inline Simple j0(Simple x) {return streflop_libm::__ieee754_j0f(x);}
	inline Simple j1(Simple x) {return streflop_libm::__ieee754_j1f(x);}
	inline Simple jn(int n, Simple x) {return streflop_libm::__ieee754_jnf(n,x);}
	inline Simple y0(Simple x) {return streflop_libm::__ieee754_y0f(x);}
	inline Simple y1(Simple x) {return streflop_libm::__ieee754_y1f(x);}
	inline Simple yn(int n, Simple x) {return streflop_libm::__ieee754_ynf(n,x);}
	inline Simple scalbn(Simple x, int n) {return streflop_libm::__scalbnf(x,n);}
	inline Simple scalbln(Simple x, long int n) {return streflop_libm::__scalblnf(x,n);}

#undef fpclassify
	inline int fpclassify(Simple x) {return streflop_libm::__fpclassifyf(x);}
#undef isnan
	inline int isnan(Simple x) {return streflop_libm::__isnanf(x);}
#undef isinf
	inline int isinf(Simple x) {return streflop_libm::__isinff(x);}
#undef isfinite
	inline int isfinite(Simple x) {return !(isnan(x) || isinf(x));}

	// Stolen from math.h and inlined instead of macroized.
	// Return nonzero value if X is neither zero, subnormal, Inf, nor NaN.  */
#undef isnormal
	inline int isnormal(Simple x) {return fpclassify(x) == STREFLOP_FP_NORMAL;}

	// Constants user may set
	extern const Simple SimplePositiveInfinity;
	extern const Simple SimpleNegativeInfinity;
	// Non-signaling NaN used for returning such results
	// Standard lets a large room for implementing different kinds of NaN
	// These NaN can be used for custom purposes
	// Threated as an integer, the bit pattern here may be incremented to get at least 2^20 different NaN custom numbers!
	// Note that when switching the left-most bit to 1, you can get another bunch of negative NaNs, whatever this mean.
	extern const Simple SimpleNaN;

	/** Generic C99 "macros" for unordered comparison
		Defined as inlined for each type, thanks to C++ overloading
	*/
#if defined isunordered
#undef isunordered
#endif // defined isunordered
	inline bool isunordered(Simple x, Simple y) {
		return (fpclassify(x) == STREFLOP_FP_NAN) || (fpclassify(y) == STREFLOP_FP_NAN);
	}
#if defined isgreater
#undef isgreater
#endif // defined isgreater
	inline bool isgreater(Simple x, Simple y) {
		return (!isunordered(x,y)) && (x > y);
	}
#if defined isgreaterequal
#undef isgreaterequal
#endif // defined isgreaterequal
	inline bool isgreaterequal(Simple x, Simple y) {
		return (!isunordered(x,y)) && (x >= y);
	}
#if defined isless
#undef isless
#endif // defined isless
	inline bool isless(Simple x, Simple y) {
		return (!isunordered(x,y)) && (x < y);
	}
#if defined islessequal
#undef islessequal
#endif // defined islessequal
	inline bool islessequal(Simple x, Simple y) {
		return (!isunordered(x,y)) && (x <= y);
	}
#if defined islessgreater
#undef islessgreater
#endif // defined islessgreater
	inline bool islessgreater(Simple x, Simple y) {
		return (!isunordered(x,y)) && ((x < y) || (x > y));
	}


// Add xxxf alias to ease porting existing code to streflop
// Additionally, using xxxf(number) avoids potential confusion

	inline Simple sqrtf(Simple x) {return sqrt(x);}
	inline Simple cbrtf(Simple x) {return cbrt(x);}
	inline Simple hypotf(Simple x, Simple y) {return hypot(x, y);}

	inline Simple expf(Simple x) {return exp(x);}
	inline Simple logf(Simple x) {return log(x);}
	inline Simple log2f(Simple x) {return log2(x);}
	inline Simple exp2f(Simple x) {return exp2(x);}
	inline Simple log10f(Simple x) {return log10(x);}
	inline Simple powf(Simple x, Simple y) {return pow(x, y);}

	inline Simple sinf(Simple x) {return sin(x);}
	inline Simple cosf(Simple x) {return cos(x);}
	inline Simple tanf(Simple x) {return tan(x);}
	inline Simple acosf(Simple x) {return acos(x);}
	inline Simple asinf(Simple x) {return asin(x);}
	inline Simple atanf(Simple x) {return atan(x);}
	inline Simple atan2f(Simple x, Simple y) {return atan2(x, y);}

	inline Simple coshf(Simple x) {return cosh(x);}
	inline Simple sinhf(Simple x) {return sinh(x);}
	inline Simple tanhf(Simple x) {return tanh(x);}
	inline Simple acoshf(Simple x) {return acosh(x);}
	inline Simple asinhf(Simple x) {return asinh(x);}
	inline Simple atanhf(Simple x) {return atanh(x);}

	inline Simple fabsf(Simple x) {return fabs(x);}
	inline Simple floorf(Simple x) {return floor(x);}
	inline Simple ceilf(Simple x) {return ceil(x);}
	inline Simple truncf(Simple x) {return trunc(x);}
	inline Simple fmodf(Simple x, Simple y) {return fmod(x,y);}
	inline Simple remainderf(Simple x, Simple y) {return remainder(x,y);}
	inline Simple remquof(Simple x, Simple y, int *quo) {return remquo(x, y, quo);}
	inline Simple rintf(Simple x) {return rint(x);}
	inline long int lrintf(Simple x) {return lrint(x);}
	inline long long int llrintf(Simple x) {return llrint(x);}
	inline Simple roundf(Simple x) {return round(x);}
	inline long int lroundf(Simple x) {return lround(x);}
	inline long long int llroundf(Simple x) {return llround(x);}
	inline Simple nearbyintf(Simple x) {return nearbyint(x);}

	inline Simple frexpf(Simple x, int *exp) {return frexp(x, exp);}
	inline Simple ldexpf(Simple value, int exp) {return ldexp(value,exp);}
	inline Simple logbf(Simple x) {return logb(x);}
	inline int ilogbf(Simple x) {return ilogb(x);}
	inline Simple copysignf(Simple x) {return copysign(x);}
	inline int signbitf(Simple x) {return signbit(x);}
	inline Simple nextafterf(Simple x, Simple y) {return nextafter(x, y);}

	inline Simple expm1f(Simple x) {return expm1(x);}
	inline Simple log1pf(Simple x) {return log1p(x);}
	inline Simple erff(Simple x) {return erf(x);}
	inline Simple j0f(Simple x) {return j0(x);}
	inline Simple j1f(Simple x) {return j1(x);}
	inline Simple jnf(int n, Simple x) {return jn(n, x);}
	inline Simple y0f(Simple x) {return y0(x);}
	inline Simple y1f(Simple x) {return y1(x);}
	inline Simple ynf(int n, Simple x) {return yn(n, x);}
	inline Simple scalbnf(Simple x, int n) {return scalbn(x, n);}
	inline Simple scalblnf(Simple x, long int n) {return scalbln(x, n);}

	inline int fpclassifyf(Simple x) {return fpclassify(x);}
	inline int isnanf(Simple x) {return isnan(x);}
	inline int isinff(Simple x) {return isinf(x);}
	inline int isfinitef(Simple x) {return isfinite(x);}
	inline int isnormalf(Simple x) {return isnormal(x);}

	inline bool isunorderedf(Simple x, Simple y) {return isunordered(x, y);}
	inline bool isgreaterf(Simple x, Simple y) {return isgreater(x, y);}
	inline bool isgreaterequalf(Simple x, Simple y) {return isgreaterequal(x, y);}
	inline bool islessf(Simple x, Simple y) {return isless(x, y);}
	inline bool islessequalf(Simple x, Simple y) {return islessequal(x, y);}
	inline bool islessgreaterf(Simple x, Simple y) {return islessgreater(x, y);}


// Declare Double functions
// Simple and double are present in all configurations

	inline Double sqrt(Double x) {return streflop_libm::__ieee754_sqrt(x);}
	inline Double cbrt(Double x) {return streflop_libm::__cbrt(x);}
	inline Double hypot(Double x, Double y) {return streflop_libm::__ieee754_hypot(x,y);}

	inline Double exp(Double x) {return streflop_libm::__ieee754_exp(x);}
	inline Double log(Double x) {return streflop_libm::__ieee754_log(x);}
	inline Double log2(Double x) {return streflop_libm::__ieee754_log2(x);}
	inline Double exp2(Double x) {return streflop_libm::__ieee754_exp2(x);}
	inline Double log10(Double x) {return streflop_libm::__ieee754_log10(x);}
	inline Double pow(Double x, Double y) {return streflop_libm::__ieee754_pow(x,y);}

	inline Double sin(Double x) {return streflop_libm::__sin(x);}
	inline Double cos(Double x) {return streflop_libm::__cos(x);}
	inline Double tan(Double x) {return streflop_libm::tan(x);}
	inline Double acos(Double x) {return streflop_libm::__ieee754_acos(x);}
	inline Double asin(Double x) {return streflop_libm::__ieee754_asin(x);}
	inline Double atan(Double x) {return streflop_libm::atan(x);}
	inline Double atan2(Double x, Double y) {return streflop_libm::__ieee754_atan2(x,y);}

	inline Double cosh(Double x) {return streflop_libm::__ieee754_cosh(x);}
	inline Double sinh(Double x) {return streflop_libm::__ieee754_sinh(x);}
	inline Double tanh(Double x) {return streflop_libm::__tanh(x);}
	inline Double acosh(Double x) {return streflop_libm::__ieee754_acosh(x);}
	inline Double asinh(Double x) {return streflop_libm::__asinh(x);}
	inline Double atanh(Double x) {return streflop_libm::__ieee754_atanh(x);}

	inline Double fabs(Double x) {return streflop_libm::__fabs(x);}
	inline Double floor(Double x) {return streflop_libm::__floor(x);}
	inline Double ceil(Double x) {return streflop_libm::__ceil(x);}
	inline Double trunc(Double x) {return streflop_libm::__trunc(x);}
	inline Double fmod(Double x, Double y) {return streflop_libm::__ieee754_fmod(x,y);}
	inline Double remainder(Double x, Double y) {return streflop_libm::__ieee754_remainder(x,y);}
	inline Double remquo(Double x, Double y, int *quo) {return streflop_libm::__remquo(x,y,quo);}
	inline Double rint(Double x) {return streflop_libm::__rint(x);}
	inline long int lrint(Double x) {return streflop_libm::__lrint(x);}
	inline long long int llrint(Double x) {return streflop_libm::__llrint(x);}
	inline Double round(Double x) {return streflop_libm::__round(x);}
	inline long int lround(Double x) {return streflop_libm::__lround(x);}
	inline long long int llround(Double x) {return streflop_libm::__llround(x);}
	inline Double nearbyint(Double x) {return streflop_libm::__nearbyint(x);}

	inline Double frexp(Double x, int *exp) {return streflop_libm::__frexp(x, exp);}
	inline Double ldexp(Double value, int exp) {return streflop_libm::__ldexp(value,exp);}
	inline Double logb(Double x) {return streflop_libm::__logb(x);}
	inline int ilogb(Double x) {return streflop_libm::__ilogb(x);}
	inline Double copysign(Double x) {return streflop_libm::__copysign(x);}
	inline int signbit(Double x) {return streflop_libm::__signbit(x);}
	inline Double nextafter(Double x, Double y) {return streflop_libm::__nextafter(x,y);}

	inline Double expm1(Double x) {return streflop_libm::__expm1(x);}
	inline Double log1p(Double x) {return streflop_libm::__log1p(x);}
	inline Double erf(Double x) {return streflop_libm::__erf(x);}
	inline Double j0(Double x) {return streflop_libm::__ieee754_j0(x);}
	inline Double j1(Double x) {return streflop_libm::__ieee754_j1(x);}
	inline Double jn(int n, Double x) {return streflop_libm::__ieee754_jn(n,x);}
	inline Double y0(Double x) {return streflop_libm::__ieee754_y0(x);}
	inline Double y1(Double x) {return streflop_libm::__ieee754_y1(x);}
	inline Double yn(int n, Double x) {return streflop_libm::__ieee754_yn(n,x);}
	inline Double scalbn(Double x, int n) {return streflop_libm::__scalbn(x,n);}
	inline Double scalbln(Double x, long int n) {return streflop_libm::__scalbln(x,n);}

	inline int fpclassify(Double x) {return streflop_libm::__fpclassify(x);}
	inline int isnan(Double x) {return streflop_libm::__isnanl(x);}
	inline int isinf(Double x) {return streflop_libm::__isinf(x);}
	inline int isfinite(Double x) {return !(isnan(x) || isinf(x));}

	// Stolen from math.h and inlined instead of macroized.
	// Return nonzero value if X is neither zero, subnormal, Inf, nor NaN.  */
	inline int isnormal(Double x) {return fpclassify(x) == STREFLOP_FP_NORMAL;}

	// Constants user may set
	extern const Double DoublePositiveInfinity;
	extern const Double DoubleNegativeInfinity;
	// Non-signaling NaN used for returning such results
	// Standard lets a large room for implementing different kinds of NaN
	// These NaN can be used for custom purposes
	// Threated as an integer, the bit pattern here may be incremented to get at least 2^20 different NaN custom numbers!
	// Note that when switching the left-most bit to 1, you can get another bunch of negative NaNs, whatever this mean.
	extern const Double DoubleNaN;

	/** Generic C99 "macros" for unordered comparison
		Defined as inlined for each type, thanks to C++ overloading
	*/
	inline bool isunordered(Double x, Double y) {
		return (fpclassify(x) == STREFLOP_FP_NAN) || (fpclassify (y) == STREFLOP_FP_NAN);
	}
	inline bool isgreater(Double x, Double y) {
		return (!isunordered(x,y)) && (x > y);
	}
	inline bool isgreaterequal(Double x, Double y) {
		return (!isunordered(x,y)) && (x >= y);
	}
	inline bool isless(Double x, Double y) {
		return (!isunordered(x,y)) && (x < y);
	}
	inline bool islessequal(Double x, Double y) {
		return (!isunordered(x,y)) && (x <= y);
	}
	inline bool islessgreater(Double x, Double y) {
		return (!isunordered(x,y)) && ((x < y) || (x > y));
	}

// Extended are not always available
#ifdef Extended

	inline Extended cbrt(Extended x) {return streflop_libm::__cbrtl(x);}
	inline Extended hypot(Extended x, Extended y) {return streflop_libm::__ieee754_hypotl(x,y);}


// Missing from libm: temporarily switch to Double and execute the Double version,
// then switch back to Extended and return the result
	inline Extended sqrt(Extended x) {streflop_init<Double>(); Double res = sqrt(Double(x)); streflop_init<Extended>(); return Extended(res);}

	inline Extended exp(Extended x) {streflop_init<Double>(); Double res = exp(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended log(Extended x) {streflop_init<Double>(); Double res = log(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended log2(Extended x) {streflop_init<Double>(); Double res = log2(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended exp2(Extended x) {streflop_init<Double>(); Double res = exp2(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended log10(Extended x) {streflop_init<Double>(); Double res = log10(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended pow(Extended x, Extended y) {streflop_init<Double>(); Double res = pow(Double(x), Double(y)); streflop_init<Extended>(); return Extended(res);}
	inline Extended sin(Extended x) {streflop_init<Double>(); Double res = sin(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended cos(Extended x) {streflop_init<Double>(); Double res = cos(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended tan(Extended x) {streflop_init<Double>(); Double res = tan(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended acos(Extended x) {streflop_init<Double>(); Double res = acos(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended asin(Extended x) {streflop_init<Double>(); Double res = asin(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended atan(Extended x) {streflop_init<Double>(); Double res = atan(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended atan2(Extended x, Extended y) {streflop_init<Double>(); Double res = atan2(Double(x), Double(y)); streflop_init<Extended>(); return Extended(res);}

	inline Extended cosh(Extended x) {streflop_init<Double>(); Double res = cosh(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended sinh(Extended x) {streflop_init<Double>(); Double res = sinh(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended tanh(Extended x) {streflop_init<Double>(); Double res = tanh(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended acosh(Extended x) {streflop_init<Double>(); Double res = acosh(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended asinh(Extended x) {streflop_init<Double>(); Double res = asinh(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended atanh(Extended x) {streflop_init<Double>(); Double res = atanh(Double(x)); streflop_init<Extended>(); return Extended(res);}


	inline Extended expm1(Extended x) {streflop_init<Double>(); Double res = expm1(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended log1p(Extended x) {streflop_init<Double>(); Double res = log1p(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended erf(Extended x) {streflop_init<Double>(); Double res = erf(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended j0(Extended x) {streflop_init<Double>(); Double res = j0(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended j1(Extended x) {streflop_init<Double>(); Double res = j1(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended jn(int n, Extended x) {streflop_init<Double>(); Double res = jn(n,Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended y0(Extended x) {streflop_init<Double>(); Double res = y0(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended y1(Extended x) {streflop_init<Double>(); Double res = y1(Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended yn(int n, Extended x) {streflop_init<Double>(); Double res = yn(n,Double(x)); streflop_init<Extended>(); return Extended(res);}
	inline Extended scalbn(Extended x, int n) {streflop_init<Double>(); Double res = scalbn(Double(x),n); streflop_init<Extended>(); return Extended(res);}
	inline Extended scalbln(Extended x, long int n) {streflop_init<Double>(); Double res = scalbln(Double(x),n); streflop_init<Extended>(); return Extended(res);}



	inline Extended fabs(Extended x) {return streflop_libm::__fabsl(x);}
	inline Extended floor(Extended x) {return streflop_libm::__floorl(x);}
	inline Extended ceil(Extended x) {return streflop_libm::__ceill(x);}
	inline Extended trunc(Extended x) {return streflop_libm::__truncl(x);}
	inline Extended fmod(Extended x, Extended y) {return streflop_libm::__ieee754_fmodl(x,y);}
	inline Extended remainder(Extended x, Extended y) {return streflop_libm::__ieee754_remainderl(x,y);}
	inline Extended remquo(Extended x, Extended y, int *quo) {return streflop_libm::__remquol(x,y,quo);}
	inline Extended rint(Extended x) {return streflop_libm::__rintl(x);}
	inline long int lrint(Extended x) {return streflop_libm::__lrintl(x);}
	inline long long int llrint(Extended x) {return streflop_libm::__llrintl(x);}
	inline Extended round(Extended x) {return streflop_libm::__roundl(x);}
	inline long int lround(Extended x) {return streflop_libm::__lroundl(x);}
	inline long long int llround(Extended x) {return streflop_libm::__llroundl(x);}
	inline Extended nearbyint(Extended x) {return streflop_libm::__nearbyintl(x);}

	inline Extended frexp(Extended x, int *exp) {return streflop_libm::__frexpl(x,exp);}
	inline Extended ldexp(Extended value, int exp) {return streflop_libm::__ldexpl(value,exp);}
	inline Extended logb(Extended x) {return streflop_libm::__logbl(x);}
	inline int ilogb(Extended x) {return streflop_libm::__ilogbl(x);}
	inline Extended copysign(Extended x) {return streflop_libm::__copysignl(x);}
	inline int signbit (Extended x) {return streflop_libm::__signbitl(x);}
	inline Extended nextafter(Extended x, Extended y) {return streflop_libm::__nextafterl(x,y);}

	inline int fpclassify(Extended x) {return streflop_libm::__fpclassifyl(x);}
	inline int isnan(Extended x) {return streflop_libm::__isnanl(x);}
	inline int isinf(Extended x) {return streflop_libm::__isinfl(x);}
	inline int isfinite(Extended x) {return !(isnan(x) || isinf(x));}

	// Stolen from math.h and inlined instead of macroized.
	// Return nonzero value if X is neither zero, subnormal, Inf, nor NaN.  */
	inline int isnormal(Extended x) {return fpclassify(x) == STREFLOP_FP_NORMAL;}

	// Constants user may set
	extern const Extended ExtendedPositiveInfinity;
	extern const Extended ExtendedNegativeInfinity;
	// Non-signaling NaN used for returning such results
	// Standard lets a large room for implementing different kinds of NaN
	// These NaN can be used for custom purposes
	// Threated as an integer, the bit pattern here may be incremented to get at least 2^20 different NaN custom numbers!
	// Note that when switching the left-most bit to 1, you can get another bunch of negative NaNs, whatever this mean.
	extern const Extended ExtendedNaN;

	/** Generic C99 "macros" for unordered comparison
		Defined as inlined for each type, thanks to C++ overloading
	*/
	inline bool isunordered(Extended x, Extended y) {
		return (fpclassify(x) == STREFLOP_FP_NAN) || (fpclassify (y) == STREFLOP_FP_NAN);
	}
	inline bool isgreater(Extended x, Extended y) {
		return (!isunordered(x,y)) && (x > y);
	}
	inline bool isgreaterequal(Extended x, Extended y) {
		return (!isunordered(x,y)) && (x >= y);
	}
	inline bool isless(Extended x, Extended y) {
		return (!isunordered(x,y)) && (x < y);
	}
	inline bool islessequal(Extended x, Extended y) {
		return (!isunordered(x,y)) && (x <= y);
	}
	inline bool islessgreater(Extended x, Extended y) {
		return (!isunordered(x,y)) && ((x < y) || (x > y));
	}


// Add xxxl alias to ease porting existing code to streflop
// Additionally, using xxxl(number) avoids potential confusion

	inline Extended sqrtl(Extended x) {return sqrt(x);}
	inline Extended cbrtl(Extended x) {return cbrt(x);}
	inline Extended hypotl(Extended x, Extended y) {return hypot(x, y);}

	inline Extended expl(Extended x) {return exp(x);}
	inline Extended logl(Extended x) {return log(x);}
	inline Extended log2l(Extended x) {return log2(x);}
	inline Extended exp2l(Extended x) {return exp2(x);}
	inline Extended log10l(Extended x) {return log10(x);}
	inline Extended powl(Extended x, Extended y) {return pow(x, y);}

	inline Extended sinl(Extended x) {return sin(x);}
	inline Extended cosl(Extended x) {return cos(x);}
	inline Extended tanl(Extended x) {return tan(x);}
	inline Extended acosl(Extended x) {return acos(x);}
	inline Extended asinl(Extended x) {return asin(x);}
	inline Extended atanl(Extended x) {return atan(x);}
	inline Extended atan2l(Extended x, Extended y) {return atan2(x, y);}

	inline Extended coshl(Extended x) {return cosh(x);}
	inline Extended sinhl(Extended x) {return sinh(x);}
	inline Extended tanhl(Extended x) {return tanh(x);}
	inline Extended acoshl(Extended x) {return acosh(x);}
	inline Extended asinhl(Extended x) {return asinh(x);}
	inline Extended atanhl(Extended x) {return atanh(x);}

	inline Extended fabsl(Extended x) {return fabs(x);}
	inline Extended floorl(Extended x) {return floor(x);}
	inline Extended ceill(Extended x) {return ceil(x);}
	inline Extended truncl(Extended x) {return trunc(x);}
	inline Extended fmodl(Extended x, Extended y) {return fmod(x,y);}
	inline Extended remainderl(Extended x, Extended y) {return remainder(x,y);}
	inline Extended remquol(Extended x, Extended y, int *quo) {return remquo(x, y, quo);}
	inline Extended rintl(Extended x) {return rint(x);}
	inline long int lrintl(Extended x) {return lrint(x);}
	inline long long int llrintl(Extended x) {return llrint(x);}
	inline Extended roundl(Extended x) {return round(x);}
	inline long int lroundl(Extended x) {return lround(x);}
	inline long long int llroundl(Extended x) {return llround(x);}
	inline Extended nearbyintl(Extended x) {return nearbyint(x);}

	inline Extended frexpl(Extended x, int *exp) {return frexp(x, exp);}
	inline Extended ldexpl(Extended value, int exp) {return ldexp(value,exp);}
	inline Extended logbl(Extended x) {return logb(x);}
	inline int ilogbl(Extended x) {return ilogb(x);}
	inline Extended copysignl(Extended x) {return copysign(x);}
	inline int signbitl(Extended x) {return signbit(x);}
	inline Extended nextafterl(Extended x, Extended y) {return nextafter(x, y);}

	inline Extended expm1l(Extended x) {return expm1(x);}
	inline Extended log1pl(Extended x) {return log1p(x);}
	inline Extended erfl(Extended x) {return erf(x);}
	inline Extended j0l(Extended x) {return j0(x);}
	inline Extended j1l(Extended x) {return j1(x);}
	inline Extended jnl(int n, Extended x) {return jn(n, x);}
	inline Extended y0l(Extended x) {return y0(x);}
	inline Extended y1l(Extended x) {return y1(x);}
	inline Extended ynl(int n, Extended x) {return yn(n, x);}
	inline Extended scalbnl(Extended x, int n) {return scalbn(x, n);}
	inline Extended scalblnl(Extended x, long int n) {return scalbln(x, n);}

	inline int fpclassifyl(Extended x) {return fpclassify(x);}
	inline int isnanl(Extended x) {return isnan(x);}
	inline int isinfl(Extended x) {return isinf(x);}
	inline int isfinitel(Extended x) {return isfinite(x);}
	inline int isnormall(Extended x) {return isnormal(x);}

	inline bool isunorderedl(Extended x, Extended y) {return isunordered(x, y);}
	inline bool isgreaterl(Extended x, Extended y) {return isgreater(x, y);}
	inline bool isgreaterequall(Extended x, Extended y) {return isgreaterequal(x, y);}
	inline bool islessl(Extended x, Extended y) {return isless(x, y);}
	inline bool islessequall(Extended x, Extended y) {return islessequal(x, y);}
	inline bool islessgreaterl(Extended x, Extended y) {return islessgreater(x, y);}


#endif // Extended

} // namespace streflop

#endif // STREFLOP_MATH_H
