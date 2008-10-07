#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _MSC_VER
	/*
	Microsoft Visual C++ 7.1: MSC_VER = 1310
	Microsoft Visual C++ 7.0: MSC_VER = 1300
	*/
	#if _MSC_VER > 1310 // >= Visual Studio 2005
		#define SNPRINTF sprintf_s
		#define VSNPRINTF vsprintf_s
		#define STRCPY strcpy_s
		#define FOPEN fopen_s
	#else              // Visual Studio 2003
		#define SNPRINTF _snprintf
		#define VSNPRINTF _vsnprintf
		#define STRCPY strcpy
		#define FOPEN fopen
	#endif
	#define STRCASECMP stricmp
#else
	// assuming GCC
	#define SNPRINTF snprintf
	#define VSNPRINTF vsnprintf
	#define STRCPY strcpy
	#define FOPEN fopen
	#define STRCASECMP strcasecmp
#endif

static inline char* mallocCopyString(const char* const orig) {
	
	char* copy;
	
	copy = (char *) malloc(/*sizeof(char) * */strlen(orig) + 1);
	strcpy(copy, orig);
	
	return copy;
}
static inline void freeString(const char* const toFreeStr) {
	free(const_cast<char*>(toFreeStr));
}


#ifdef	__cplusplus

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


static inline std::string IntToString(int i, const std::string& format = "%i")
{
	char buf[64];
	SNPRINTF(buf, sizeof(buf), format.c_str(), i);
	return std::string(buf);
}

#endif	/* __cplusplus */

#endif
