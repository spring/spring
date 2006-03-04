#ifndef WINDOWS_H_INCLUDED
#define WINDOWS_H_INCLUDED

#ifdef _WIN32
#include <windows.h>

#undef PlaySound

// std min&max are used instead of the macros
#ifdef min
#undef min
#undef max
#endif
#endif

#endif
