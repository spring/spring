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

	#include <sys/types.h>
	#include <sys/stat.h>
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
	bool isFsRoot = false;

#ifdef WIN32
	// examples: "C:\", "C:/", "C:", "c:", "D:"
	isFsRoot = (p.length() >= 2 && p[1] == ':' &&
			((p[0] >= 'a' && p[0] <= 'z') || (p[0] >= 'A' && p[0] <= 'Z')) &&
			(p.length() == 2 || (p.length() == 3 && IsPathSeparator(p[2]))));
#else
	// examples: "/"
	isFsRoot = (p.length() == 1 && IsNativePathSeparator(p[0]));
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
		path += "."sPS;
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

std::string FileSystemAbstraction::GetParent(const std::string& path) {

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
	std::string time = "";

#if       defined(WIN32)
	HANDLE hFile = CreateFile(file.c_str(), // file to open
			GENERIC_READ,                   // open for reading
			FILE_SHARE_READ,                // share for reading
			NULL,                           // default security
			OPEN_EXISTING,                  // existing file only
			FILE_ATTRIBUTE_NORMAL,          // normal file
			NULL);                          // no attr. template

	if (hFile == INVALID_HANDLE_VALUE) {
		LOG_L(L_WARNING, "Failed opening file for retreiving last modification time: %s", file.c_str());
	} else {
		FILETIME /*ftCreate, ftAccess,*/ ftWrite;

		// Retrieve the file times for the file.
		if (GetFileTime(hFile, NULL, NULL, &ftWrite) != 0) {
			LOG_L(L_WARNING, "Failed fetching last modification time from file: %s", file.c_str());
		} else {
			// Convert the last-write time to local time.
			const size_t cTime_size = 20;
			char cTime[cTime_size];

			SYSTEMTIME stUTC, stLocal;
			if ((FileTimeToSystemTime(&ftWrite, &stUTC) != 0) &&
				SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal))
			{
				// Build a string showing the date and time.
				SNPRINTF(cTime, cTime_size,
						"%d%02d%02d%02d%02d%02d",
						stLocal.wYear, stLocal.wMonth, stLocal.wDay,
						stLocal.wHour, stLocal.wMinute, stLocal.wSecond);
				time = cTime;
			} else {
				LOG_L(L_WARNING, "Failed converting last modification time to a string");
			}
		}
		CloseHandle(hFile);
	}

#else  // defined(WIN32)
	struct tm* clock;
	struct stat attrib;
	const size_t cTime_size = 20;
	char cTime[cTime_size];

	const int fetchOk = stat(file.c_str(), &attrib); // get the attributes of file
	if (fetchOk != 0) {
		LOG_L(L_WARNING, "Failed opening file for retrieving last modification time: %s", file.c_str());
	} else {
		// Get the last modified time and put it into the time structure
		clock = gmtime(&(attrib.st_mtime));
		if (clock == NULL) {
			LOG_L(L_WARNING, "Failed fetching last modification time from file: %s", file.c_str());
		} else {
			SNPRINTF(cTime, cTime_size, "%d%02d%02d%02d%02d%02d", 1900+clock->tm_year, clock->tm_mon, clock->tm_mday, clock->tm_hour, clock->tm_min, clock->tm_sec);
			time = cTime;
		}
	}
#endif // defined(WIN32)

	return time;
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

	bool dirCreated = false;

	// If it doesn't exist we try to mkdir it and return success if that succeeds.
#ifndef _WIN32
	dirCreated = (::mkdir(dir.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0);
#else
	dirCreated = (::_mkdir(StripTrailingSlashes(dir).c_str()) == 0);
#endif

	if (!dirCreated) {
		LOG_L(L_WARNING, "Could not create directory %s: %s", dir.c_str(), strerror(errno));
	}

	return dirCreated;
}

bool FileSystemAbstraction::DeleteFile(const std::string& file)
{
	bool fileDeleted = (remove(file.c_str()) == 0);

	if (!fileDeleted) {
		LOG_L(L_WARNING, "Could not delete file %s: %s", file.c_str(), strerror(errno));
	}

	return fileDeleted;
}

bool FileSystemAbstraction::FileExists(const std::string& file)
{
	bool fileExists = false;

#ifdef _WIN32
	struct _stat info;
	const int ret = _stat(StripTrailingSlashes(file).c_str(), &info);
	fileExists = ((ret == 0 && (info.st_mode & _S_IFREG)));
#else
	struct stat info;
	const int ret = stat(file.c_str(), &info);
	fileExists = ((ret == 0 && !S_ISDIR(info.st_mode)));
#endif

	return fileExists;
}

bool FileSystemAbstraction::DirExists(const std::string& dir)
{
	bool dirExists = false;

#ifdef _WIN32
	struct _stat info;
	std::string myDir = dir;
	// only for the root dir on a drive (for example C:\)
	// we need the trailing slash
	if (!IsFSRoot(myDir)) {
		myDir = StripTrailingSlashes(myDir);
	} else if ((myDir.length() == 2) && (myDir[1] == ':')) {
		myDir += "\\";
	}
	const int ret = _stat(myDir.c_str(), &info);
	dirExists = ((ret == 0) && (info.st_mode & _S_IFDIR));
#else
	struct stat info;
	const int ret = stat(dir.c_str(), &info);
	dirExists = ((ret == 0) && S_ISDIR(info.st_mode));
#endif

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
#ifdef _WIN32
	bool ret = false;

	HANDLE fh1 = CreateFile(path1.c_str(), // file to open
			GENERIC_READ,                   // open for reading
			FILE_SHARE_READ,                // share for reading
			NULL,                           // default security
			OPEN_EXISTING,                  // existing file only
			FILE_ATTRIBUTE_NORMAL,          // normal file
			NULL);                          // no attr. template

	HANDLE fh2 = CreateFile(path2.c_str(), // file to open
			GENERIC_READ,                   // open for reading
			FILE_SHARE_READ,                // share for reading
			NULL,                           // default security
			OPEN_EXISTING,                  // existing file only
			FILE_ATTRIBUTE_NORMAL,          // normal file
			NULL);                          // no attr. template

	if ((fh1 != INVALID_HANDLE_VALUE) && (fh2 != INVALID_HANDLE_VALUE)) {
		BY_HANDLE_FILE_INFORMATION info1, info2;

		BOOL fine;
		fine  = GetFileInformationByHandle(fh1, &info1);
		fine = GetFileInformationByHandle(fh2, &info2) && fine;

		if (fine) {
			ret =
				   (info1.nFileIndexLow == info2.nFileIndexLow)
				&& (info1.nFileIndexHigh == info2.nFileIndexHigh)
				&& (info1.dwVolumeSerialNumber == info2.dwVolumeSerialNumber);
		} else {
			//GetLastError()
		}
	}

	CloseHandle(fh1);
	CloseHandle(fh2);

	return ret;
#else
	int r = 0;
	struct stat s1, s2;
	r  = stat(path1.c_str(), &s1);
	r |= stat(path2.c_str(), &s2);

	if (r != 0) {
		return false;
	}

	return (s1.st_ino == s2.st_ino) && (s1.st_dev == s2.st_dev);
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

