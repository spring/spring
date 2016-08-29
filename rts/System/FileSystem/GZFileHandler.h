/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GZ_FILE_HANDLER_H
#define _GZ_FILE_HANDLER_H

#include "FileHandler.h"

#include <string>

#include "VFSModes.h"
/**
 * Uncompresses the entire file to memory, so don't use with huge files.
 */
class CGZFileHandler : public CFileHandler
{
public:
	CGZFileHandler(const char* fileName, const char* modes = SPRING_VFS_RAW_FIRST);
	CGZFileHandler(const std::string& fileName, const std::string& modes = SPRING_VFS_RAW_FIRST);

private:
	bool TryReadFromPWD(const std::string& fileName) override;
	bool TryReadFromRawFS(const std::string& fileName) override;
	bool TryReadFromVFS(const std::string& fileName, int section) override;
	bool ReadToBuffer(const std::string& path);
	bool UncompressBuffer();
};

#endif // _GZ_FILE_HANDLER_H
