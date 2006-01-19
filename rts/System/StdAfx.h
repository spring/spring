#ifndef __STD_AFX_H__
#define __STD_AFX_H__

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

#pragma warning (disable:4530)

#ifdef _MSC_VER
/*
Microsoft Visual C++ 7.1: MSC_VER = 1310
Microsoft Visual C++ 7.0: MSC_VER = 1300
*/
	#if _MSC_VER > 1310 // >= Visual Studio 2005
		#pragma warning(disable: 4996) // hide warnings about deprecated functions
		#define SNPRINTF sprintf_s 
		#define VSNPRINTF vsprintf_s
	#else              // Visual Studio 2003
		#define SNPRINTF _snprintf
		#define SNPRINTF _vsnprintf
	#endif
#else
	// assuming GCC 
	#define SNPRINTF snprintf
	#define VSNPRINTF vsnprintf
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
