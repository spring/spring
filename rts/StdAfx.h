#ifndef __STD_AFX_H__
#define __STD_AFX_H__

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

#pragma warning (disable:4530)

#ifdef _MSC_VER
#define SNPRINTF _snprintf
#else
#define SNPRINTF snprintf
#endif

// This reduces compile-time with precompiled headers on msvc
#ifdef _MSC_VER
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#endif

#include "Syncify.h"
#include "float3.h"
#include "GlobalStuff.h"

#endif // __STD_AFX_H__
