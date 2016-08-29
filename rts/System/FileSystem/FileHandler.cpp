/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "FileHandler.h"

#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <limits.h>
#include <boost/regex.hpp>

#include "FileQueryFlags.h"
#include "FileSystem.h"

#ifndef TOOLS
	#include "VFSHandler.h"
	#include "DataDirsAccess.h"
	#include "System/Util.h"
	#include "System/Platform/Misc.h"
#endif

using std::string;



/******************************************************************************/
/******************************************************************************/

CFileHandler::CFileHandler(const char* fileName, const char* modes)
	: filePos(0), fileSize(-1)
{
	Open(fileName, modes);
}


CFileHandler::CFileHandler(const string& fileName, const string& modes)
	: filePos(0), fileSize(-1)
{
	Open(fileName, modes);
}


CFileHandler::~CFileHandler()
{
	ifs.close();
}


/******************************************************************************/

bool CFileHandler::TryReadFromPWD(const string& fileName)
{
#ifndef TOOLS
	if (FileSystem::IsAbsolutePath(fileName))
		return false;
	const std::string fullpath(Platform::GetOrigCWD() + fileName);
#else
	const std::string fullpath(fileName);
#endif
	ifs.open(fullpath.c_str(), std::ios::in | std::ios::binary);
	if (ifs && !ifs.bad() && ifs.is_open()) {
		ifs.seekg(0, std::ios_base::end);
		fileSize = ifs.tellg();
		ifs.seekg(0, std::ios_base::beg);
		return true;
	}
	ifs.close();
	return false;
}


bool CFileHandler::TryReadFromRawFS(const string& fileName)
{
#ifndef TOOLS
	const string rawpath = dataDirsAccess.LocateFile(fileName);
	ifs.open(rawpath.c_str(), std::ios::in | std::ios::binary);
	if (ifs && !ifs.bad() && ifs.is_open()) {
		ifs.seekg(0, std::ios_base::end);
		fileSize = ifs.tellg();
		ifs.seekg(0, std::ios_base::beg);
		return true;
	}
#endif
	ifs.close();
	return false;
}


bool CFileHandler::TryReadFromVFS(const string& fileName, int section)
{
#ifndef TOOLS
	if (vfsHandler == NULL) {
		return false;
	}

	const string file = StringToLower(fileName);
	if (vfsHandler->LoadFile(file, fileBuffer, (CVFSHandler::Section) section)) {
		// did we allocated more mem than needed
		// (e.g. because of incorrect usage of std::vector)?
		assert(fileBuffer.size() == fileBuffer.capacity());

		fileSize = fileBuffer.size();
		return true;
	}
#endif
	return false;
}


void CFileHandler::Open(const string& fileName, const string& modes)
{
	this->fileName = fileName;
	for (char c: modes) {
		CVFSHandler::Section section = CVFSHandler::GetModeSection(c);
		if ((section != CVFSHandler::Section::Error) && TryReadFromVFS(fileName, section))
			break;

		if ((c == SPRING_VFS_RAW[0]) && TryReadFromRawFS(fileName))
			break;

		if ((c == SPRING_VFS_PWD[0]) && TryReadFromPWD(fileName))
			break;
	}
}


/******************************************************************************/

bool CFileHandler::FileExists(const std::string& filePath, const std::string& modes)
{
	for (char c: modes) {
#ifndef TOOLS
		CVFSHandler::Section section = CVFSHandler::GetModeSection(c);
		if ((section != CVFSHandler::Section::Error) && vfsHandler->FileExists(filePath, section))
			return true;

		if ((c == SPRING_VFS_RAW[0]) && FileSystem::FileExists(dataDirsAccess.LocateFile(filePath)))
			return true;
#endif
		if (c == SPRING_VFS_PWD[0]) {
#ifndef TOOLS
			if (!FileSystem::IsAbsolutePath(filePath)) {
				const std::string fullpath(Platform::GetOrigCWD() + filePath);
				if (FileSystem::FileExists(fullpath))
					return true;
			}
#else
			const std::string fullpath(filePath);
			if (FileSystem::FileExists(fullpath))
				return true;
#endif
		}
	}

	return false;
}


bool CFileHandler::FileExists() const
{
	return (fileSize >= 0);
}


int CFileHandler::Read(void* buf, int length)
{
	if (ifs.is_open()) {
		ifs.read(static_cast<char*>(buf), length);
		return ifs.gcount();
	}

	if (!fileBuffer.empty()) {
		if ((length + filePos) > fileSize)
			length = fileSize - filePos;

		if (length > 0) {
			assert(fileBuffer.size() >= (filePos + length));
			memcpy(buf, &fileBuffer[filePos], length);
			filePos += length;
		}
		return length;
	}

	return 0;
}


int CFileHandler::ReadString(void* buf, int length)
{
	assert(buf != nullptr);
	assert(length > 0);
	const int pos = GetPos();
	const int rlen = Read(buf, length);
	if (rlen < length) ((char*)buf)[rlen] = 0;
	const int slen = strlen((const char*)buf);
	if (rlen > 0) {
		const int send = pos + slen + 1;
		Seek(send, std::ios_base::beg);
	}
	return slen;
}


void CFileHandler::Seek(int length, std::ios_base::seekdir where)
{
	if (ifs.is_open())
	{
		// Status bits must be cleared before seeking, otherwise it might fail
		// in the common case of EOF
		// http://en.cppreference.com/w/cpp/io/basic_istream/seekg
		ifs.clear();
		ifs.seekg(length, where);
	}
	else if (!fileBuffer.empty())
	{
		if (where == std::ios_base::beg)
		{
			filePos = length;
		}
		else if (where == std::ios_base::cur)
		{
			filePos += length;
		}
		else if (where == std::ios_base::end)
		{
			filePos = fileSize + length;
		}
	}
}

bool CFileHandler::Eof() const
{
	if (ifs.is_open()) {
		return ifs.eof();
	}
	if (!fileBuffer.empty()) {
		return (filePos >= fileSize);
	}
	return true;
}


int CFileHandler::FileSize() const
{
	return fileSize;
}


int CFileHandler::GetPos()
{
	if (ifs.is_open()) {
		return ifs.tellg();
	} else {
		return filePos;
	}
}


bool CFileHandler::LoadStringData(string& data)
{
	if (!FileExists()) {
		return false;
	}
	char* buf = new char[fileSize];
	Read(buf, fileSize);
	data.append(buf, fileSize);
	delete[] buf;
	return true;
}

std::string CFileHandler::GetFileExt() const
{
	return FileSystem::GetExtension(fileName);
}

/******************************************************************************/

std::vector<string> CFileHandler::FindFiles(const string& path,
		const string& pattern)
{
#ifndef TOOLS
	return DirList(path, pattern, SPRING_VFS_ALL);
#else
	return std::vector<std::string>();
#endif
}


/******************************************************************************/

std::vector<string> CFileHandler::DirList(const string& path,
		const string& pattern, const string& modes)
{
	const string pat = pattern.empty() ? "*" : pattern;

	std::set<string> fileSet;
	for (char c: modes) {
		CVFSHandler::Section section = CVFSHandler::GetModeSection(c);
		if (section != CVFSHandler::Section::Error)
			InsertVFSFiles(fileSet, path, pat, section);

		if (c == SPRING_VFS_RAW[0])
			InsertRawFiles(fileSet, path, pat);
	}
	std::vector<string> fileVec;
	std::set<string>::const_iterator it;
	for (it = fileSet.begin(); it != fileSet.end(); ++it) {
		fileVec.push_back(*it);
	}
	return fileVec;
}


bool CFileHandler::InsertRawFiles(std::set<string>& fileSet,
		const string& path, const string& pattern)
{
#ifndef TOOLS
	const std::vector<string> &found = dataDirsAccess.FindFiles(path, pattern);
	std::vector<string>::const_iterator fi;
	for (fi = found.begin(); fi != found.end(); ++fi) {
		fileSet.insert(fi->c_str());
	}
#endif
	return true;
}


bool CFileHandler::InsertVFSFiles(std::set<string>& fileSet,
		const string& path, const string& pattern, int section)
{
#ifndef TOOLS
	if (!vfsHandler) {
		return false;
	}

	string prefix = path;
	if (path.find_last_of("\\/") != (path.size() - 1)) {
		prefix += '/';
	}

	boost::regex regexpattern(FileSystem::ConvertGlobToRegex(pattern),
			boost::regex::icase);

	const std::vector<string> &found = vfsHandler->GetFilesInDir(path, (CVFSHandler::Section) section);
	std::vector<string>::const_iterator fi;
	for (fi = found.begin(); fi != found.end(); ++fi) {
		if (boost::regex_match(*fi, regexpattern)) {
			fileSet.insert(prefix + *fi);
		}
	}
#endif
	return true;
}


/******************************************************************************/

std::vector<string> CFileHandler::SubDirs(const string& path,
		const string& pattern, const string& modes)
{
	const string pat = pattern.empty() ? "*" : pattern;

	std::set<string> dirSet;
	for (char c: modes) {
		CVFSHandler::Section section = CVFSHandler::GetModeSection(c);
		if (section != CVFSHandler::Section::Error)
			InsertVFSDirs(dirSet, path, pat, section);

		if (c == SPRING_VFS_RAW[0])
			InsertRawDirs(dirSet, path, pat);
	}
	std::vector<string> dirVec;
	std::set<string>::const_iterator it;
	for (it = dirSet.begin(); it != dirSet.end(); ++it) {
		dirVec.push_back(*it);
	}
	return dirVec;
}


bool CFileHandler::InsertRawDirs(std::set<string>& dirSet,
		const string& path, const string& pattern)
{
#ifndef TOOLS
	boost::regex regexpattern(FileSystem::ConvertGlobToRegex(pattern),
	                          boost::regex::icase);

	const std::vector<string> &found = dataDirsAccess.FindFiles(path, pattern,
	                                            FileQueryFlags::ONLY_DIRS);
	std::vector<string>::const_iterator fi;
	for (fi = found.begin(); fi != found.end(); ++fi) {
		const string& dir = *fi;
		if (boost::regex_match(dir, regexpattern)) {
			dirSet.insert(dir);
		}
	}
#endif
	return true;
}


bool CFileHandler::InsertVFSDirs(std::set<string>& dirSet,
		const string& path, const string& pattern, int section)
{
#ifndef TOOLS
	if (!vfsHandler) {
		return false;
	}

	string prefix = path;
	if (path.find_last_of("\\/") != (path.size() - 1)) {
		prefix += '/';
	}

	boost::regex regexpattern(FileSystem::ConvertGlobToRegex(pattern),
			boost::regex::icase);

	const std::vector<string> &found = vfsHandler->GetDirsInDir(path, (CVFSHandler::Section) section);
	std::vector<string>::const_iterator fi;
	for (fi = found.begin(); fi != found.end(); ++fi) {
		if (boost::regex_match(*fi, regexpattern)) {
			dirSet.insert(prefix + *fi);
		}
	}
#endif
	return true;
}


/******************************************************************************/

string CFileHandler::AllowModes(const string& modes, const string& allowed)
{
	string newModes;
	for (unsigned i = 0; i < modes.size(); i++) {
		if (allowed.find(modes[i]) != string::npos) {
			newModes += modes[i];
		}
	}
	return newModes;
}


string CFileHandler::ForbidModes(const string& modes, const string& forbidden)
{
	string newModes;
	for (unsigned i = 0; i < modes.size(); i++) {
		if (forbidden.find(modes[i]) == string::npos) {
			newModes += modes[i];
		}
	}
	return newModes;
}


/******************************************************************************/
/******************************************************************************/
