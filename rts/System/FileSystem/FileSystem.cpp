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
#include "FileSystem.h"

#include <assert.h>
#include <limits.h>
#include <boost/regex.hpp>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#ifndef _WIN32
	#include <dirent.h>
	#include <sstream>
	#include <unistd.h>
#else
	#include <windows.h>
	#include <io.h>
	#include <direct.h>
	// winapi redifines these which breaks things
	#if defined(CreateDirectory)
		#undef CreateDirectory
	#endif
	#if defined(DeleteFile)
		#undef DeleteFile
	#endif
#endif

#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/VFSHandler.h"
#include "FileSystem/FileHandler.h"
#include "ConfigHandler.h"
#include "LogOutput.h"
#include "Exceptions.h"
#include "Util.h"
#include "mmgr.h"


#define fs (FileSystemHandler::GetInstance())

FileSystem filesystem;

std::string StripTrailingSlashes(std::string path)
{
	while (!path.empty() && (path.at(path.length()-1) == '\\' || path.at(path.length()-1) == '/'))
		path = path.substr(0, path.length()-1);
	return path;
}

////////////////////////////////////////
////////// FileSystemHandler

/**
 * @brief The global data directory handler instance
 */
FileSystemHandler* FileSystemHandler::instance = NULL;

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
		instance = new FileSystemHandler();
		try {
			instance->locater.LocateDataDirs();
			instance->InitVFS();
		}
		catch (...) {
			SafeDelete(instance);
			throw;
		}
	}
}

void FileSystemHandler::Cleanup()
{
	SafeDelete(instance);
}

FileSystemHandler::~FileSystemHandler()
{
	SafeDelete(archiveScanner);
	SafeDelete(vfsHandler);
	configHandler->Deallocate();
}

#ifndef _WIN32
const int FileSystemHandler::native_path_separator = '/';
#else
const int FileSystemHandler::native_path_separator = '\\';
#endif

FileSystemHandler::FileSystemHandler()
{
	// NOTE TO SELF: NEVER EVAR DO THIS AGAIN THIS WAY

	// combined with init code in constructor and exceptions being thrown
	// when this init code fails it results in dangling pointers.

	// (if constructor throws exception object doesn't exist, by definition,
	//  but global variable instance would still point to it....)

//	// need to set this here already or we get stuck in an infinite loop
//	// because WinFileSystemHandler/FileSystemHandler ctor initializes the
//	// ArchiveScanner (which uses FileSystemHandler::GetInstance again...).
//	instance = this;
}

/**
 * @brief returns the highest priority writable directory, aka the writedir
 */
std::string FileSystemHandler::GetWriteDir() const
{
	const DataDir* writedir = locater.GetWriteDir();
	assert(writedir && writedir->writable); // duh
	return writedir->path;
}

/**
 * Removes "./" or ".\" from the start of a path string.
 */
static std::string RemoveLocalPathPrefix(const std::string& path)
{
	std::string p(path);

	if (p.length() >= 2 && p[0] == '.' && (p[1] == '/' || p[1] == '\\')) {
	    p.erase(0, 2);
	}

	return p;
}

bool FileSystemHandler::IsFSRoot(const std::string& p)
{
#ifdef WIN32
	if (p.length() >= 2 && p[1] == ':' && ((p[0] >= 'a' && p[0] <= 'z') || (p[0] >= 'A' && p[0] <= 'Z')) && (p.length() == 2 || (p.length() == 3 && (p[2] == '\\' || p[2] == '/')))) {
#else
	if (p.length() == 1 && p[0] == '/') {
#endif
		return true;
	}

	return false;
}

/**
 * @brief find files
 * @param dir path in which to start looking (tried relative to each data directory)
 * @param pattern pattern to search for
 * @param recurse whether or not to recursively search
 * @param include_dirs whether or not to include directory names in the result
 * @return vector of std::strings containing absolute paths to the files
 *
 * Will search for a file given a particular pattern.
 * Starts from dirpath, descending down if recurse is true.
 */
std::vector<std::string> FileSystemHandler::FindFiles(const std::string& dir, const std::string& pattern, int flags) const
{
	std::vector<std::string> matches;

	// if it's an absolute path, don't look for it in the data directories
	if (FileSystemHandler::IsAbsolutePath(dir)) {
		FindFilesSingleDir(matches, dir, pattern, flags);
		return matches;
	}

	std::string dir2 = RemoveLocalPathPrefix(dir);

	const std::vector<DataDir>& datadirs = locater.GetDataDirs();
	for (std::vector<DataDir>::const_reverse_iterator d = datadirs.rbegin(); d != datadirs.rend(); ++d) {
		FindFilesSingleDir(matches, d->path + dir2, pattern, flags);
	}
	return matches;
}

bool FileSystemHandler::IsReadableFile(const std::string& file) const
{
#ifdef WIN32
	return (_access(StripTrailingSlashes(file).c_str(), 4) == 0);
#else
	return (access(file.c_str(), R_OK | F_OK) == 0);
#endif
}

std::string FileSystemHandler::LocateFile(const std::string& file) const
{
	// if it's an absolute path, don't look for it in the data directories
	if (FileSystemHandler::IsAbsolutePath(file)) {
		return file;
	}

	const std::vector<DataDir>& datadirs = locater.GetDataDirs();
	for (std::vector<DataDir>::const_iterator d = datadirs.begin(); d != datadirs.end(); ++d) {
		std::string fn(d->path + file);
		// does the file exist, and is it readable?
		if (IsReadableFile(fn)) {
			return fn;
		}
	}
	return file;
}


std::vector<std::string> FileSystemHandler::GetDataDirectories() const
{
	std::vector<std::string> f;

	const std::vector<DataDir>& datadirs = locater.GetDataDirs();
	for (std::vector<DataDir>::const_iterator d = datadirs.begin(); d != datadirs.end(); ++d) {
		f.push_back(d->path);
	}
	return f;
}


bool FileSystemHandler::IsAbsolutePath(const std::string& path)
{
#ifdef WIN32
	return ((path.length() > 1) && (path[1] == ':'));
#else
	return ((path.length() > 0) && (path[0] == '/'));
#endif
}

/**
 * @brief creates a rwxr-xr-x dir in the writedir
 *
 * Returns true if the postcondition of this function is that dir exists in
 * the write directory.
 *
 * Note that this function does not check access to the dir, ie. if you've
 * created it manually with 0000 permissions then this function may return
 * true, subsequent operation on files inside the directory may still fail.
 *
 * As a rule of thumb, set identical permissions on identical items in the
 * data directory, ie. all subdirectories the same perms, all files the same
 * perms.
 */
bool FileSystemHandler::mkdir(const std::string& dir) const
{
	// First check if directory exists. We'll return success if it does.
	if (DirExists(dir)) {
		return true;
	}

	// If it doesn't exist we try to mkdir it and return success if that succeeds.
#ifndef _WIN32
	if (::mkdir(dir.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0)
#else
	if (::_mkdir(StripTrailingSlashes(dir).c_str()) == 0)
#endif
	{
		return true;
	}
	else
	{
		logOutput.Print("Could not create directory %s: %s", dir.c_str(), strerror(errno));
		// Otherwise we return false.
		return false;
	}
}

bool FileSystemHandler::DeleteFile(const std::string& file)
{
	if (remove(file.c_str()) == 0)
	{
		return true;
	}
	else
	{
		logOutput.Print("Could not delete file %s: %s", file.c_str(), strerror(errno));
		// Otherwise we return false.
		return false;
	}
}

bool FileSystemHandler::FileExists(const std::string& file)
{
#ifdef _WIN32
	struct _stat info;
	const int ret = _stat(StripTrailingSlashes(file).c_str(), &info);
	if ((ret == 0 && (info.st_mode & _S_IFREG)))
#else
	struct stat info;
	const int ret = stat(file.c_str(), &info);
	if ((ret == 0 && !S_ISDIR(info.st_mode)))
#endif
	{
		return true;
	}
	else
		return false;
}

bool FileSystemHandler::DirExists(const std::string& dir)
{
#ifdef _WIN32
	struct _stat info;
	const int ret = _stat(StripTrailingSlashes(dir).c_str(), &info);
	if ((ret == 0) && (info.st_mode & _S_IFDIR))
#else
	struct stat info;
	const int ret = stat(dir.c_str(), &info);
	if ((ret == 0) && S_ISDIR(info.st_mode))
#endif
		return true;
	else
		return false;
}


bool FileSystemHandler::DirIsWritable(const std::string& dir)
{
#ifdef _WIN32
	// this exists because _access doesn't do the right thing
	// see http://msdn.microsoft.com/en-us/library/1w06ktdy(VS.71).aspx
	// for now, try to create a temporary file in a directory and open it
	// to rule out the possibility of it being created in the virtual store
	// TODO perhaps use SECURITY_DESCRIPTOR winapi calls here

	std::string testfile = dir + "\\__$testfile42$.test";
	std::ofstream os(testfile.c_str());
	if (os.fail())
		return false;
	const char *testdata = "THIS IS A TEST";
	os << testdata;
	os.close();

	// this part should only be needed when there's no manifest embedded
	std::ifstream is(testfile.c_str());
	if (is.fail())
		return false; // the file most likely exists in the virtual store
	std::string input;
	getline(is, input);
	if (input != testdata) {
		unlink(testfile.c_str());
		return false;
	}
	is.close();
	unlink(testfile.c_str());
	return true;
#else
	return (access(dir.c_str(), W_OK) == 0);
#endif
}

void FileSystemHandler::Chdir(const std::string& dir)
{
#ifndef _WIN32
	const int err = chdir(dir.c_str());
#else
	const int err = _chdir(StripTrailingSlashes(dir).c_str());
#endif
	if (err)
		throw content_error("Could not chdir into SPRING_DATADIR");
}

static void FindFiles(std::vector<std::string>& matches, const std::string& dir, const boost::regex &regexpattern, int flags)
{
#ifdef _WIN32
	WIN32_FIND_DATA wfd;
	HANDLE hFind = FindFirstFile((dir+"\\*").c_str(), &wfd);

	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if(strcmp(wfd.cFileName,".") && strcmp(wfd.cFileName ,"..")) {
				if(!(wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)) {
					if ((flags & FileSystem::ONLY_DIRS) == 0) {
						if (boost::regex_match(wfd.cFileName, regexpattern)) {
							matches.push_back(dir + wfd.cFileName);
						}
					}
				}
				else {
					if (flags & FileSystem::INCLUDE_DIRS) {
						if (boost::regex_match(wfd.cFileName, regexpattern)) {
							matches.push_back(dir + wfd.cFileName + "\\");
						}
					}
					if (flags & FileSystem::RECURSE) {
						FindFiles(matches, dir + wfd.cFileName + "\\", regexpattern, flags);
					}
				}
			}
		} while (FindNextFile(hFind, &wfd));
		FindClose(hFind);
	}
#else
	DIR* dp;
	struct dirent* ep;

	if (!(dp = opendir(dir.c_str())))
		return;

	while ((ep = readdir(dp))) {
		// exclude hidden files
		if (ep->d_name[0] != '.') {
			// is it a file? (we just treat sockets / pipes / fifos / character&block devices as files...)
			// (need to stat because d_type is DT_UNKNOWN on linux :-/)
			struct stat info;
			if (stat((dir + ep->d_name).c_str(), &info) == 0) {
				if (!S_ISDIR(info.st_mode)) {
					if ((flags & FileSystem::ONLY_DIRS) == 0) {
						if (boost::regex_match(ep->d_name, regexpattern)) {
							matches.push_back(dir + ep->d_name);
						}
					}
				}
				else {
					// or a directory?
					if (flags & FileSystem::INCLUDE_DIRS) {
						if (boost::regex_match(ep->d_name, regexpattern)) {
							matches.push_back(dir + ep->d_name + "/");
						}
					}
					if (flags & FileSystem::RECURSE) {
						FindFiles(matches, dir + ep->d_name + "/", regexpattern, flags);
					}
				}
			}
		}
	}
	closedir(dp);
#endif
}


/**
 * @brief internal find-files-in-a-single-datadir-function
 * @param dir path in which to start looking
 * @param pattern pattern to search for
 * @param recurse whether or not to recursively search
 * @param include_dirs whether or not to include directory names in the result
 * @return vector of std::strings
 *
 * Will search for a file given a particular pattern.
 * Starts from dirpath, descending down if recurse is true.
 */
void FileSystemHandler::FindFilesSingleDir(std::vector<std::string>& matches, const std::string& dir, const std::string &pattern, int flags) const
{
	assert(!dir.empty() && dir[dir.length() - 1] == native_path_separator);

	boost::regex regexpattern(filesystem.glob_to_regex(pattern));

	::FindFiles(matches, dir, regexpattern, flags);
}

/**
 * @brief Creates the archive scanner and vfs handler
 *
 * For the archiveScanner, it keeps cache data ("archivecache.txt") in the
 * writedir. Cache data in other directories is ignored.
 * It scans maps, base and mods subdirectories of all readable datadirs
 * for archives. Archives in higher priority datadirs override archives
 * in lower priority datadirs.
 *
 * Note that the archive namespace is global, ie. each archive basename may
 * only occur once in all data directories. If a name is found more then once,
 * then higher priority datadirs override lower priority datadirs and per
 * datadir the 'mods' subdir overrides 'base' which overrides 'maps'.
 */
void FileSystemHandler::InitVFS() const
{
	const DataDir* writedir = locater.GetWriteDir();
	const std::vector<DataDir>& datadirs = locater.GetDataDirs();

	archiveScanner = new CArchiveScanner();

	archiveScanner->ReadCacheData(writedir->path + archiveScanner->GetFilename());

	std::vector<std::string> scanDirs;
	for (std::vector<DataDir>::const_reverse_iterator d = datadirs.rbegin(); d != datadirs.rend(); ++d) {
		scanDirs.push_back(d->path + "maps");
		scanDirs.push_back(d->path + "base");
		scanDirs.push_back(d->path + "mods");
		scanDirs.push_back(d->path + "packages");
	}
	archiveScanner->ScanDirs(scanDirs, true);

	archiveScanner->WriteCacheData(writedir->path + archiveScanner->GetFilename());

	vfsHandler = new CVFSHandler();
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
	if (dot != std::string::npos) {
		return fn.substr(0, dot);
	}
	return fn;
}

/**
 * @brief get the extension of the filename part of the path
 */
std::string FileSystem::GetExtension(const std::string& path) const
{
	std::string fn = GetFilename(path);
	size_t dot = fn.find_last_of('.');
	if (dot != std::string::npos) {
		return fn.substr(dot + 1);
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
	if (file.find("..") != std::string::npos)
		return false;

	return true;
}
// bool FileSystem::CheckDir(const std::string& dir) const {
// 	return CheckFile(dir);
// }

std::string FileSystem::LocateFile(std::string file, int flags) const
{
	if (!CheckFile(file)) {
		return "";
	}

	// if it's an absolute path, don't look for it in the data directories
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
		std::vector<std::string> localMainDirs =
				CFileHandler::SubDirs(*dir, pattern, SPRING_VFS_RAW);
		mainDirs.insert(mainDirs.end(), localMainDirs.begin(), localMainDirs.end());
	}
	//found.insert(found.end(), mainDirs.begin(), mainDirs.end());

	// and add all subdriectories of these
	for (dir = mainDirs.begin(); dir != mainDirs.end(); ++dir) {
		std::vector<std::string> subDirs =
				CFileHandler::SubDirs(*dir, pattern, SPRING_VFS_RAW);
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
	if (!CheckFile(file))
		return false;
	FixSlashes(file);
	return ::remove(file.c_str()) == 0;
}
