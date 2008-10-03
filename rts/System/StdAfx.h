#ifndef __STD_AFX_H__
#define __STD_AFX_H__

#ifndef NOMINMAX
#define NOMINMAX // avoid conflicts with std::min and std::max
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

#ifdef _MSC_VER
/*
Microsoft Visual C++ 7.1: MSC_VER = 1310
Microsoft Visual C++ 7.0: MSC_VER = 1300
*/
	#if _MSC_VER > 1310 // >= Visual Studio 2005
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


/** @brief Compile time assertion
    @param condition Condition to test for.
    @param message Message to include in the compile error if the assert fails.
    This must be a valid C++ symbol. */
#define COMPILE_TIME_ASSERT(condition, message) \
	typedef int _compile_time_assertion_failed__ ## message [(condition) ? 1 : -1]


// This reduces compile-time with precompiled headers on msvc
// It used to increase compile-time with precompiled headers on gcc
#if defined(_MSC_VER) || defined(USE_PRECOMPILED_HEADER)
#include <vector>
#include <map>
#include <set>
#endif

// need this to support boost 1.36+
#ifdef USE_GML
# ifndef BOOST_DETAIL_ATOMIC_COUNT_HPP_INCLUDED
#  define GML_COMPATIBLE_ATOMIC_COUNT
#  define private public
#  include <boost/detail/atomic_count.hpp>
#  undef private
# endif
#endif

#include <algorithm>
#include <stdexcept>
#include <string>
#include <cctype>
#include <string>
#include <cstring>
#include <cmath>

#include "Sync/Syncify.h"
#include "creg/creg.h"
#include "FastMath.h"
#include "float3.h"
#include "GlobalStuff.h"


/**
 * content_error
 *   thrown when content couldn't be found/loaded.
 *   any other type of exception will cause a crashreport box appearing
 *     (if it is installed).
 */
class content_error : public std::runtime_error
{
	public:
		content_error(const std::string& msg) : std::runtime_error(msg) {}
};


static inline void StringToLowerInPlace(std::string &s)
{
	std::transform(s.begin(), s.end(), s.begin(), (int (*)(int))tolower);
}


static inline std::string StringToLower(std::string s)
{
	StringToLowerInPlace(s);
	return s;
}


static inline std::string IntToString(int i, const std::string& format = "%i")
{
	char buf[64];
	SNPRINTF(buf, sizeof(buf), format.c_str(), i);
	return std::string(buf);
}


#endif // __STD_AFX_H__
