/**
 * @file FileSystem.cpp
 * @brief Abstracts locating of content on different platforms
 * @author Tobi Vollebregt
 *
 * FindFiles implementation by Chris Han.
 * Glob conversion by Chris Han (based on work by Nathaniel Smith).
 * Copyright (C) 2005-2006.  Licensed under the terms of the
 * GNU GPL, v2 or later
 */

#include "StdAfx.h"
#include <assert.h>
#include <limits.h>
#include <boost/regex.hpp>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include "FileSystem.h"

#ifdef WIN32
#	include "Win/WinFileSystemHandler.h"
#elif defined(__APPLE__)
#	include "Mac/MacFileSystemHandler.h"
#else
#	include "Linux/UnixFileSystemHandler.h"
#endif

#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/VFSHandler.h"
#include "LogOutput.h"
#include "mmgr.h"

#define fs (FileSystemHandler::GetInstance())

FileSystem filesystem;

////////////////////////////////////////
////////// FileSystemHandler

/**
 * @brief The global data directory handler instance
 */
FileSystemHandler* FileSystemHandler::instance;

/**
 * @brief get the data directory handler instance
 */
FileSystemHandler& FileSystemHandler::GetInstance()
{
	if (!instance)
		Initialize(false);
	return *instance;
}

/**
 * @brief initialize the data directory handler
 */
void FileSystemHandler::Initialize(bool verbose)
{
	if (!instance) {
#ifdef WIN32
		instance = new WinFileSystemHandler(verbose);
#elif defined(__APPLE__)
		instance = new MacFileSystemHandler(verbose);
#else
		instance = new UnixFileSystemHandler(verbose);
#endif
	}
}

void FileSystemHandler::Cleanup()
{
	delete instance;
	instance = NULL;
}

FileSystemHandler::~FileSystemHandler()
{
	delete archiveScanner;
	delete hpiHandler;
	archiveScanner = NULL;
	hpiHandler = NULL;
}

FileSystemHandler::FileSystemHandler(int native_path_sep): native_path_separator(native_path_sep)
{
	// need to set this here already or we get stuck in an infinite loop
	// because WinFileSystemHandler/UnixFileSystemHandler ctor initializes the
	// ArchiveScanner (which uses FileSystemHandler::GetInstance again...).
	instance = this;
}

/**
 * @brief return a writable directory
 */
std::string FileSystemHandler::GetWriteDir() const
{
	char buf[3];
	buf[0] = '.';
	buf[1] = native_path_separator;
	buf[2] = 0;
	return buf;
}

/**
 * @brief return a vector of all data directories
 */
std::vector<std::string> FileSystemHandler::GetDataDirectories() const
{
	std::vector<std::string> f;
	f.push_back(FileSystemHandler::GetWriteDir());
	return f;
}

/**
 * @brief locate a file
 *
 * This implementation just assumes the file lives in the current working
 * directory, so it just returns it's argument: file.
 */
std::string FileSystemHandler::LocateFile(const std::string& file) const
{
	return file;
}

////////////////////////////////////////
////////// FileSystem

/**
 * @brief quote macro
 * @param c Character to test
 * @param str string currently being built
 *
 * Given a string str that we're assembling,
 * and an upcoming character c, will append
 * an extra '\\' to quote the character if necessary.
 */
#define QUOTE(c,str)			\
	do {					\
		if (!(isalnum(c)||(c)=='_'))	\
			str+='\\';		\
		str+=c;				\
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
		if (braces>=5)
			logOutput.Print("glob_to_regex warning: braces nested too deeply\n%s", glob.c_str());
#endif
		switch (c) {
			case '*':
				regex+=".*";
				break;
			case '?':
				regex+='.';
				break;
			case '{':
				braces++;
				regex+='(';
				break;
			case '}':
#ifdef DEBUG
				if (!braces)
					logOutput.Print("glob_to_regex warning: closing brace without an equivalent opening brace\n%s", glob.c_str());
#endif
				regex+=')';
				braces--;
				break;
			case ',':
				if (braces)
					regex+='|';
				else
					QUOTE(c,regex);
				break;
			case '\\':
#ifdef DEBUG
				if (++i==glob.end())
					logOutput.Print("glob_to_regex warning: pattern ends with backslash\n%s", glob.c_str());
#else
				++i;
#endif
				QUOTE(*i,regex);
				break;
			default:
				QUOTE(c,regex);
				break;
		}
	}
#ifdef DEBUG
	if (braces)
		logOutput.Print("glob_to_regex warning: unterminated brace expression\n%s", glob.c_str());
#endif
	return regex;
}

/**
 * @brief get filesize
 *
 * @return the filesize or 0 if the file doesn't exist.
 */
size_t FileSystem::GetFilesize(std::string file) const
{
	if (!CheckFile(file))
		return 0;
	FixSlashes(file);
	struct stat info;
	if (stat(file.c_str(), &info) != 0)
		return 0;
	return info.st_size;
}

/**
 * @brief create a directory
 *
 * Works like mkdir -p, ie. attempts to create parent directories too.
 * Operates on the current working directory.
 */
bool FileSystem::CreateDirectory(std::string dir) const
{
	if (!CheckFile(dir))
		return false;

	ForwardSlashes(dir);
	size_t prev_slash = 0, slash;
	while ((slash = dir.find('/', prev_slash + 1)) != std::string::npos) {
		if (!fs.mkdir(dir.substr(0, slash)))
			return false;
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
	}
	else {
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
 */
std::string FileSystem::GetDirectory(const std::string& path) const
{
	size_t s = path.find_last_of('/');
	size_t bs = path.find_last_of('\\');
	if (s != std::string::npos && bs != std::string::npos)
		return path.substr(0, std::max(s, bs) + 1);
	if (s != std::string::npos)
		return path.substr(0, s + 1);
	if (bs != std::string::npos)
		return path.substr(0, bs + 1);
	return path;
}

/**
 * @brief get the filename part of a path
 */
std::string FileSystem::GetFilename(const std::string& path) const
{
	size_t s = path.find_last_of('/');
	size_t bs = path.find_last_of('\\');
	if (s != std::string::npos && bs != std::string::npos)
		return path.substr(std::max(s, bs) + 1);
	if (s != std::string::npos)
		return path.substr(s + 1);
	if (bs != std::string::npos)
		return path.substr(bs + 1);
	return path;
}

/**
 * @brief get the basename part of a path, ie. the filename without extension
 */
std::string FileSystem::GetBasename(const std::string& path) const
{
	std::string fn = GetFilename(path);
	size_t dot = fn.find_last_of('.');
	if (dot != std::string::npos)
		return fn.substr(0, dot);
	return fn;
}

/**
 * @brief get the extension of the filename part of the path
 */
std::string FileSystem::GetExtension(const std::string& path) const
{
	std::string fn = GetFilename(path);
	size_t dot = fn.find_last_of('.');
	if (dot != std::string::npos)
		return fn.substr(dot + 1);
	return "";
}

/**
 * @brief converts all slashes and backslashes in path to the native_path_separator
 */
std::string& FileSystem::FixSlashes(std::string& path) const
{
	int sep = fs.GetNativePathSeparator();
	for (unsigned i = 0; i < path.size(); ++i)
		if (path[i] == '/' || path[i] == '\\')
			path[i] = sep;
	return path;
}

/**
 * @brief converts backslashes in path to forward slashes
 */
std::string& FileSystem::ForwardSlashes(std::string& path) const
{
	for (unsigned i = 0; i < path.size(); ++i)
		if (path[i] == '\\')
			path[i] = '/';
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
	if (file.find("..") != std::string::npos)
		return false;

	return true;
}

/**
 * @brief does a little checking of a modestring
 */
bool FileSystem::CheckMode(const char* mode) const
{
	assert(mode && *mode);
	return true;
}

/**
 * @brief locate a file
 *
 * Attempts to locate a file.
 *
 * If the FileSystem::WRITE flag is set, it just returns the argument
 * (assuming the file should come in the current working directory).
 * If FileSystem::CREATE_DIRS is set it tries to create the subdirectory
 * the file should live in.
 *
 * Otherwise (if flags == 0), it dispatches the call to
 * FileSystemHandler::LocateFile(), which either searches for it in multiple
 * data directories (UNIX) or just returns the argument (Windows).
 */
std::string FileSystem::LocateFile(std::string file, int flags) const
{
	if (!CheckFile(file))
		return "";
	FixSlashes(file);

	if (flags & WRITE) {

		if (flags & CREATE_DIRS)
			CreateDirectory(GetDirectory(file));

		return file;
	}
	return fs.LocateFile(file);
}

/**
 * @brief remove a file
 *
 * Operates on the current working directory.
 */
bool FileSystem::Remove(std::string file) const
{
	if (!CheckFile(file))
		return false;
	FixSlashes(file);
	return ::remove(file.c_str()) == 0;
}
