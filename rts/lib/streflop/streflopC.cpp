/* Copyright (C) 2009 Tobi Vollebregt */

#include "streflop_cond.h"

#ifdef __cplusplus
extern "C" {
#endif

void streflop_init_Simple() {
	streflop_init<Simple>();
	#if defined(__SUPPORT_SNAN__)
	feraiseexcept(streflop::FPU_Exceptions(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW));
	#endif
}

void streflop_init_Double() {
	streflop_init<Double>();
	#if defined(__SUPPORT_SNAN__)
	feraiseexcept(streflop::FPU_Exceptions(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW));
	#endif
}

#if defined(Extended)
void streflop_init_Extended() {
	streflop_init<Extended>();
	#if defined(__SUPPORT_SNAN__)
	feraiseexcept(streflop::FPU_Exceptions(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW));
	#endif
}
#endif // defined(Extended)

#ifdef __cplusplus
} // extern "C"
#endif
