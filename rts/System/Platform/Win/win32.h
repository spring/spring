#ifndef WINDOWS_H_INCLUDED
#define WINDOWS_H_INCLUDED

	#ifdef _WIN32
  #ifndef NOMINMAX
  #define NOMINMAX
  #endif
	#include <windows.h>

		#undef  PlaySound
		#define PlaySound  use_PlaySample_instead_of_PlaySound

		#undef CreateDirectory

		// std min&max are used instead of the macros
		#ifdef min
			#undef min
			#undef max
		#endif

	#endif // _WIN32

#endif // WINDOWS_H_INCLUDED
