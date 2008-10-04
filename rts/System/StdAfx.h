#ifndef __STD_AFX_H__
#define __STD_AFX_H__

#ifndef NOMINMAX
#define NOMINMAX // avoid conflicts with std::min and std::max
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif


// This reduces compile-time with precompiled headers on msvc
// It used to increase compile-time with precompiled headers on gcc
#if defined(_MSC_VER) || defined(USE_PRECOMPILED_HEADER)
#include <vector>
#include <map>
#include <set>
#include <string>
#endif

// maybe we should remove syncify altogether?
#include "Sync/Syncify.h"

#endif // __STD_AFX_H__
