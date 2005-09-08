/*
 * errorhandler.h
 * Error handling based on platform
 * Copyright (C) 2005 Christopher Han
 */
#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#ifdef _WIN32

#include <windows.h>
#define handleerror(o, m, c, f) MessageBox(o, m, c, f)

#else

#include <iostream>

#ifdef DEBUG
#ifdef __GNUC__
#define DEBUGSTRING std::cerr << "  " << __FILE__ << ":" << std::dec << __LINE__ << " : " << __PRETTY_FUNCTION__ << std::endl;
#else
#define DEBUGSTRING std::cerr << "  " << __FILE__ << ":" << std::dec << __LINE__ << std::endl;
#endif
#else
#define DEBUGSTRING
#endif

#define MB_OK 			0x00000001
#define MB_ICONINFORMATION	0x00000002
#define MB_ICONEXCLAMATION	0x00000004

#define handleerror(o, m, c, f)					\
do {								\
	if (f & MB_ICONINFORMATION)				\
	    std::cerr << "Info: ";				\
	else if (f & MB_ICONEXCLAMATION)			\
	    std::cerr << "Warning: ";				\
	else							\
	    std::cerr << "ERROR: ";				\
	std::cerr << c << std::endl;				\
	std::cerr << "  " << m << std::endl;			\
	if (!(f&(MB_ICONINFORMATION|MB_ICONEXCLAMATION)))	\
		do {DEBUGSTRING} while (0);			\
} while (0)

#endif

#endif
