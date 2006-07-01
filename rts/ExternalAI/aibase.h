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

// Virtual destructor support
#ifdef _WIN32
	#define IMPLEMENT_PURE_VIRTUAL(proto)
#else
	#define IMPLEMENT_PURE_VIRTUAL(proto) proto {}
#endif

#endif /* AIBASE_H */
