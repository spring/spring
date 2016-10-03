/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#if defined(_MSC_VER) && !defined(S_ISDIR)
#	define S_ISDIR(m) (((m) & 0170000) == 0040000)
#endif

#include "FileSystemAbstraction.h"

#include "FileQueryFlags.h"

#include "System/Util.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"

#include <cassert>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <cstring>
#include <regex>
#include <boost/filesystem.hpp>

#ifndef _WIN32
	#include <dirent.h>
	#include <sstream>
	#include <unistd.h>
	#include <time.h>
#else
	#include <windows.h>
	#include <io.h>
	#include <direct.h>
	#include <fstream>
	// Win-API redifines these, which breaks things
	#if defined(CreateDirectory)
		#undef CreateDirectory
	#endif
	#if defined(DeleteFile)
		#undef DeleteFile
	#endif
#endif



std::string FileSystemAbstraction::RemoveLocalPathPrefix(const std::string& path)
{
	std::string p(path);

	if ((p.length() >= 2) && (p[0] == '.') && IsPathSeparator(p[1])) {
	    p.erase(0, 2);
	}

	return p;
}

bool FileSystemAbstraction::IsFSRoot(const std::string& p)
{

#ifdef WIN32
	// examples: "C:\", "C:/", "C:", "c:", "D:"
	bool isFsRoot = (p.length() >= 2 && p[1] == ':' &&
			((p[0] >= 'a' && p[0] <= 'z') || (p[0] >= 'A' && p[0] <= 'Z')) &&
			(p.length() == 2 || (p.length() == 3 && IsPathSeparator(p[2]))));
#else
	// examples: "/"
	bool isFsRoot = (p.length() == 1 && IsNativePathSeparator(p[0]));
#endif

	return isFsRoot;
}

bool FileSystemAbstraction::IsPathSeparator(char aChar) {
	return ((aChar == cPS_WIN32) || (aChar == cPS_POSIX));
}

bool FileSystemAbstraction::IsNativePathSeparator(char aChar) {
	return (aChar == cPS);
}

bool FileSystemAbstraction::HasPathSepAtEnd(const std::string& path) {

	bool pathSepAtEnd = false;

	if (!path.empty()) {
		pathSepAtEnd = IsNativePathSeparator(path.at(path.size() - 1));
	}

	return pathSepAtEnd;
}

void FileSystemAbstraction::EnsurePathSepAtEnd(std::string& path) {

	if (path.empty()) {
		path += "." sPS;
	} else if (!HasPathSepAtEnd(path)) {
		path += cPS;
	}
}
std::string FileSystemAbstraction::EnsurePathSepAtEnd(const std::string& path) {

	std::string pathCopy(path);
	EnsurePathSepAtEnd(pathCopy);
	return pathCopy;
}

void FileSystemAbstraction::EnsureNoPathSepAtEnd(std::string& path) {

	if (HasPathSepAtEnd(path)) {
		path.resize(path.size() - 1);
	}
}
std::string FileSystemAbstraction::EnsureNoPathSepAtEnd(const std::string& path) {

	std::string pathCopy(path);
	EnsureNoPathSepAtEnd(pathCopy);
	return pathCopy;
}

std::string FileSystemAbstraction::StripTrailingSlashes(const std::string& path)
{
	size_t len = path.length();

	while (len > 0) {
		if (IsPathSeparator(path.at(len - 1))) {
			--len;
		} else {
			break;
		}
	}

	return path.substr(0, len);
}

std::string FileSystemAbstraction::GetParent(const std::string& path)
{
	//TODO uncomment and test if it breaks other code
	/*try {
		const boost::filesystem::path p(path);
		return p.parent_path.string();
	} catch (const boost::filesystem::filesystem_error& ex) {
		return "";
	}*/

	std::string parent = path;
	EnsureNoPathSepAtEnd(parent);

	static const char* PATH_SEP_REGEX = sPS_POSIX sPS_WIN32;
	const std::string::size_type slashPos = parent.find_last_of(PATH_SEP_REGEX);
	if (slashPos == std::string::npos) {
		parent = "";
	} else {
		parent.resize(slashPos + 1);
	}

	return parent;
}

size_t FileSystemAbstraction::GetFileSize(const std::string& file)
{
	size_t fileSize = -1;

	struct stat info;
	if ((stat(file.c_str(), &info) == 0) && (!S_ISDIR(info.st_mode))) {
		fileSize = info.st_size;
	}

	return fileSize;
}

bool FileSystemAbstraction::IsReadableFile(const std::string& file)
{
	// Exclude directories!
	if (!FileExists(file)) {
		return false;
	}

#ifdef WIN32
	return (_access(StripTrailingSlashes(file).c_str(), 4) == 0);
#else
	return (access(file.c_str(), R_OK | F_OK) == 0);
#endif
}

std::string FileSystemAbstraction::GetFileModificationDate(const std::string& file)
{
	try {
		const boost::filesystem::path f(file);
		const std::time_t t = boost::filesystem::last_write_time(f);
		struct tm* clock = std::gmtime(&t);

		const size_t cTime_size = 20;
		char cTime[cTime_size];
		SNPRINTF(cTime, cTime_size, "%d%02d%02d%02d%02d%02d", 1900+clock->tm_year, clock->tm_mon, clock->tm_mday, clock->tm_hour, clock->tm_min, clock->tm_sec);
		return cTime;
	} catch (const boost::filesystem::filesystem_error& ex) {
		LOG_L(L_WARNING, "Failed fetching last modification time from file: %s", file.c_str());
		LOG_L(L_WARNING, "Error is: \"%s\"", ex.what());
		return "";
	}
}


char FileSystemAbstraction::GetNativePathSeparator()
{
	#ifndef _WIN32
	return '/';
	#else
	return '\\';
	#endif
}

bool FileSystemAbstraction::IsAbsolutePath(const std::string& path)
{
	//TODO uncomment this and test if there are conflicts in the code when this returns true but other custom code doesn't (e.g. with IsFSRoot)
	//const boost::filesystem::path f(file);
	//return f.is_absolute();

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
bool FileSystemAbstraction::MkDir(const std::string& dir)
{
	// First check if directory exists. We'll return success if it does.
	if (DirExists(dir)) {
		return true;
	}


	// If it doesn't exist we try to mkdir it and return success if that succeeds.
#ifndef _WIN32
	bool dirCreated = (::mkdir(dir.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0);
#else
	bool dirCreated = (::_mkdir(StripTrailingSlashes(dir).c_str()) == 0);
#endif

	if (!dirCreated) {
		LOG_L(L_WARNING, "Could not create directory %s: %s", dir.c_str(), strerror(errno));
	}

	return dirCreated;
}

bool FileSystemAbstraction::DeleteFile(const std::string& file)
{
	try {
		const boost::filesystem::path f(file);
		// same as posix remove(), but on windows that func doesn't allow deletion of dirs
		// so it's easier to use boost from the start
		return (boost::filesystem::remove_all(f) >= 1);
	} catch (const boost::filesystem::filesystem_error& ex) {
		LOG_L(L_WARNING, "Could not delete file %s: %s", file.c_str(), ex.what());
		return false;
	}
}

bool FileSystemAbstraction::FileExists(const std::string& file)
{
	struct stat info;
	const int ret = stat(file.c_str(), &info);
	bool fileExists = ((ret == 0 && !S_ISDIR(info.st_mode)));
	return fileExists;
}

bool FileSystemAbstraction::DirExists(const std::string& dir)
{
	try {
		const boost::filesystem::path p(dir);
		return boost::filesystem::exists(p) && boost::filesystem::is_directory(p);
	} catch (const boost::filesystem::filesystem_error& ex) {
		return false;
	}
}


bool FileSystemAbstraction::DirIsWritable(const std::string& dir)
{
#ifdef _WIN32
	// this exists because _access does not do the right thing
	// see http://msdn.microsoft.com/en-us/library/1w06ktdy(VS.71).aspx
	// for now, try to create a temporary file in a directory and open it
	// to rule out the possibility of it being created in the virtual store
	// TODO perhaps use SECURITY_DESCRIPTOR winapi calls here

	std::string testfile = dir + "\\__$testfile42$.test";
	std::ofstream os(testfile.c_str());
	if (os.fail()) {
		return false;
	}
	const char* testdata = "THIS IS A TEST";
	os << testdata;
	os.close();

	// this part should only be needed when there is no manifest embedded
	std::ifstream is(testfile.c_str());
	if (is.fail()) {
		return false; // the file most likely exists in the virtual store
	}
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


bool FileSystemAbstraction::ComparePaths(const std::string& path1, const std::string& path2)
{
	try {
		const boost::filesystem::path p1(path1);
		const boost::filesystem::path p2(path2);
		return boost::filesystem::equivalent(p1, p2);
	} catch (const boost::filesystem::filesystem_error& ex) {
		return false;
	}
}


std::string FileSystemAbstraction::GetCwd()
{
	std::string cwd = "";

#ifndef _WIN32
	#define GETCWD getcwd
#else
	#define GETCWD _getcwd
#endif

	const size_t path_maxSize = 1024;
	char path[path_maxSize];
	if (GETCWD(path, path_maxSize) != NULL) {
		cwd = path;
	}

	return cwd;
}

void FileSystemAbstraction::ChDir(const std::string& dir)
{
#ifndef _WIN32
	const int err = chdir(dir.c_str());
#else
	const int err = _chdir(StripTrailingSlashes(dir).c_str());
#endif

	if (err) {
		throw content_error("Could not chdir into " + dir);
	}
}

static void FindFiles(std::vector<std::string>& matches, const std::string& datadir, const std::string& dir, const std::regex& regexPattern, int flags)
{
#ifdef _WIN32
	WIN32_FIND_DATA wfd;
	HANDLE hFind = FindFirstFile((datadir + dir + "\\*").c_str(), &wfd);

	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if(strcmp(wfd.cFileName,".") && strcmp(wfd.cFileName ,"..")) {
				if(!(wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)) {
					if ((flags & FileQueryFlags::ONLY_DIRS) == 0) {
						if (std::regex_match(wfd.cFileName, regexPattern)) {
							matches.push_back(dir + wfd.cFileName);
						}
					}
				} else {
					if (flags & FileQueryFlags::INCLUDE_DIRS) {
						if (std::regex_match(wfd.cFileName, regexPattern)) {
							matches.push_back(dir + wfd.cFileName + "\\");
						}
					}
					if (flags & FileQueryFlags::RECURSE) {
						FindFiles(matches, datadir, dir + wfd.cFileName + "\\", regexPattern, flags);
					}
				}
			}
		} while (FindNextFile(hFind, &wfd));
		FindClose(hFind);
	}
#else
	DIR* dp;
	struct dirent* ep;

	if (!(dp = opendir((datadir + dir).c_str()))) {
		return;
	}

	while ((ep = readdir(dp))) {
		// exclude hidden files
		if (ep->d_name[0] != '.') {
			// is it a file? (we just treat sockets / pipes / fifos / character&block devices as files...)
			// (need to stat because d_type is DT_UNKNOWN on linux :-/)
			struct stat info;
			if (stat((datadir + dir + ep->d_name).c_str(), &info) == 0) {
				if (!S_ISDIR(info.st_mode)) {
					if ((flags & FileQueryFlags::ONLY_DIRS) == 0) {
						if (std::regex_match(ep->d_name, regexPattern)) {
							matches.push_back(dir + ep->d_name);
						}
					}
				} else {
					// or a directory?
					if (flags & FileQueryFlags::INCLUDE_DIRS) {
						if (std::regex_match(ep->d_name, regexPattern)) {
							matches.push_back(dir + ep->d_name + "/");
						}
					}
					if (flags & FileQueryFlags::RECURSE) {
						FindFiles(matches, datadir, dir + ep->d_name + "/", regexPattern, flags);
					}
				}
			}
		}
	}
	closedir(dp);
#endif
}

void FileSystemAbstraction::FindFiles(std::vector<std::string>& matches, const std::string& dataDir, const std::string& dir, const std::string& regex, int flags)
{
	const std::regex regexPattern(regex);
	::FindFiles(matches, dataDir, dir, regexPattern, flags);
}

