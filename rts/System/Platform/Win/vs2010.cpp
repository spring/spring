#if defined(_MSC_VER) && _MSC_VER >= 1600
#include "lib/streflop/streflop_cond.h"
// FIXME ugly hacks for MSVC 2010 due to the _CMATH_ preprocessor flag workaround
// Unsigned int is intentional to avoid ambiguous call
#undef ldexp
#undef ldexpl
float ldexpf(float x, unsigned int y) { return streflop::ldexpf(x, (int)y); }
float ldexp(float x, unsigned int y) { return streflop::ldexpf(x, (int)y); }
float ldexpl(float x, unsigned int y) { return streflop::ldexpf(x, (int)y); }
float frexp(float x, int *y) { return streflop::frexp(x, y); }
#endif