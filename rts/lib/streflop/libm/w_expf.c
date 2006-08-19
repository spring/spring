/* The original libm wrapper may call the double implementation
   and declares double constants.
   This wrapper purely wraps the float version
*/

#include "math_private.h"

float __expf(float x) {
    return __ieee754_expf(x);
}
