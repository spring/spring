/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FileSystemHandler.h"

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
	// winapi redifines these which breaks things
	#if defined(CreateDirectory)
		#undef CreateDirectory
	#endif
	#if defined(DeleteFile)
		#undef DeleteFile
	#endif
#endif

#include "Util.h"
#include "LogOutput.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/VFSHandler.h"
#include "Exceptions.h"
#include "FileSystem.h"

std::string StripTrailingSlashes(std::string path)
{
	while (!path.empty() && (path.at(path.length()-1) == '\\' || path.at(path.length()-1) == '/')) {
		path = path.substr(0, path.length()-1);
	}
	return path;
}

/**
 * @brief The global data directory handler instance
 */
FileSystemHandler* FileSystemHandler::instance = NULL;

/**
 * @brief get the data directory handler instance
 */
FileSystemHandler& FileSystemHandler::GetInstance()
{
	if (!instance) {
		Initialize(false);
	}

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
			// TODO: move these two out of here; this class is for low level file system abstraction only
			archiveScanner = new CArchiveScanner();
			vfsHandler = new CVFSHandler();
		} catch (...) {
			FileSystemHandler::Cleanup();
			throw;
		}
	}
}

void FileSystemHandler::Cleanup()
{
	FileSystemHandler* tmp = instance;
	instance = NULL;
	delete tmp;
}

FileSystemHandler::~FileSystemHandler()
{
	SafeDelete(archiveScanner);
	SafeDelete(vfsHandler);
	// configHandler->Deallocate();
}

#ifndef _WIN32
const int FileSystemHandler::native_path_separator = '/';
#else
const int FileSystemHandler::native_path_separator = '\\';
#endif

FileSystemHandler::FileSystemHandler()
{
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
	bool isRootFs = false;

#ifdef WIN32
	isRootFs = (p.length() >= 2 && p[1] == ':' && ((p[0] >= 'a' && p[0] <= 'z') || (p[0] >= 'A' && p[0] <= 'Z')) && (p.length() == 2 || (p.length() == 3 && (p[2] == '\\' || p[2] == '/'))));
#else
	isRootFs = (p.length() == 1 && p[0] == '/');
#endif

	return isRootFs;
}

// sPS/cPS (depending on OS): "\\" & '\\' or "/" & '/'
bool FileSystemHandler::HasPathSepAtEnd(const std::string& path) {

	bool pathSepAtEnd = false;

	if (!path.empty() && (path[path.size() - 1] == cPS)) {
		pathSepAtEnd = true;
	}

	return pathSepAtEnd;
}
void FileSystemHandler::EnsurePathSepAtEnd(std::string& path) {

	if (path.empty()) {
		path += "."sPS;
	} else if (path[path.size() - 1] != cPS) {
		path += cPS;
	}
}

size_t FileSystemHandler::GetFileSize(const std::string& file)
{
	size_t fileSize = 0;

	struct stat info;
	if (stat(file.c_str(), &info) == 0) {
		fileSize = info.st_size;
	}

	return fileSize;
}

std::vector<std::string> FileSystemHandler::FindFiles(const std::string& dir, const std::string& pattern, int flags) const
{
	std::vector<std::string> matches;

	// if it's an absolute path, don't look for it in the data directories
	if (FileSystemHandler::IsAbsolutePath(dir)) {
		// pass the directory as second directory argument, so the path gets included in the matches.
		FindFilesSingleDir(matches, "", dir, pattern, flags);
		return matches;
	}

	std::string dir2 = RemoveLocalPathPrefix(dir);

	const std::vector<DataDir>& datadirs = locater.GetDataDirs();
	for (std::vector<DataDir>::const_reverse_iterator d = datadirs.rbegin(); d != datadirs.rend(); ++d) {
		FindFilesSingleDir(matches, d->path, dir2, pattern, flags);
	}
	return matches;
}

bool FileSystemHandler::IsReadableFile(const std::string& file)
{
#ifdef WIN32
	return (_access(StripTrailingSlashes(file).c_str(), 4) == 0);
#else
	return (access(file.c_str(), R_OK | F_OK) == 0);
#endif
}

std::string FileSystemHandler::GetFileModificationDate(const std::string& file)
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
		logOutput.Print("WARNING: Failed opening file for retreiving last modification time: %s", file.c_str());
	} else {
		FILETIME /*ftCreate, ftAccess,*/ ftWrite;

		// Retrieve the file times for the file.
		if (GetFileTime(hFile, NULL, NULL, &ftWrite) != 0) {
			logOutput.Print("WARNING: Failed fetching last modification time from file: %s", file.c_str());
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
				logOutput.Print("WARNING: Failed converting last modification time to a string");
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
		logOutput.Print("WARNING: Failed opening file for retreiving last modification time: %s", file.c_str());
	} else {
		// Get the last modified time and put it into the time structure
		clock = gmtime(&(attrib.st_mtime));
		if (clock == NULL) {
			logOutput.Print("WARNING: Failed fetching last modification time from file: %s", file.c_str());
		} else {
			SNPRINTF(cTime, cTime_size, "%d%02d%02d%02d%02d%02d", 1900+clock->tm_year, clock->tm_mon, clock->tm_mday, clock->tm_hour, clock->tm_min, clock->tm_sec);
			time = cTime;
		}
	}
#endif // defined(WIN32)

	return time;
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
	std::vector<std::string> dataDirPaths;

	const std::vector<DataDir>& datadirs = locater.GetDataDirs();
	for (std::vector<DataDir>::const_iterator d = datadirs.begin(); d != datadirs.end(); ++d) {
		dataDirPaths.push_back(d->path);
	}

	return dataDirPaths;
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
bool FileSystemHandler::mkdir(const std::string& dir)
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
		logOutput.Print("Could not create directory %s: %s", dir.c_str(), strerror(errno));
	}

	return dirCreated;
}

bool FileSystemHandler::DeleteFile(const std::string& file)
{
	bool fileDeleted = (remove(file.c_str()) == 0);

	if (!fileDeleted) {
		logOutput.Print("Could not delete file %s: %s", file.c_str(), strerror(errno));
	}

	return fileDeleted;
}

bool FileSystemHandler::FileExists(const std::string& file)
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

bool FileSystemHandler::DirExists(const std::string& dir)
{
	bool dirExists = false;

#ifdef _WIN32
	struct _stat info;
	std::string myDir = dir;
	// for the root dir on a drive (for example C:\)
	// we need the trailing slash
	if ((myDir.length() == 3) && (myDir[1] == ':') && ((myDir[2] != '\\') || (myDir[2] != '/'))) {
		// do nothing
	} else if ((myDir.length() == 2) && (myDir[1] == ':')) {
		myDir += "\\";
	} else {
		// for any other dir (for example C:\WINDOWS\),
		// we need to get rid of it.
		myDir = StripTrailingSlashes(myDir);
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


bool FileSystemHandler::DirIsWritable(const std::string& dir)
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

std::string FileSystemHandler::GetCwd()
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

void FileSystemHandler::Chdir(const std::string& dir)
{
#ifndef _WIN32
	const int err = chdir(dir.c_str());
#else
	const int err = _chdir(StripTrailingSlashes(dir).c_str());
#endif

	if (err) {
		throw content_error("Could not chdir into SPRING_DATADIR");
	}
}

static void FindFiles(std::vector<std::string>& matches, const std::string& datadir, const std::string& dir, const boost::regex &regexpattern, int flags)
{
#ifdef _WIN32
	WIN32_FIND_DATA wfd;
	HANDLE hFind = FindFirstFile((datadir + dir + "\\*").c_str(), &wfd);

	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if(strcmp(wfd.cFileName,".") && strcmp(wfd.cFileName ,"..")) {
				if(!(wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)) {
					if ((flags & FileSystem::ONLY_DIRS) == 0) {
						if (boost::regex_match(wfd.cFileName, regexpattern)) {
							matches.push_back(dir + wfd.cFileName);
						}
					}
				} else {
					if (flags & FileSystem::INCLUDE_DIRS) {
						if (boost::regex_match(wfd.cFileName, regexpattern)) {
							matches.push_back(dir + wfd.cFileName + "\\");
						}
					}
					if (flags & FileSystem::RECURSE) {
						FindFiles(matches, datadir, dir + wfd.cFileName + "\\", regexpattern, flags);
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
					if ((flags & FileSystem::ONLY_DIRS) == 0) {
						if (boost::regex_match(ep->d_name, regexpattern)) {
							matches.push_back(dir + ep->d_name);
						}
					}
				} else {
					// or a directory?
					if (flags & FileSystem::INCLUDE_DIRS) {
						if (boost::regex_match(ep->d_name, regexpattern)) {
							matches.push_back(dir + ep->d_name + "/");
						}
					}
					if (flags & FileSystem::RECURSE) {
						FindFiles(matches, datadir, dir + ep->d_name + "/", regexpattern, flags);
					}
				}
			}
		}
	}
	closedir(dp);
#endif
}


void FileSystemHandler::FindFilesSingleDir(std::vector<std::string>& matches, const std::string& datadir, const std::string& dir, const std::string &pattern, int flags) const
{
	assert(datadir.empty() || datadir[datadir.length() - 1] == native_path_separator);
	assert(!dir.empty() && dir[dir.length() - 1] == native_path_separator);

	boost::regex regexpattern(filesystem.glob_to_regex(pattern));

	::FindFiles(matches, datadir, dir, regexpattern, flags);
}
