/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FILE_QUERY_FLAGS_H
#define FILE_QUERY_FLAGS_H

namespace FileQueryFlags { // FIXME not good, as we already have a class with that name

// flags for FindFiles
enum FindFilesBits {
	RECURSE      = (1 << 0),
	INCLUDE_DIRS = (1 << 1),
	ONLY_DIRS    = (1 << 2),
};

// flags for LocateFile
enum LocateFileBits {
	WRITE       = (1 << 0),
	CREATE_DIRS = (1 << 1),
};

}

#endif // !FILE_QUERY_FLAGS_H
