/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PFS_TYPES_HDR
#define PFS_TYPES_HDR

enum {
	NOPFS_TYPE  = -1, // for editors
	TKPFS_TYPE  =  0, // default w/ multi-thread request support
	QTPFS_TYPE  =  1,
	HAPFS_TYPE  =  2, // original HPA
	PFS_TYPE_MAX = HAPFS_TYPE,
};

#endif

