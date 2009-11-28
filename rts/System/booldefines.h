/* Public Domain (PD) 2009 Robin Vobruba <hoijui.quaero@gmail.com> */

#ifndef __H_STDBOOL__
#define __H_STDBOOL__

#ifndef bool
#define bool int
#endif
#ifndef true
#define true 255
#endif
#ifndef false
#define false 0
#endif

#ifndef __bool_true_false_are_defined
#define __bool_true_false_are_defined 1
#endif

#endif // __H_STDBOOL__
