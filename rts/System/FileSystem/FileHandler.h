/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FILE_HANDLER_H
#define _FILE_HANDLER_H

#include <set>
#include <vector>
#include <string>
#include <ios>
#include <boost/cstdint.hpp>

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
	~CFileHandler();

	int Read(void* buf, int length);
	void Seek(int pos, std::ios_base::seekdir where = std::ios_base::beg);

	static bool FileExists(const std::string& filePath, const std::string& modes);
	bool FileExists() const;

	bool Eof() const;
	int Peek() const;
	int GetPos() const;
	int FileSize() const;

	bool LoadStringData(std::string& data);
	std::string GetFileExt() const;

	static bool InReadDir(const std::string& path);
	static bool InWriteDir(const std::string& path);

	static std::vector<std::string> FindFiles(const std::string& path, const std::string& pattern);
	static std::vector<std::string> DirList(const std::string& path, const std::string& pattern, const std::string& modes);
	static std::vector<std::string> SubDirs(const std::string& path, const std::string& pattern, const std::string& modes);

	static std::string AllowModes(const std::string& modes, const std::string& allowed);
	static std::string ForbidModes(const std::string& modes, const std::string& forbidden);

private:
	void TryReadContent(const std::string& fileName, const std::string& modes);

	bool TryReadFromRawFS(const std::string& fileName);
	bool TryReadFromModFS(const std::string& fileName);
	bool TryReadFromMapFS(const std::string& fileName);
	bool TryReadFromBaseFS(const std::string& fileName);

	static bool InsertRawFiles(std::set<std::string>& fileSet, const std::string& path, const std::string& pattern);
	static bool InsertModFiles(std::set<std::string>& fileSet, const std::string& path, const std::string& pattern);
	static bool InsertMapFiles(std::set<std::string>& fileSet, const std::string& path, const std::string& pattern);
	static bool InsertBaseFiles(std::set<std::string>& fileSet, const std::string& path, const std::string& pattern);

	static bool InsertRawDirs(std::set<std::string>& dirSet, const std::string& path, const std::string& pattern);
	static bool InsertModDirs(std::set<std::string>& dirSet, const std::string& path, const std::string& pattern);
	static bool InsertMapDirs(std::set<std::string>& dirSet, const std::string& path, const std::string& pattern);
	static bool InsertBaseDirs(std::set<std::string>& dirSet, const std::string& path, const std::string& pattern);

	std::string fileName;
	std::ifstream* ifs;
	std::vector<boost::uint8_t> fileBuffer;
	int filePos;
	int fileSize;
};

#endif // _FILE_HANDLER_H
