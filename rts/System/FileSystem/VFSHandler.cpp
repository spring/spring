#include "StdAfx.h"
#include "VFSHandler.h"
#include "ArchiveFactory.h"
#include "ArchiveBase.h"
#include "ArchiveDir.h" // for FileData::dynamic
#include <algorithm>
#include <set>
#include "Platform/FileSystem.h"
#include "mmgr.h"

CVFSHandler* hpiHandler=0;

CVFSHandler::CVFSHandler()
{
}

// Override determines whether if conflicts overwrites an existing entry in the virtual filesystem or not
bool CVFSHandler::AddArchive(std::string arName, bool override)
{
	CArchiveBase* ar = archives[arName];
	if (!ar) {
		ar = CArchiveFactory::OpenArchive(arName);
		if (!ar) {
			return false;
		}
		archives[arName] = ar;
	}

	int cur;
	std::string name;
	int size;

	for (cur = ar->FindFiles(0, &name, &size); cur != 0; cur = ar->FindFiles(cur, &name, &size)) {
		StringToLowerInPlace(name);
		if ((!override) && (files.find(name) != files.end())) {
			continue;
		}

		FileData d;
		d.ar = ar;
		d.size = size;
		d.dynamic = !!dynamic_cast<CArchiveDir*>(ar);
		files[name] = d;
	}
	return true;
}

CVFSHandler::~CVFSHandler(void)
{
	for (std::map<std::string, CArchiveBase*>::iterator i = archives.begin(); i != archives.end(); ++i) {
		delete i->second;
	}
}

int CVFSHandler::LoadFile(std::string name, void* buffer)
{
	StringToLowerInPlace(name);
	filesystem.ForwardSlashes(name);

	std::map<std::string, FileData>::iterator fi = files.find(name);
	if (fi == files.end()) {
		return -1;
	}
	FileData& fd = fi->second;

	int fh = fd.ar->OpenFile(name);
	if (!fh) {
		return -1;
	}
	const int fsize = fd.dynamic ? fd.ar->FileSize(fh) : fd.size;

	fd.ar->ReadFile(fh, buffer, fsize);
	fd.ar->CloseFile(fh);

	return fsize;
}

int CVFSHandler::GetFileSize(std::string name)
{
	StringToLowerInPlace(name);
	filesystem.ForwardSlashes(name);

	std::map<std::string, FileData>::iterator fi = files.find(name);
	if (fi == files.end()) {
		return -1;
	}
	FileData& fd = fi->second;

	if (!fd.dynamic) {
		return fd.size;
	}
	else {
		const int fh = fd.ar->OpenFile(name);
		if (fh == 0) {
			return -1;
		} else {
			const int fsize = fd.ar->FileSize(fh);
			fd.ar->CloseFile(fh);
			return fsize;
		}
	}
	return -1;
}

// Returns all the files in the given (virtual) directory without the preceeding pathname
std::vector<std::string> CVFSHandler::GetFilesInDir(std::string dir)
{
	std::vector<std::string> ret;
	StringToLowerInPlace(dir);
	filesystem.ForwardSlashes(dir);

	std::map<std::string, FileData>::const_iterator filesStart = files.begin();
	std::map<std::string, FileData>::const_iterator filesEnd   = files.end();

	// Non-empty directories to look in should have a trailing backslash
	if (!dir.empty()) {
		std::string::size_type dirLast = (dir.length() - 1);
		if (dir[dirLast] != '/') {
			dir += "/";
			dirLast++;
		}
		// limit the iterator range
		std::string dirEnd = dir;
		dirEnd[dirLast] = dirEnd[dirLast] + 1;
		filesStart = files.lower_bound(dir);
		filesEnd   = files.upper_bound(dirEnd);
	}

	while (filesStart != filesEnd) {
		const std::string path = filesystem.GetDirectory(filesStart->first);

		// Test to see if this file start with the dir path
		if (path.compare(0, dir.length(), dir) == 0) {

			// Strip pathname
			const std::string name = filesStart->first.substr(dir.length());

			// Do not return files in subfolders
			if ((name.find('/') == std::string::npos) &&
			    (name.find('\\') == std::string::npos)) {
				ret.push_back(name);
			}
		}
		filesStart++;
	}

	return ret;
}


// Returns all the sub-directories in the given (virtual) directory without the preceeding pathname
std::vector<std::string> CVFSHandler::GetDirsInDir(std::string dir)
{
	std::vector<std::string> ret;
	StringToLowerInPlace(dir);
	filesystem.ForwardSlashes(dir);

	std::map<std::string, FileData>::const_iterator filesStart = files.begin();
	std::map<std::string, FileData>::const_iterator filesEnd   = files.end();

	// Non-empty directories to look in should have a trailing backslash
	if (!dir.empty()) {
		std::string::size_type dirLast = (dir.length() - 1);
		if (dir[dirLast] != '/') {
			dir += "/";
			dirLast++;
		}
		// limit the iterator range
		std::string dirEnd = dir;
		dirEnd[dirLast] = dirEnd[dirLast] + 1;
		filesStart = files.lower_bound(dir);
		filesEnd   = files.upper_bound(dirEnd);
	}

	std::set<std::string> dirs;

	while (filesStart != filesEnd) {
		const std::string path = filesystem.GetDirectory(filesStart->first);
		// Test to see if this file start with the dir path
		if (path.compare(0, dir.length(), dir) == 0) {
			// Strip pathname
			const std::string name = filesStart->first.substr(dir.length());
			const std::string::size_type slash = name.find_first_of("/\\");
			if (slash != std::string::npos) {
				dirs.insert(name.substr(0, slash + 1));
			}
		}
		filesStart++;
	}

	for (std::set<std::string>::const_iterator it = dirs.begin(); it != dirs.end(); ++it) {
		ret.push_back(*it);
	}

	return ret;
}
