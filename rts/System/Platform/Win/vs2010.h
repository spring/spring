#if defined(_MSC_VER) && _MSC_VER >= 1600
// FIXME ugly hacks for MSVC 2010 due to the _CMATH_ preprocessor flag workaround
// Unsigned int is intentional to avoid ambiguous call
extern float ldexpf(float x, unsigned int y);
extern float ldexp(float x, unsigned int y);
extern float ldexpl(float x, unsigned int y);
extern float frexp(float x, int *y);
#endif