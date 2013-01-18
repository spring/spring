/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DataDirsAccess.h"
#include "DataDirLocater.h"
#include "FileHandler.h"
#include "FileQueryFlags.h"
#include "FileSystem.h"

#include <cassert>
#include <string>
#include <vector>


DataDirsAccess dataDirsAccess;


std::vector<std::string> DataDirsAccess::FindFiles(std::string dir, const std::string& pattern, int flags) const
{
	if (!FileSystem::CheckFile(dir)) {
		return std::vector<std::string>();
	}

	if (dir.empty()) {
		dir = "./";
	} else {
		const char lastChar = dir[dir.length() - 1];
		if ((lastChar != '/') && (lastChar != '\\')) {
			dir += '/';
		}
	}

	FileSystem::FixSlashes(dir);

	if (flags & FileQueryFlags::ONLY_DIRS) {
		flags |= FileQueryFlags::INCLUDE_DIRS;
	}

	return FindFilesInternal(dir, pattern, flags);
}

std::vector<std::string> DataDirsAccess::FindFilesInternal(const std::string& dir, const std::string& pattern, int flags) const
{
	std::vector<std::string> matches;

	// if it is an absolute path, do not look for it in the data directories
	if (FileSystem::IsAbsolutePath(dir)) {
		// pass the directory as second directory argument,
		// so the path gets included in the matches.
		FindFilesSingleDir(matches, "", dir, pattern, flags);
		return matches;
	}

	std::string dir2 = FileSystem::RemoveLocalPathPrefix(dir);

	const std::vector<DataDir>& datadirs = dataDirLocater.GetDataDirs();
	for (std::vector<DataDir>::const_reverse_iterator d = datadirs.rbegin(); d != datadirs.rend(); ++d) {
		FindFilesSingleDir(matches, d->path, dir2, pattern, flags);
	}
	return matches;
}

std::string DataDirsAccess::LocateFileInternal(const std::string& file) const
{
	// if it's an absolute path, don't look for it in the data directories
	if (FileSystem::IsAbsolutePath(file)) {
		return file;
	}

	const std::vector<DataDir>& datadirs = dataDirLocater.GetDataDirs();
	for (std::vector<DataDir>::const_iterator d = datadirs.begin(); d != datadirs.end(); ++d) {
		std::string fn(d->path + file);
		// does the file exist, and is it readable?
		if (FileSystem::IsReadableFile(fn)) {
			return fn;
		}
	}

	return file;
}


void DataDirsAccess::FindFilesSingleDir(std::vector<std::string>& matches, const std::string& datadir, const std::string& dir, const std::string& pattern, int flags) const
{
	assert(datadir.empty() || datadir[datadir.length() - 1] == FileSystem::GetNativePathSeparator());

	const std::string regexPattern = FileSystem::ConvertGlobToRegex(pattern);

	FileSystem::FindFiles(matches, datadir, dir, regexPattern, flags);
}



std::string DataDirsAccess::LocateFile(std::string file, int flags) const
{
	if (!FileSystem::CheckFile(file)) {
		return "";
	}

	// if it is an absolute path, do not look for it in the data directories
	if (FileSystem::IsAbsolutePath(file)) {
		return file;
	}

	FileSystem::FixSlashes(file);

	if (flags & FileQueryFlags::WRITE) {
		std::string writeableFile = dataDirLocater.GetWriteDirPath() + file;
		FileSystem::FixSlashes(writeableFile);
		if (flags & FileQueryFlags::CREATE_DIRS) {
			FileSystem::CreateDirectory(FileSystem::GetDirectory(writeableFile));
		}
		return writeableFile;
	}

	return LocateFileInternal(file);
}

std::string DataDirsAccess::LocateDir(std::string _dir, int flags) const
{
	if (!FileSystem::CheckFile(_dir)) {
		return "";
	}

	// if it's an absolute path, don't look for it in the data directories
	if (FileSystem::IsAbsolutePath(_dir)) {
		return _dir;
	}

	std::string dir = _dir;
	FileSystem::FixSlashes(dir);

	if (flags & FileQueryFlags::WRITE) {
		std::string writeableDir = dataDirLocater.GetWriteDirPath() + dir;
		FileSystem::FixSlashes(writeableDir);
		if (flags & FileQueryFlags::CREATE_DIRS) {
			FileSystem::CreateDirectory(writeableDir);
		}
		return writeableDir;
	} else {
		const std::vector<std::string>& datadirs = dataDirLocater.GetDataDirPaths();
		std::vector<std::string>::const_iterator dd;
		for (dd = datadirs.begin(); dd != datadirs.end(); ++dd) {
			std::string dirPath((*dd) + dir);
			if (FileSystem::DirExists(dirPath)) {
				return dirPath;
			}
		}
		return dir;
	}
}
std::vector<std::string> DataDirsAccess::LocateDirs(const std::string& _dir) const
{
	std::vector<std::string> found;

	if (!FileSystem::CheckFile(_dir) || FileSystem::IsAbsolutePath(_dir)) {
		return found;
	}

	std::string dir = _dir;
	FileSystem::FixSlashes(dir);

	const std::vector<std::string>& datadirs = dataDirLocater.GetDataDirPaths();
	std::vector<std::string>::const_iterator dd;
	for (dd = datadirs.begin(); dd != datadirs.end(); ++dd) {
		std::string dirPath((*dd) + dir);
		if (FileSystem::DirExists(dirPath)) {
			found.push_back(dirPath);
		}
	}

	return found;
}

std::vector<std::string> DataDirsAccess::FindDirsInDirectSubDirs(
		const std::string& relPath) const {

	std::vector<std::string> found;

	static const std::string pattern = "*";

	// list of all occurences of the relative path in the data directories
	const std::vector<std::string> &rootDirs = LocateDirs(relPath);

	// list of subdirs in all occurences of the relative path in the data directories
	std::vector<std::string> mainDirs;

	// find all subdirectories in the rootDirs
	std::vector<std::string>::const_iterator dir;
	for (dir = rootDirs.begin(); dir != rootDirs.end(); ++dir) {
		const std::vector<std::string>& localMainDirs = CFileHandler::SubDirs(*dir, pattern, SPRING_VFS_RAW);
		mainDirs.insert(mainDirs.end(), localMainDirs.begin(), localMainDirs.end());
	}
	//found.insert(found.end(), mainDirs.begin(), mainDirs.end());

	// and add all subdriectories of these
	for (dir = mainDirs.begin(); dir != mainDirs.end(); ++dir) {
		const std::vector<std::string>& subDirs = CFileHandler::SubDirs(*dir, pattern, SPRING_VFS_RAW);
		found.insert(found.end(), subDirs.begin(), subDirs.end());
	}

	return found;
}


bool DataDirsAccess::InReadDir(const std::string& path)
{
	std::string locatedFile = LocateFile(path);
	return (locatedFile != "") && (locatedFile != path);
}


bool DataDirsAccess::InWriteDir(const std::string& path)
{
	std::string locatedFile = LocateFile(path, FileQueryFlags::WRITE);
	return (locatedFile != "") && (locatedFile != path);
}
