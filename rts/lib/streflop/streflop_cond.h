/* Copyright (C) 2008 Tobi Vollebregt */

/* Conditionally include streflop depending on STREFLOP_* #defines:
   If one of those is present, #include "streflop.h", otherwise #include <math.h> */

#ifndef STREFLOP_COND_H
#define STREFLOP_COND_H

#if defined(STREFLOP_X87) || defined(STREFLOP_SSE) || defined(STREFLOP_SOFT)
#include "streflop.h"
using namespace streflop;
#else
#include <cmath>
#endif

#endif
