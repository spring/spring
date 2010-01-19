/* Copyright (C) 2009 Tobi Vollebregt */

#include "streflopC.h"

#include "streflop_cond.h"

#ifdef __cplusplus
extern "C" {
#endif

void streflop_init_Simple() {
	streflop_init<Simple>();
}

void streflop_init_Double() {
	streflop_init<Double>();
}

#if defined(Extended)
void streflop_init_Extended() {
	streflop_init<Extended>();
}
#endif // defined(Extended)

#ifdef __cplusplus
} // extern "C"
#endif
