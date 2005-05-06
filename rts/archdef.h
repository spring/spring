// archdef.h: declare platform/compiler specific macros that'll be used
//            throughout the code
//
///////////////////////////////////////////////////////////////////////////

#ifndef __ARCHDEF_H__
#define __ARCHDEF_H__

//
// Attempt to detect the current compiler based on macros they export
//
#ifndef linux
#if defined(_MSC_VER) // Visual C++
    #define ARCHDEF_COMPILER_MSVC
    // Export various stuff for VC++
    #ifndef VC_EXTRALEAN
    #define VC_EXTRALEAN    // Exclude rarely-used stuff from Windows headers
    #endif
    #pragma warning (disable:4530)
    #pragma warning (disable:4786)
#elsif defined(__MINGW32__) // MingW (Dev-C++, Code::Blocks)
    #define ARCHDEF_COMPILER_MINGW32
#elsif defined (__GNUG__) // Gnu C++ compiler
    #define ARCHDEF_COMPILER_GCC
#else
    #error Unknown compiler used!
#endif

//
// Deduce the platform from the compiler
//
#if defined(ARCHDEF_COMPILER_MSVC) || defined(ARCHDEF_COMPILER_MINGW32)
    #define ARCHDEF_PLATFORM_WIN32
    #define WIN32_LEAN_AND_MEAN 1
#else // We assume plain GCC means linux for now
    #define ARCHDEF_PLATFORM_LINUX
#endif
#else //linux
#define ARCHDEF_COMPILER_GCC
#define ARCHDEF_PLATFORM_LINUX
#endif

#endif // __ARCHDEF_H__
