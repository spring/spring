/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ARCHIVE_TYPES_H
#define _ARCHIVE_TYPES_H

enum {
	ARCHIVE_TYPE_SDP = 0, // pool
	ARCHIVE_TYPE_GIT = 1, // git repo
	ARCHIVE_TYPE_SDD = 2, // dir
	ARCHIVE_TYPE_SDZ = 3, // zip
	ARCHIVE_TYPE_SD7 = 4, // 7zip
	ARCHIVE_TYPE_SDV = 5, // virtual
	ARCHIVE_TYPE_CNT = 6,
	ARCHIVE_TYPE_BUF = 7, // buffered, not created directly
};

#endif

