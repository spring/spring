#ifndef __STD_AFX_H__
#define __STD_AFX_H__

#ifndef NOMINMAX
#define NOMINMAX // avoid conflicts with std::min and std::max
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

// on windows, assume win2k or higher
#ifdef _WIN32
# ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x500
# endif
#endif

// This reduces compile-time with precompiled headers on msvc
// It used to increase compile-time with precompiled headers on gcc, it's different now
#if defined(_MSC_VER) || defined(USE_PRECOMPILED_HEADER)
#include <vector>
#include <list>
#include <map>
#include <set>
#include <deque>
#include <string>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <assert.h>

// do not include <cmath> or <math.h> before this, it'll cause ambiguous call er
#include "streflop_cond.h"
#endif

// maybe we should remove syncify altogether?
#include "Sync/Syncify.h"

#endif // __STD_AFX_H__
