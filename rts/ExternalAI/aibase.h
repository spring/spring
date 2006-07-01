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
#ifdef _WIN32
	#define DLL_EXPORT extern "C" __declspec(dllexport)
	#define SPRING_EXPORT __declspec(dllexport)
	#define SPRING_IMPORT __declspec(dllimport)
	#define SPRING_LOCAL
#else
	#if __GNUC__ >= 4 && !defined(SYNCDEBUG)
		#define DLL_EXPORT extern "C" __attribute__ ((visibility("default")))
		#define SPRING_EXPORT __attribute__ ((visibility("default")))
		#define SPRING_IMPORT __attribute__ ((visibility("default")))
		#define SPRING_LOCAL  __attribute__ ((visibility("hidden")))
	#else
		#define DLL_EXPORT
		#define SPRING_EXPORT
		#define SPRING_IMPORT
		#define SPRING_LOCAL
	#endif
#endif

#ifdef BUILDING_SPRING
	#define SPRING_API SPRING_EXPORT
#else
	#define SPRING_API SPRING_IMPORT
#endif

#endif /* AIBASE_H */
