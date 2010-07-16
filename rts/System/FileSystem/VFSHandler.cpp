/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "VFSHandler.h"

#include <algorithm>
#include <set>
#include <cstring>

#include "ArchiveFactory.h"
#include "ArchiveBase.h"
#include "ArchiveDir.h" // for FileData::dynamic
#include "LogOutput.h"
#include "FileSystem.h"
#include "ArchiveScanner.h"
#include "Exceptions.h"
#include "Util.h"


static CLogSubsystem LOG_VFS("VFS");
static CLogSubsystem LOG_VFS_DETAIL("VFS-detail");


CVFSHandler* vfsHandler = NULL;


CVFSHandler::CVFSHandler()
{
	logOutput.Print(LOG_VFS, "CVFSHandler::CVFSHandler()");
}


bool CVFSHandler::AddArchive(const std::string& arName, bool override, const std::string& type)
{
	logOutput.Print(LOG_VFS, "AddArchive(arName = \"%s\", override = %s, type = \"%s\")",
	                arName.c_str(), override ? "true" : "false", type.c_str());

	CArchiveBase* ar = archives[arName];
	if (!ar) {
		ar = CArchiveFactory::OpenArchive(arName, type);
		if (!ar) {
			logOutput.Print(LOG_VFS, "AddArchive: Failed to open archive '%s'.", arName.c_str());
			return false;
		}
		archives[arName] = ar;
	}

	for (unsigned fid = 0; fid != ar->NumFiles(); ++fid)
	{
		std::string name;
		int size;
		ar->FileInfo(fid, name, size);
		StringToLowerInPlace(name);

		if (!override) {
			if (files.find(name) != files.end()) {
				logOutput.Print(LOG_VFS_DETAIL, "%s (skipping, exists)", name.c_str());
				continue;
			}
			else
				logOutput.Print(LOG_VFS_DETAIL, "%s (adding, doesn't exist)", name.c_str());
		}
		else
			logOutput.Print(LOG_VFS_DETAIL, "%s (overriding)", name.c_str());

		FileData d;
		d.ar = ar;
		d.size = size;
		d.dynamic = !!dynamic_cast<CArchiveDir*>(ar);
		files[name] = d;
	}
	return true;
}

bool CVFSHandler::AddArchiveWithDeps(const std::string& archiveName, bool override, const std::string& type)
{
	const std::vector<std::string> ars = archiveScanner->GetArchives(archiveName);
	if (ars.empty())
		throw content_error("Couldn't find any archives for '" + archiveName + "'.");
	std::vector<std::string>::const_iterator it;
	for (it = ars.begin(); it != ars.end(); ++it)
	{
		if (!AddArchive(*it, override, type))
			throw content_error("Couldn't load archive '" + *it + "' for '" + archiveName + "'.");
	}
	return true;
}

bool CVFSHandler::RemoveArchive(const std::string& arName)
{
	logOutput.Print(LOG_VFS, "RemoveArchive(arName = \"%s\")", arName.c_str());

	CArchiveBase* ar = archives[arName];
	if (ar == NULL) {
		// archive is not loaded
		return true;
	}
	
	// remove the files loaded from the archive to remove
	for (std::map<std::string, FileData>::iterator f = files.begin(); f != files.end();) {
		if (f->second.ar == ar) {
			logOutput.Print(LOG_VFS_DETAIL, "%s (removing)", f->first.c_str());
#ifdef _MSC_VER
			f = files.erase(f);
#else
			files.erase(f++);
#endif
		}
		else
			 ++f;
	}
	delete ar;
	archives.erase(arName);

	return true;
}

CVFSHandler::~CVFSHandler()
{
	logOutput.Print(LOG_VFS, "CVFSHandler::~CVFSHandler()");

	for (std::map<std::string, CArchiveBase*>::iterator i = archives.begin(); i != archives.end(); ++i) {
		delete i->second;
	}
}

bool CVFSHandler::LoadFile(const std::string& rawName, std::vector<boost::uint8_t>& buffer)
{
	logOutput.Print(LOG_VFS, "LoadFile(rawName = \"%s\", )", rawName.c_str());

	std::string name = StringToLower(rawName);
	filesystem.ForwardSlashes(name);

	std::map<std::string, FileData>::iterator fi = files.find(name);
	if (fi == files.end()) {
		logOutput.Print(LOG_VFS, "LoadFile: File '%s' does not exist in VFS.", rawName.c_str());
		return false;
	}
	FileData& fd = fi->second;

	if (!fd.ar->GetFile(name, buffer))
	{
		logOutput.Print(LOG_VFS, "LoadFile: File '%s' does not exist in archive.", rawName.c_str());
		return false;
	}
	return true;
}

// Returns all the files in the given (virtual) directory without the preceeding pathname
std::vector<std::string> CVFSHandler::GetFilesInDir(const std::string& rawDir)
{
	logOutput.Print(LOG_VFS, "GetFilesInDir(rawDir = \"%s\")", rawDir.c_str());

	std::vector<std::string> ret;
	std::string dir = StringToLower(rawDir);
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
				logOutput.Print(LOG_VFS_DETAIL, "%s", name.c_str());
			}
		}
		filesStart++;
	}

	return ret;
}


// Returns all the sub-directories in the given (virtual) directory without the preceeding pathname
std::vector<std::string> CVFSHandler::GetDirsInDir(const std::string& rawDir)
{
	logOutput.Print(LOG_VFS, "GetDirsInDir(rawDir = \"%s\")", rawDir.c_str());

	std::vector<std::string> ret;
	std::string dir = StringToLower(rawDir);
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
		logOutput.Print(LOG_VFS_DETAIL, "%s", it->c_str());
	}

	return ret;
}
