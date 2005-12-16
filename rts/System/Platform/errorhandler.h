/*
 * errorhandler.h
 * Error handling based on platform
 * Copyright (C) 2005 Christopher Han
 */
#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#define MBF_OK		1
#define MBF_INFO	2
#define MBF_EXCL	4

#ifdef _WIN32

#define handleerror(o, m, c, f) ErrorMessageBox(m, c, f)
void ErrorMessageBox(const char *msg, const char *caption, unsigned int flags);

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

#define handleerror(o, m, c, f)					\
do {								\
	if (f & MBF_INFO)				\
	    std::cerr << "Info: ";				\
	else if (f & MBF_EXCL)			\
	    std::cerr << "Warning: ";				\
	else							\
	    std::cerr << "ERROR: ";				\
	std::cerr << c << std::endl;				\
	std::cerr << "  " << m << std::endl;			\
	if (!(f&(MBF_INFO|MBF_EXCL)))	\
		do {DEBUGSTRING} while (0);			\
} while (0)

#endif

#endif
