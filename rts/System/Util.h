#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <algorithm>
#include <stdio.h>

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

/** @brief Safely delete object by first setting pointer to NULL and then deleting.
	This way it is guaranteed other objects can not access the object through the
	pointer while the object is running it's destructor. */
template<class T> void SafeDelete(T &a)
{
	T tmp = a;
	a = NULL;
	delete tmp;
}

#endif
