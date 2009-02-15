/* Copyright (C) 2009 Tobi Vollebregt */

/*
	Serves as a C compatible interface to the most basic streflop functions.
*/

#ifndef STREFLOP_C_H
#define STREFLOP_C_H

#ifdef __cplusplus
extern "C" {
#endif

/// Initializes the FPU to single precision
void streflop_init_Simple();

/// Initializes the FPU to double precision
void streflop_init_Double();

#if defined(Extended)
/// Initializes the FPU to extended precision
void streflop_init_Extended();
#endif // defined(Extended)

#ifdef __cplusplus
} // extern "C"
#endif

#endif // STREFLOP_C_H
