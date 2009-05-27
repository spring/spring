#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "maindefines.h"

/*
static inline char* mallocCopyString(const char* const orig)
{
	char* copy;
	copy = (char *) malloc(sizeof(char) * strlen(orig) + 1);
	strcpy(copy, orig);
	return copy;
}
static inline void freeString(const char* const toFreeStr)
{
	free(const_cast<char*>(toFreeStr));
}
*/



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
	size_t l = s.length();
#ifdef WIN32
	//! windows eats dots and spaces at the end of filenames
	while (l > 0) {
		if (s[l-1]=='.') {
			l--;
		} else if (s[l-1]==' ') {
			l--;
		} else break;
	}
#endif
	size_t i = s.rfind('.', l);
	if (i != std::string::npos) {
		return StringToLower(s.substr(i+1, l - i));
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



namespace proc {
	#if defined(__GNUC__)
	// function inlining breaks the stack layout this assumes
	__attribute__((__noinline__)) static void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d)
	{
		unsigned int t = 0;
		__asm__ __volatile__ (
			" mov %5, %%eax;"
			" mov %6, %%ecx;"
			" mov %%ebx, %0;"
			" cpuid;"
			" mov %%eax, %1;"
			" mov %%ebx, %2;"
			" mov %%ecx, %3;"
			" mov %%edx, %4;"
			" mov %7, %%ebx"
			: "=r" (t), "=a" (*a), "=r" (*b), "=c" (*c), "=d" (*d)
			: "a" (*a), "c" (*c), "r" (t)
		);
	}
	#else
	// no-op on other compilers
	static void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d)
	{
	}
	#endif

	static unsigned int GetProcMaxStandardLevel()
	{
		unsigned int rEAX = 0x00000000;
		unsigned int rEBX =          0;
		unsigned int rECX =          0;
		unsigned int rEDX =          0;

		ExecCPUID(&rEAX, &rEBX, &rECX, &rEDX);

		return rEAX;
	}
	static unsigned int GetProcMaxExtendedLevel()
	{
		unsigned int rEAX = 0x80000000;
		unsigned int rEBX =          0;
		unsigned int rECX =          0;
		unsigned int rEDX =          0;

		ExecCPUID(&rEAX, &rEBX, &rECX, &rEDX);

		return rEAX;
	}

	static inline unsigned int GetProcSSEBits()
	{
		unsigned int rEAX = 0;
		unsigned int rEBX = 0;
		unsigned int rECX = 0;
		unsigned int rEDX = 0;
		unsigned int bits = 0;

		if (GetProcMaxStandardLevel() >= 0x00000001U) {
			rEAX = 0x00000001U; ExecCPUID(&rEAX, &rEBX, &rECX, &rEDX);

			int SSE42  = (rECX >> 20) & 1; bits |= ( SSE42 << 0); // SSE 4.2
			int SSE41  = (rECX >> 19) & 1; bits |= ( SSE41 << 1); // SSE 4.1
			int SSSE30 = (rECX >>  9) & 1; bits |= (SSSE30 << 2); // Supplemental SSE 3.0
			int SSE30  = (rECX >>  0) & 1; bits |= ( SSE30 << 3); // SSE 3.0

			int SSE20  = (rEDX >> 26) & 1; bits |= ( SSE20 << 4); // SSE 2.0
			int SSE10  = (rEDX >> 25) & 1; bits |= ( SSE10 << 5); // SSE 1.0
			int MMX    = (rEDX >> 23) & 1; bits |= ( MMX   << 6); // MMX
		}

		if (GetProcMaxExtendedLevel() >= 0x80000001U) {
			rEAX = 0x80000001U; ExecCPUID(&rEAX, &rEBX, &rECX, &rEDX);

			int SSE50A = (rECX >> 11) & 1; bits |= (SSE50A << 7); // SSE 5.0A
			int SSE40A = (rECX >>  6) & 1; bits |= (SSE40A << 8); // SSE 4.0A
			int MSSE   = (rECX >>  7) & 1; bits |= (MSSE   << 9); // Misaligned SSE
		}

		return bits;
	}
}

#endif // __cplusplus

#endif // UTIL_H
