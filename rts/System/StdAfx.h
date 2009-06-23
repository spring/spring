#ifndef __STD_AFX_H__
#define __STD_AFX_H__

#ifdef	__cplusplus

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
//
// USE_MMGR needs to include a lot of the standard library because it
// re#defines new which breaks STL
#if defined(_MSC_VER) || defined(USE_PRECOMPILED_HEADER) || defined(USE_MMGR)
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
#include "lib/streflop/streflop_cond.h"
#endif // defined(_MSC_VER) || defined(USE_PRECOMPILED_HEADER) || defined(USE_MMGR)


#if !defined(USE_GML) && (defined(_MSC_VER) || defined(USE_PRECOMPILED_HEADER))
// top included files without lots of dependencies
// also, they shouldn't get in the way of mmgr
#if !defined BUILDING_AI
#include "Rendering/GL/myGL.h"
#endif // !defined BUILDING_AI
#include "float3.h"
#include "Util.h"
#include "GlobalUnsynced.h"
#if !defined BUILDING_AI
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Game/Camera.h"
#endif // !defined BUILDING_AI
#endif // !defined(USE_GML) && (defined(_MSC_VER) || defined(USE_PRECOMPILED_HEADER))

#endif // __cplusplus

#endif // __STD_AFX_H__
