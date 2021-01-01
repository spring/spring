/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FILE_HANDLER_H
#define _FILE_HANDLER_H

#include <vector>
#include <string>
#include <fstream>
#include <cinttypes>

#include "VFSModes.h"

/**
 * This is for direct VFS file content access.
 * If you need data-dir related file and dir handling methods,
 * have a look at the FileSystem class.
 *
 * This class should be threadsafe (multiple threads can use multiple
 * CFileHandler pointing to the same file simulatneously) as long as there are
 * no new Archives added to the VFS (which should not happen after PreGame).
 */
class CFileHandler
{
public:
	CFileHandler(const char* fileName, const char* modes = SPRING_VFS_RAW_FIRST);
	CFileHandler(const std::string& fileName, const std::string& modes = SPRING_VFS_RAW_FIRST);
	virtual ~CFileHandler() { Close(); }

	void Open(const std::string& fileName, const std::string& modes = SPRING_VFS_RAW_FIRST);
	void Close();

	int Read(void* buf, int length);
	int ReadString(void* buf, int length); //< stops after the first 0 char
	void Seek(int pos, std::ios_base::seekdir where = std::ios_base::beg);

	static bool FileExists(const std::string& filePath, const std::string& modes);
	// true if any of TryReadFrom{RawFS,PWD,VFS} succeed
	bool FileExists() const { return (fileSize >= 0); }
	// true if (and only if) TryReadFromVFS succeeds
	bool IsBuffered() const { return (!fileBuffer.empty()); }

	bool Eof() const;
	int GetPos();
	int FileSize() const { return fileSize; }
	int LoadCode() const { return loadCode; }

	bool LoadStringData(std::string& data);
	std::string GetFileExt() const;
	static std::string GetFileAbsolutePath(const std::string& filePath, const std::string& modes);
	static std::string GetArchiveContainingFile(const std::string& filePath, const std::string& modes);

	std::vector<std::uint8_t>& GetBuffer() { return fileBuffer; }

	static bool InReadDir(const std::string& path);
	static bool InWriteDir(const std::string& path);

	static std::vector<std::string> FindFiles(const std::string& path, const std::string& pattern);
	static std::vector<std::string> DirList(const std::string& path, const std::string& pattern, const std::string& modes);
	static std::vector<std::string> SubDirs(const std::string& path, const std::string& pattern, const std::string& modes);

	static std::string AllowModes(const std::string& modes, const std::string& allowed);
	static std::string ForbidModes(const std::string& modes, const std::string& forbidden);


protected:
	CFileHandler() { Close(); } // for CGZFileHandler

	virtual bool TryReadFromPWD(const std::string& fileName);
	virtual bool TryReadFromRawFS(const std::string& fileName);
	virtual bool TryReadFromVFS(const std::string& fileName, int section);

	static bool InsertRawFiles(std::vector<std::string>& fileSet, const std::string& path, const std::string& pattern);
	static bool InsertVFSFiles(std::vector<std::string>& fileSet, const std::string& path, const std::string& pattern, int section);

	static bool InsertRawDirs(std::vector<std::string>& dirSet, const std::string& path, const std::string& pattern);
	static bool InsertVFSDirs(std::vector<std::string>& dirSet, const std::string& path, const std::string& pattern, int section);

	std::string fileName;
	std::ifstream ifs;
	std::vector<std::uint8_t> fileBuffer;

	int filePos = 0;
	int fileSize = -1;
	int loadCode = -3; // {-1,0,1} if loaded from VFS
};

#endif // _FILE_HANDLER_H
