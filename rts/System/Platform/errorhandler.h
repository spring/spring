/**
 * @file errorhandler.h
 * @brief error messages
 * @author Christopher Han <xiphux@gmail.com>
 * @author Tobi Vollebregt <tobivollebregt@gmail.com>
 * @author Robin Vobruba <hoijui.quaero@gmail.com>
 *
 * Error handling based on platform
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */
#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#define MBF_OK		1
#define MBF_INFO	2
#define MBF_EXCL	4

#if defined __GNUC__ && (__GNUC__ >= 4)
	#define NO_RETURN __attribute__ ((noreturn))
#else
	#define NO_RETURN
#endif

#define handleerror(o, m, c, f) ErrorMessageBox(m, c, f)
void ErrorMessageBox(const char *msg, const char *caption, unsigned int flags) NO_RETURN;

#endif
