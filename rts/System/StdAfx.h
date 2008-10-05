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


#if defined(_MSC_VER) || defined(USE_PRECOMPILED_HEADER)
// top included files without lots of dependencies
// also, they shouldn't get in the way of mmgr
#include "float3.h"
#include "GlobalStuff.h"
#include "System/Util.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Game/Camera.h"
#include "LogOutput.h"
#include "Rendering/GL/myGL.h"
#endif // USE_PRECOMPILED_HEADER


#endif // __STD_AFX_H__
