/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief Abstracts locating of content on different platforms
 * Glob conversion by Chris Han (based on work by Nathaniel Smith).
 */

#include "StdAfx.h"
#include "FileSystem.h"

#include <assert.h>
#include <limits.h>
#include <fstream>

#include "FileHandler.h"
#include "FileSystemHandler.h"
#include "LogOutput.h"
#include "Util.h"
#include "mmgr.h"


#define fs (FileSystemHandler::GetInstance())

FileSystem filesystem;

////////////////////////////////////////
////////// FileSystem

/**
 * @brief quote macro
 * @param c Character to test
 * @param str string currently being built
 *
 * Given an std::string str that we are assembling,
 * and an upcoming char c, will append
 * an extra '\\' to quote the character if necessary.
 * The do-while is used for legalizing the ';' in "QUOTE(c, regex);".
 */
#define QUOTE(c,str)			\
	do {					\
		if (!(isalnum(c) || (c) == '_'))	\
			str += '\\';		\
		str += c;				\
	} while (0)

/**
 * @brief glob to regex
 * @param glob string containing glob
 * @return string containing regex
 *
 * Converts a glob expression to a regex
 */
std::string FileSystem::glob_to_regex(const std::string& glob) const
{
	std::string regex;
	regex.reserve(glob.size()<<1);
	int braces = 0;
	for (std::string::const_iterator i = glob.begin(); i != glob.end(); ++i) {
		char c = *i;
#ifdef DEBUG
		if (braces >= 5) {
			logOutput.Print("glob_to_regex warning: braces nested too deeply\n%s", glob.c_str());
		}
#endif
		switch (c) {
			case '*':
				regex += ".*";
				break;
			case '?':
				regex += '.';
				break;
			case '{':
				braces++;
				regex += '(';
				break;
			case '}':
#ifdef DEBUG
				if (braces == 0) {
					logOutput.Print("glob_to_regex warning: closing brace without an equivalent opening brace\n%s", glob.c_str());
				}
#endif
				regex += ')';
				braces--;
				break;
			case ',':
				if (braces > 0) {
					regex += '|';
				} else {
					QUOTE(c,regex);
				}
				break;
			case '\\':
				++i;
#ifdef DEBUG
				if (i == glob.end()) {
					logOutput.Print("glob_to_regex warning: pattern ends with backslash\n%s", glob.c_str());
				}
#endif
				QUOTE(*i,regex);
				break;
			default:
				QUOTE(c,regex);
				break;
		}
	}

#ifdef DEBUG
	if (braces > 0) {
		logOutput.Print("glob_to_regex warning: unterminated brace expression\n%s", glob.c_str());
	} else if (braces < 0) {
		logOutput.Print("glob_to_regex warning: too many closing braces\n%s", glob.c_str());
	}
#endif

	return regex;
}

bool FileSystem::FileExists(std::string file) const
{
	FixSlashes(file);
	return FileSystemHandler::FileExists(file);
}

/**
 * @brief get filesize
 *
 * @return the filesize or 0 if the file doesn't exist.
 */
size_t FileSystem::GetFileSize(std::string file) const
{
	if (!CheckFile(file)) {
		return 0;
	}
	FixSlashes(file);
	return FileSystemHandler::GetFileSize(file);
}

/**
 * @brief creates a directory recursively
 *
 * Works like mkdir -p, ie. attempts to create parent directories too.
 * Operates on the current working directory.
 */
bool FileSystem::CreateDirectory(std::string dir) const
{
	if (!CheckFile(dir)) {
		return false;
	}

	ForwardSlashes(dir);
	size_t prev_slash = 0, slash;
	while ((slash = dir.find('/', prev_slash + 1)) != std::string::npos) {
		std::string pathPart = dir.substr(0, slash);
		if (!FileSystemHandler::IsFSRoot(pathPart) && !fs.mkdir(pathPart)) {
			return false;
		}
		prev_slash = slash;
	}
	return fs.mkdir(dir);
}


/**
 * @brief find files
 *
 * Compiles a std::vector of all files below dir that match pattern.
 *
 * If FileSystem::RECURSE flag is set it recurses into subdirectories.
 * If FileSystem::INCLUDE_DIRS flag is set it includes directories in the
 * result too, as long as they match the pattern.
 *
 * Note that pattern doesn't apply to the names of recursed directories,
 * it does apply to the files inside though.
 */
std::vector<std::string> FileSystem::FindFiles(std::string dir, const std::string& pattern, int flags) const
{
	if (!CheckFile(dir)) {
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

	FixSlashes(dir);

	if (flags & ONLY_DIRS) {
		flags |= INCLUDE_DIRS;
	}

	return fs.FindFiles(dir, pattern, flags);
}


/**
 * @brief get the directory part of a path
 * "/home/user/.spring/test.txt" -> "/home/user/.spring"
 */
std::string FileSystem::GetDirectory(const std::string& path) const
{
	size_t s = path.find_last_of("\\/");
	if (s != std::string::npos) {
		return path.substr(0, s + 1);
	}
	return path;
}

/**
 * @brief get the filename part of a path
 * "/home/user/.spring/test.txt" -> "test.txt"
 */
std::string FileSystem::GetFilename(const std::string& path) const
{
	size_t s = path.find_last_of("\\/");
	if (s != std::string::npos) {
		return path.substr(s + 1);
	}
	return path;
}

/**
 * @brief get the basename part of a path, ie. the filename without extension
 * "/home/user/.spring/test.txt" -> "test"
 */
std::string FileSystem::GetBasename(const std::string& path) const
{
	std::string fn = GetFilename(path);
	size_t dot = fn.find_last_of('.');
	if (dot != std::string::npos) {
		return fn.substr(0, dot);
	}
	return fn;
}

/**
 * @brief get the extension of the filename part of the path
 * "/home/user/.spring/test.txt" -> "txt"
 */
std::string FileSystem::GetExtension(const std::string& path) const
{
	const std::string fileName = GetFilename(path);
	size_t l = fileName.length();
//#ifdef WIN32
	//! windows eats dots and spaces at the end of filenames
	while (l > 0) {
		const char prevChar = fileName[l-1];
		if ((prevChar == '.') || (prevChar == ' ')) {
			l--;
		} else {
			break;
		}
	}
//#endif
	size_t dot = fileName.rfind('.', l);
	if (dot != std::string::npos) {
		return StringToLower(fileName.substr(dot + 1));
	}

	return "";
}

/**
 * @brief converts all slashes and backslashes in path to the native_path_separator
 */
std::string& FileSystem::FixSlashes(std::string& path) const
{
	int sep = fs.GetNativePathSeparator();
	for (size_t i = 0; i < path.size(); ++i) {
		if (path[i] == '/' || path[i] == '\\') {
			path[i] = sep;
		}
	}

	return path;
}

/**
 * @brief converts backslashes in path to forward slashes
 */
std::string& FileSystem::ForwardSlashes(std::string& path) const
{
	for (size_t i = 0; i < path.size(); ++i) {
		if (path[i] == '\\') {
			path[i] = '/';
		}
	}

	return path;
}

/**
 * @brief does a little checking of a filename
 */
bool FileSystem::CheckFile(const std::string& file) const
{
	// Don't allow code to escape from the data directories.
	// Note: this does NOT mean this is a SAFE fopen function:
	// symlink-, hardlink-, you name it-attacks are all very well possible.
	// The check is just ment to "enforce" certain coding behaviour.
	bool hasParentRef = (file.find("..") != std::string::npos);

	return !hasParentRef;
}
// bool FileSystem::CheckDir(const std::string& dir) const {
// 	return CheckFile(dir);
// }

std::string FileSystem::LocateFile(std::string file, int flags) const
{
	if (!CheckFile(file)) {
		return "";
	}

	// if it is an absolute path, do not look for it in the data directories
	if (FileSystemHandler::IsAbsolutePath(file)) {
		return file;
	}

	FixSlashes(file);

	if (flags & WRITE) {
		std::string writeableFile = fs.GetWriteDir() + file;
		FixSlashes(writeableFile);
		if (flags & CREATE_DIRS) {
			CreateDirectory(GetDirectory(writeableFile));
		}
		return writeableFile;
	}

	return fs.LocateFile(file);
}

std::string FileSystem::LocateDir(std::string _dir, int flags) const
{
	if (!CheckFile(_dir)) {
		return "";
	}

	// if it's an absolute path, don't look for it in the data directories
	if (FileSystemHandler::IsAbsolutePath(_dir)) {
		return _dir;
	}

	std::string dir = _dir;
	FixSlashes(dir);

	if (flags & WRITE) {
		std::string writeableDir = fs.GetWriteDir() + dir;
		FixSlashes(writeableDir);
		if (flags & CREATE_DIRS) {
			CreateDirectory(writeableDir);
		}
		return writeableDir;
	} else {
		const std::vector<std::string>& datadirs = fs.GetDataDirectories();
		std::vector<std::string>::const_iterator dd;
		for (dd = datadirs.begin(); dd != datadirs.end(); ++dd) {
			std::string dirPath((*dd) + dir);
			if (fs.DirExists(dirPath)) {
				return dirPath;
			}
		}
		return dir;
	}
}
std::vector<std::string> FileSystem::LocateDirs(const std::string& _dir) const
{
	std::vector<std::string> found;

	if (!CheckFile(_dir) || FileSystemHandler::IsAbsolutePath(_dir)) {
		return found;
	}

	std::string dir = _dir;
	FixSlashes(dir);

	const std::vector<std::string>& datadirs = fs.GetDataDirectories();
	std::vector<std::string>::const_iterator dd;
	for (dd = datadirs.begin(); dd != datadirs.end(); ++dd) {
		std::string dirPath((*dd) + dir);
		if (fs.DirExists(dirPath)) {
			found.push_back(dirPath);
		}
	}

	return found;
}

std::vector<std::string> FileSystem::FindDirsInDirectSubDirs(
		const std::string& relPath) const {

	std::vector<std::string> found;

	static const std::string pattern = "*";

	// list of all occurences of the relative path in the data directories
	std::vector<std::string> rootDirs = LocateDirs(relPath);

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


bool FileSystem::InReadDir(const std::string& path)
{
	std::string locatedFile = LocateFile(path);
	return (locatedFile != "") && (locatedFile != path);
}


bool FileSystem::InWriteDir(const std::string& path, const std::string& prefix)
{
	std::string locatedFile = LocateFile(path, WRITE);
	return (locatedFile != "") && (locatedFile != path);
}


/**
 * @brief remove a file
 *
 * Operates on the current working directory.
 */
bool FileSystem::Remove(std::string file) const
{
	if (!CheckFile(file)) {
		return false;
	}
	FixSlashes(file);
	return FileSystemHandler::DeleteFile(file);
}
