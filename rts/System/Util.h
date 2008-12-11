#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _MSC_VER
	// Microsoft Visual C++ 7.1: MSC_VER = 1310
	// Microsoft Visual C++ 7.0: MSC_VER = 1300
	#if _MSC_VER > 1310 // >= Visual Studio 2005
		#define SNPRINTF sprintf_s
		#define VSNPRINTF vsprintf_s
		#define STRCPY strcpy_s
		#define FOPEN fopen_s
	#else               // Visual Studio 2003
		#define SNPRINTF _snprintf
		#define VSNPRINTF _vsnprintf
		#define STRCPY strcpy
		#define FOPEN fopen
	#endif
	#define STRCASECMP stricmp
#else // defined _MSC_
	// assuming GCC
	#define SNPRINTF snprintf
	#define VSNPRINTF vsnprintf
	#define STRCPY strcpy
	#define FOPEN fopen
	#define STRCASECMP strcasecmp
#endif // defined _MSC_VER

static inline char* mallocCopyString(const char* const orig) {

	char* copy;

	copy = (char *) malloc(/*sizeof(char) * */strlen(orig) + 1);
	strcpy(copy, orig);

	return copy;
}
static inline void freeString(const char* const toFreeStr) {
	free(const_cast<char*>(toFreeStr));
}


#ifdef __cplusplus

#include <string>
#include <algorithm>

static inline void StringToLowerInPlace(std::string &s)
{
	std::transform(s.begin(), s.end(), s.begin(), (int (*)(int))tolower);
}


static inline std::string StringToLower(std::string s)
{
	StringToLowerInPlace(s);
	return s;
}

static inline std::string GetFileExt(const std::string& s)
{
	size_t i = s.rfind('.', s.length());
	if (i != std::string::npos) {
		return s.substr(i+1, s.length() - i);
	}
	return "";
}

static inline std::string IntToString(int i, const std::string& format = "%i")
{
	char buf[64];
	SNPRINTF(buf, sizeof(buf), format.c_str(), i);
	return std::string(buf);
}

/**
 * @brief Safely delete object by first setting pointer to NULL and then deleting.
 * This way it is guaranteed other objects can not access the object through the
 * pointer while the object is running it's destructor.
 */
template<class T> void SafeDelete(T &a)
{
	T tmp = a;
	a = NULL;
	delete tmp;
}

#endif // __cplusplus

#endif
