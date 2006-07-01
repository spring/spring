/*
 * aibase.h
 * Base header for ai shared libaries
 * Copyright (C) 2005 Christopher Han
 */
#ifndef AIBASE_H
#define AIBASE_H

#ifdef _WIN32

#include <windows.h>

#else

#define WINAPI

#endif

// Shared library support
#ifdef _MSC_VER
	#define DLL_EXPORT extern "C" __declspec(dllexport)
	#define SPRING_API
#else
	#if __GNUC__ >= 4
		// Support for '-fvisibility=hidden'.
		#define DLL_EXPORT extern "C" __attribute__ ((visibility("default")))
		#define SPRING_API __attribute__ ((visibility("default")))
	#else
		#define DLL_EXPORT extern "C"
		#define SPRING_API
	#endif
#endif

// Virtual destructor support (across DLL/SO interface)
#ifdef __GNUC__
	// MinGW crashes on pure virtual destructors. Bah.
	// GCC on linux works fine, but vtable is emitted at place of the
	// implementation of the virtual destructor, so even if it's pure,
	// it must be implemented.
	// We just combine these 2 facts and don't make dtors pure virtual
	// on GCC in any case.
	#define DECLARE_PURE_VIRTUAL(proto) virtual proto;
	#define IMPLEMENT_PURE_VIRTUAL(proto) proto{}
#else
	// MSVC chokes if we don't declare the destructor pure virtual.
	#define DECLARE_PURE_VIRTUAL(proto) virtual proto = 0;
	#define IMPLEMENT_PURE_VIRTUAL(proto)
#endif

#endif /* AIBASE_H */
