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
#include <string.h>
#include <boost/regex.hpp>

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
	struct stat info;
	const int statfailed = stat(file.c_str(), &info);
	if (statfailed) {
		LOG_L(L_WARNING, "Failed fetching last modification time from file: %s", file.c_str());
		LOG_L(L_WARNING, "Error is: \"%s\"", strerror(errno));
		return "";
	}

	const std::time_t t = info.st_mtime;
	struct tm* clock = std::gmtime(&t);

	const size_t cTime_size = 20;
	char cTime[cTime_size];
	SNPRINTF(cTime, cTime_size, "%d%02d%02d%02d%02d%02d", 1900+clock->tm_year, clock->tm_mon, clock->tm_mday, clock->tm_hour, clock->tm_min, clock->tm_sec);
	return cTime;
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
	// on windows this func doesn't allow deletion of dirs
	// but no such usage exist in spring at the moment
	int ret = remove(file.c_str());
	if (ret) {
		LOG_L(L_WARNING, "Could not delete file %s: %s", file.c_str(), strerror(errno));
		return false;
	}

	return true;
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
	struct stat info;
	const int ret = stat(dir.c_str(), &info);
	bool dirExists = ((ret == 0 && S_ISDIR(info.st_mode)));
	return dirExists;
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

#ifndef WIN32
	struct stat info1, info2;
	const int ret1 = stat(path1.c_str(), &info1);
	const int ret2 = stat(path2.c_str(), &info2);

	// If either files doesn't exist, return false
	if (ret1 || ret2)
		return false;

	return (info1.st_dev == info2.st_dev) && (info1.st_ino == info2.st_ino);
#else
	HANDLE h1 = CreateFile(
		path1.c_str(),
		0,
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
		0,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		0);

	if (h1 == INVALID_HANDLE_VALUE)
		return false;

	HANDLE h2 = CreateFile(
		path2.c_str(),
		0,
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
		0,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		0);

	if (h2 == INVALID_HANDLE_VALUE)
		return false;

	BY_HANDLE_FILE_INFORMATION info1, info2;
	if (!GetFileInformationByHandle(h1, &info1))
		return false;

	if (!GetFileInformationByHandle(h2, &info2))
		return false;

	return
		info1.dwVolumeSerialNumber == info2.dwVolumeSerialNumber
		&& info1.nFileIndexHigh == info2.nFileIndexHigh
		&& info1.nFileIndexLow == info2.nFileIndexLow
		&& info1.nFileSizeHigh == info2.nFileSizeHigh
		&& info1.nFileSizeLow == info2.nFileSizeLow
		&& info1.ftLastWriteTime.dwLowDateTime
		== info2.ftLastWriteTime.dwLowDateTime
		&& info1.ftLastWriteTime.dwHighDateTime
		== info2.ftLastWriteTime.dwHighDateTime;
#endif
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

static void FindFiles(std::vector<std::string>& matches, const std::string& datadir, const std::string& dir, const boost::regex& regexPattern, int flags)
{
#ifdef _WIN32
	WIN32_FIND_DATA wfd;
	HANDLE hFind = FindFirstFile((datadir + dir + "\\*").c_str(), &wfd);

	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if(strcmp(wfd.cFileName,".") && strcmp(wfd.cFileName ,"..")) {
				if(!(wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)) {
					if ((flags & FileQueryFlags::ONLY_DIRS) == 0) {
						if (boost::regex_match(wfd.cFileName, regexPattern)) {
							matches.push_back(dir + wfd.cFileName);
						}
					}
				} else {
					if (flags & FileQueryFlags::INCLUDE_DIRS) {
						if (boost::regex_match(wfd.cFileName, regexPattern)) {
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
						if (boost::regex_match(ep->d_name, regexPattern)) {
							matches.push_back(dir + ep->d_name);
						}
					}
				} else {
					// or a directory?
					if (flags & FileQueryFlags::INCLUDE_DIRS) {
						if (boost::regex_match(ep->d_name, regexPattern)) {
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
	const boost::regex regexPattern(regex);
	::FindFiles(matches, dataDir, dir, regexPattern, flags);
}

