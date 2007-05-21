#ifndef __STD_AFX_H__
#define __STD_AFX_H__

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

#ifdef _MSC_VER
	#pragma warning (disable: 4018 4244 4305 4530)
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
		#define VSNPRINTF _vsnprintf
	#endif
	#define STRCASECMP stricmp
#else
	// assuming GCC
	#define SNPRINTF snprintf
	#define VSNPRINTF vsnprintf
	#define STRCASECMP strcasecmp
#endif


#ifdef _MSC_VER
	#define SPRING_HASH_SET stdext::hash_set
	#define SPRING_HASH_SET_H <hash_set>

	#define SPRING_HASH_MAP stdext::hash_map
	#define SPRING_HASH_MAP_H <hash_map>
#elif __GNUG__
	#define SPRING_HASH_SET __gnu_cxx::hash_set
	#define SPRING_HASH_SET_H <ext/hash_set>

	#define SPRING_HASH_MAP __gnu_cxx::hash_map
	#define SPRING_HASH_MAP_H <ext/hash_map>
#else
	#error Unsupported compiler
#endif


/*
 * Compile time assertion
 */
#define COMPILE_TIME_ASSERT(condition, message) \
	typedef int _compile_time_assertion_failed__ ## message [(condition) ? 1 : -1]


// This reduces compile-time with precompiled headers on msvc
// It increases compile-time with precompiled headers on gcc
#ifdef _MSC_VER
#include <vector>
#include <map>
#include <set>
#endif

#include "Sync/Syncify.h"
#include "creg/creg.h"
#include "float3.h"
#include "Sync/SyncedFloat3.h"
#include "GlobalStuff.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <string>
#include <ctype.h>

/**
 * content_error
 * thrown when content couldn't be found/loaded.
 * any other type of exception will cause a crashreport box appearing (if it is installed).
 */
class content_error : public std::runtime_error
{
public:
	content_error(const std::string& msg) :
	  std::runtime_error(msg) {}
};

static inline void
StringToLowerInPlace(std::string &s)
{
	std::transform (s.begin(), s.end(), s.begin(), (int (*)(int))tolower);
}

static inline std::string
StringToLower(std::string s)
{
	StringToLowerInPlace(s);
	return s;
}

#endif // __STD_AFX_H__
