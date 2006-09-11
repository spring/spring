#ifndef WINDOWS_H_INCLUDED
#define WINDOWS_H_INCLUDED

#ifdef _WIN32
#include <windows.h>

	#undef PlaySound
	#define PlaySound  Use_PlaySample_instead_of_PlaySound

	// std min&max are used instead of the macros
	#ifdef min
	#undef min
	#undef max
	#endif

	#undef CreateDirectory

#endif // _WIN32

#endif // WINDOWS_H_INCLUDED
