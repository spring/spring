/* Copyright (C) 2009 Tobi Vollebregt */

#include "streflopC.h"

#include "streflop_cond.h"

#ifdef __cplusplus
extern "C" {
#endif

void streflop_init_Simple() {
	streflop::streflop_init<streflop::Simple>();
}

void streflop_init_Double() {
	streflop::streflop_init<streflop::Double>();
}

#if defined(Extended)
void streflop_init_Extended() {
	streflop::streflop_init<streflop::Extended>();
}
#endif // defined(Extended)

#ifdef __cplusplus
} // extern "C"
#endif
