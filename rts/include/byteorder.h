/*
 * byteorder.h
 * Handling of endian byte orders
 * Copyright (C) 2005 Christopher Han
 */
#ifndef BYTEORDER_H
#define BYTEORDER_H

/*
 * The swabbing stuff looks backwards, but the files
 * are _originally_ little endian (win32 x86).
 * So in this case little endian is the standard while
 * big endian is the exception.
 */

#if defined(__linux__)
#include <byteswap.h>

#if __BYTE_ORDER == __BIG_ENDIAN
#define swabword(w)	(bswap_16(w))
#define swabdword(w)	(bswap_32(w))
#else
#define swabword(w)	(w)
#define swabdword(w)	(w)
#endif

#elif defined(__FreeBSD__)

#include <sys/endian.h>

#define swabword(w)	(htole16(w))
#define swabdword(w)	(htole32(w))

#endif

#endif
