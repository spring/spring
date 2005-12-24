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

#ifdef _MSC_VER
#define DLL_EXPORT extern "C" __declspec(dllexport)
#else
#define DLL_EXPORT extern "C" __attribute__ ((visibility("default")))
#endif

#endif /* AIBASE_H */
