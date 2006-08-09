/**
 * @file FileSystemHandler.cpp
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
#include <boost/filesystem/operations.hpp>
#include <boost/regex.hpp>
#include <sys/stat.h>
#include <sys/types.h>
#include "FileSystem.h"
#ifdef WIN32
#include "Win/WinFileSystemHandler.h"
#else
#include "Linux/UnixFileSystemHandler.h"
#endif
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/VFSHandler.h"
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
	if (!instance) {
#ifdef WIN32
		instance = new WinFileSystemHandler();
#else
		instance = new UnixFileSystemHandler();
#endif
	}
	return *instance;
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

bool FileSystemHandler::remove(const std::string& file) const
{
	return remove(file.c_str()) == 0;
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
 * @brief get size of a file
 */
size_t FileSystemHandler::GetFilesize(const std::string& path) const
{
	struct stat info;
	if (stat(path.c_str(), &info) != 0)
		return 0;
	return info.st_size;
}

/**
 * @brief internal find files
 *
 * Helper function for FileSystemHandler::FindFiles.
 *
 * This one assumes dirpath is not empty, exists & is a directory,
 * and it expects arguments of types boost::filesystem::path and boost::regex instead
 * of 2x std::string.
 *
 * This function does the real work: the actual walking through the
 * directory tree.
 */
static std::vector<std::string> FindFiles(const boost::filesystem::path& dirpath, const boost::regex &regexpattern, const bool recurse, const bool include_dirs)
{
	std::vector<std::string> matches;
	boost::filesystem::directory_iterator enditr;
	for (boost::filesystem::directory_iterator diritr(dirpath); diritr != enditr; ++diritr) {
		// Don't traverse hidden entries (ie. starting with a dot).
		//Exclude broken symlinks by checking if the file actually exists.
		if (diritr->leaf()[0] != '.' && boost::filesystem::exists(*diritr)) {
			if (!boost::filesystem::is_directory(*diritr)) {
				if (boost::regex_match(diritr->leaf(), regexpattern))
					matches.push_back(diritr->native_file_string());
			} else if (recurse) {
				if (include_dirs && boost::regex_match(diritr->leaf(), regexpattern))
					matches.push_back(diritr->native_file_string());
				std::vector<std::string> submatch = FindFiles(*diritr, regexpattern, recurse, include_dirs);
				if (!submatch.empty()) {
					for (std::vector<std::string>::iterator it=submatch.begin(); it != submatch.end(); it++)
						matches.push_back(*it);
				}
			}
		}
	}
	return matches;
}

/**
 * @brief find files
 * @param dirpath path in which to start looking
 * @param pattern pattern to search for
 * @param recurse whether or not to recursively search
 * @param include_dirs whether or not to include directory names in the result
 * @return vector of std::strings
 *
 * Will search for a file given a particular pattern.
 * Starts from dirpath, descending down if recurse is true.
 */
std::vector<std::string> FileSystemHandler::FindFiles(const std::string& dir, const std::string &pattern, bool recurse, bool include_dirs) const
{
	std::vector<std::string> matches;
	boost::filesystem::path dirpath(dir, boost::filesystem::no_check);
	if (boost::filesystem::exists(dirpath) && boost::filesystem::is_directory(dirpath)) {
		boost::regex regexpattern(filesystem.glob_to_regex(pattern));
		matches = ::FindFiles(dirpath, regexpattern, recurse, include_dirs);
	} else {
#ifdef DEBUG
		fprintf(stderr,"find_files warning: search path %s is not a directory\n",dirpath.string().c_str());
#endif
	}
	return matches;
}

std::vector<std::string> FileSystemHandler::GetNativeFilenames(const std::string& file, bool write) const
{
	std::vector<std::string> f;
	f.push_back(file);
	return f;
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
			fprintf(stderr,"glob_to_regex warning: braces nested too deeply\n%s\n", glob.c_str());
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
					fprintf(stderr,"glob_to_regex warning: closing brace without an equivalent opening brace\n%s\n", glob.c_str());
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
					fprintf(stderr,"glob_to_regex warning: pattern ends with backslash\n%s\n", glob.c_str());
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
		fprintf(stderr,"glob_to_regex warning: unterminated brace expression\n%s\n", glob.c_str());
#endif
	return regex;
}

std::string FileSystem::GetWriteDir() const
{
	return fs.GetWriteDir();
}

FILE* FileSystem::fopen(std::string file, const char* mode) const
{
	if (!CheckFile(file) || !CheckMode(mode))
		return NULL;
	FixSlashes(file);
	return fs.fopen(file, mode);
}

std::ifstream* FileSystem::ifstream(std::string file, std::ios_base::openmode mode) const
{
	if (!CheckFile(file))
		return NULL;
	FixSlashes(file);
	return fs.ifstream(file, mode);
}

std::ofstream* FileSystem::ofstream(std::string file, std::ios_base::openmode mode) const
{
	if (!CheckFile(file))
		return NULL;
	FixSlashes(file);
	return fs.ofstream(file, mode);
}

size_t FileSystem::GetFilesize(std::string file) const
{
	if (!CheckFile(file))
		return 0;
	FixSlashes(file);
	return fs.GetFilesize(file);
}

bool FileSystem::CreateDirectory(std::string dir) const
{
	if (!CheckFile(dir))
		return false;
	FixSlashes(dir);
	return fs.mkdir(dir);
}

std::vector<std::string> FileSystem::FindFiles(std::string dir, const std::string& pattern, bool recurse, bool include_dirs) const
{
	if (!CheckFile(dir))
		return std::vector<std::string>();
	if (dir.empty())
		dir = "./";
	if (dir[dir.length() - 1] != '/' && dir[dir.length() - 1] != '\\')
		dir += '/';
	FixSlashes(dir);
	return fs.FindFiles(dir, pattern, recurse, include_dirs);
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
	return fn.substr(0, fn.find_last_of('.'));
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

std::vector<std::string> FileSystem::GetNativeFilenames(std::string file, bool write) const
{
	if (!CheckFile(file))
		return std::vector<std::string>();
	FixSlashes(file);
	return fs.GetNativeFilenames(file, write);
}

bool FileSystem::Remove(std::string file) const
{
	if (!CheckFile(file))
		return false;
	FixSlashes(file);
	return fs.remove(file);
}
