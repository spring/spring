/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __FILE_HANDLER_H__
#define __FILE_HANDLER_H__

#include <set>
#include <vector>
#include <string>
#include <ios>
#include <boost/cstdint.hpp>

#include "VFSModes.h"

/**
 * This is for direct file system access.
 * If you need data-dir related file and dir handling methods,
 * have a look at the FileSystem class.
 * 
 * This class should be threadsafe (multiple threads can use multiple CFileHandler simulatneously)
 * as long as there are no new Archives added to the VFS (should not happen after PreGame).
 */
class CFileHandler
{
public:
	CFileHandler(const char* filename,
					const char* modes = SPRING_VFS_RAW_FIRST);
	CFileHandler(const std::string& filename,
					const std::string& modes = SPRING_VFS_RAW_FIRST);
	~CFileHandler(void);

	int Read(void* buf,int length);
	void Seek(int pos, std::ios_base::seekdir where = std::ios_base::beg);

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
	void Init(const std::string& filename, const std::string& modes);

	bool TryRawFS(const std::string& filename);
	bool TryModFS(const std::string& filename);
	bool TryMapFS(const std::string& filename);
	bool TryBaseFS(const std::string& filename);

	static bool InsertRawFiles(std::set<std::string>& fileSet,
								const std::string& path,
								const std::string& pattern);
	static bool InsertModFiles(std::set<std::string>& fileSet,
								const std::string& path,
								const std::string& pattern);
	static bool InsertMapFiles(std::set<std::string>& fileSet,
								const std::string& path,
								const std::string& pattern);
	static bool InsertBaseFiles(std::set<std::string>& fileSet,
								const std::string& path,
								const std::string& pattern);

	static bool InsertRawDirs(std::set<std::string>& dirSet,
								const std::string& path,
								const std::string& pattern);
	static bool InsertModDirs(std::set<std::string>& dirSet,
								const std::string& path,
								const std::string& pattern);
	static bool InsertMapDirs(std::set<std::string>& dirSet,
								const std::string& path,
								const std::string& pattern);
	static bool InsertBaseDirs(std::set<std::string>& dirSet,
								const std::string& path,
								const std::string& pattern);

	std::string filename;
	std::ifstream* ifs;
	std::vector<boost::uint8_t> fileBuffer;
	int filePos;
	int fileSize;
};

#endif // __FILE_HANDLER_H__
