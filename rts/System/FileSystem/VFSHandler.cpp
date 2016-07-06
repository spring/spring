/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "VFSHandler.h"

#include <algorithm>
#include <set>
#include <cstring>

#include "ArchiveLoader.h"
#include "System/FileSystem/Archives/IArchive.h"
#include "FileSystem.h"
#include "ArchiveScanner.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/Util.h"


#define LOG_SECTION_VFS "VFS"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_VFS)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_VFS


CVFSHandler* vfsHandler = NULL;


CVFSHandler::CVFSHandler()
{
	LOG_L(L_DEBUG, "CVFSHandler::CVFSHandler()");
}


bool CVFSHandler::AddArchive(const std::string& archiveName, bool override, const std::string& type)
{
	LOG_L(L_DEBUG,
		"AddArchive(arName = \"%s\", override = %s, type = \"%s\")",
		archiveName.c_str(), override ? "true" : "false", type.c_str());

	IArchive* ar = archives[archiveName];

	if (ar == NULL) {
		ar = archiveLoader.OpenArchive(archiveName, type);
		if (!ar) {
			LOG_L(L_ERROR, "AddArchive: Failed to open archive '%s'.", archiveName.c_str());
			return false;
		}
		archives[archiveName] = ar;
	}

	for (unsigned fid = 0; fid != ar->NumFiles(); ++fid) {
		std::string name;
		int size;
		ar->FileInfo(fid, name, size);
		StringToLowerInPlace(name);

		if (!override) {
			if (files.find(name) != files.end()) {
				LOG_L(L_DEBUG, "%s (skipping, exists)", name.c_str());
				continue;
			} else {
				LOG_L(L_DEBUG, "%s (adding, does not exist)", name.c_str());
			}
		} else {
			LOG_L(L_DEBUG, "%s (overriding)", name.c_str());
		} 

		FileData d;
		d.ar = ar;
		d.size = size;
		files[name] = d;
	}

	return true;
}

bool CVFSHandler::AddArchiveWithDeps(const std::string& archiveName, bool override, const std::string& type)
{
	const std::vector<std::string> &ars = archiveScanner->GetAllArchivesUsedBy(archiveName);

	if (ars.empty())
		throw content_error("Could not find any archives for '" + archiveName + "'.");

	std::vector<std::string>::const_iterator it;

	for (it = ars.begin(); it != ars.end(); ++it) {
		if (!AddArchive(*it, override, type))
			throw content_error("Failed loading archive '" + *it + "', dependency of '" + archiveName + "'.");
	}

	return true;
}

bool CVFSHandler::RemoveArchive(const std::string& archiveName)
{
	LOG_L(L_DEBUG, "RemoveArchive(archiveName = \"%s\")", archiveName.c_str());

	const auto it = archives.find(archiveName);

	if (it == archives.end())
		return true;

	IArchive* ar = it->second;

	if (ar == NULL) {
		// archive is not loaded
		return true;
	}

	// remove the files loaded from the archive-to-remove
	for (std::map<std::string, FileData>::iterator f = files.begin(); f != files.end();) {
		if (f->second.ar == ar) {
			LOG_L(L_DEBUG, "%s (removing)", f->first.c_str());
			f = files.erase(f);
		} else {
			 ++f;
		}
	}
	delete ar;
	archives.erase(archiveName);

	return true;
}

CVFSHandler::~CVFSHandler()
{
	LOG_L(L_INFO, "[%s] #archives=%lu", __FUNCTION__, (long unsigned) archives.size());

	for (std::map<std::string, IArchive*>::iterator i = archives.begin(); i != archives.end(); ++i) {
		LOG_L(L_INFO, "\tarchive=%s (%p)", (i->first).c_str(), i->second);
		delete i->second;
	}
}

std::string CVFSHandler::GetNormalizedPath(const std::string& rawPath)
{
	std::string path = StringToLower(rawPath);
	FileSystem::ForwardSlashes(path);
	return path;
}

const CVFSHandler::FileData* CVFSHandler::GetFileData(const std::string& normalizedFilePath)
{
	const FileData* fileData = NULL;

	const std::map<std::string, FileData>::const_iterator fi = files.find(normalizedFilePath);
	if (fi != files.end()) {
		fileData = &(fi->second);
	}

	return fileData;
}

bool CVFSHandler::LoadFile(const std::string& filePath, std::vector<boost::uint8_t>& buffer)
{
	LOG_L(L_DEBUG, "LoadFile(filePath = \"%s\", )", filePath.c_str());

	const std::string normalizedPath = GetNormalizedPath(filePath);

	const FileData* fileData = GetFileData(normalizedPath);
	if (fileData == NULL) {
		LOG_L(L_DEBUG, "LoadFile: File '%s' does not exist in VFS.", filePath.c_str());
		return false;
	}

	if (!fileData->ar->GetFile(normalizedPath, buffer))
	{
		LOG_L(L_DEBUG, "LoadFile: File '%s' does not exist in archive.", filePath.c_str());
		return false;
	}
	return true;
}

bool CVFSHandler::FileExists(const std::string& filePath)
{
	LOG_L(L_DEBUG, "FileExists(filePath = \"%s\", )", filePath.c_str());

	const std::string normalizedPath = GetNormalizedPath(filePath);

	const FileData* fileData = GetFileData(normalizedPath);
	if (fileData == NULL) {
		// the file does not exist in the VFS
		return false;
	}

	if (!fileData->ar->FileExists(normalizedPath)) {
		// the file does not exist in the archive
		return false;
	}

	return true;
}

std::vector<std::string> CVFSHandler::GetFilesInDir(const std::string& rawDir)
{
	LOG_L(L_DEBUG, "GetFilesInDir(rawDir = \"%s\")", rawDir.c_str());

	std::vector<std::string> ret;
	std::string dir = GetNormalizedPath(rawDir);

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
		const std::string path = FileSystem::GetDirectory(filesStart->first);

		// Check if this file starts with the dir path
		if (path.compare(0, dir.length(), dir) == 0) {

			// Strip pathname
			const std::string name = filesStart->first.substr(dir.length());

			// Do not return files in subfolders
			if ((name.find('/') == std::string::npos)
					&& (name.find('\\') == std::string::npos))
			{
				ret.push_back(name);
				LOG_L(L_DEBUG, "%s", name.c_str());
			}
		}
		++filesStart;
	}

	return ret;
}


std::vector<std::string> CVFSHandler::GetDirsInDir(const std::string& rawDir)
{
	LOG_L(L_DEBUG, "GetDirsInDir(rawDir = \"%s\")", rawDir.c_str());

	std::vector<std::string> ret;
	std::string dir = GetNormalizedPath(rawDir);

	std::map<std::string, FileData>::const_iterator filesStart = files.begin();
	std::map<std::string, FileData>::const_iterator filesEnd   = files.end();

	// Non-empty directories to look in should have a trailing backslash
	if (!dir.empty()) {
		std::string::size_type dirLast = (dir.length() - 1);
		if (dir[dirLast] != '/') {
			dir += "/";
			++dirLast;
		}
		// limit the iterator range
		std::string dirEnd = dir;
		dirEnd[dirLast] = dirEnd[dirLast] + 1;
		filesStart = files.lower_bound(dir);
		filesEnd   = files.upper_bound(dirEnd);
	}

	std::set<std::string> dirs;

	while (filesStart != filesEnd) {
		const std::string path = FileSystem::GetDirectory(filesStart->first);
		// Test to see if this file start with the dir path
		if (path.compare(0, dir.length(), dir) == 0) {
			// Strip pathname
			const std::string name = filesStart->first.substr(dir.length());
			const std::string::size_type slash = name.find_first_of("/\\");
			if (slash != std::string::npos) {
				dirs.insert(name.substr(0, slash + 1));
			}
		}
		++filesStart;
	}

	for (std::set<std::string>::const_iterator it = dirs.begin(); it != dirs.end(); ++it) {
		ret.push_back(*it);
		LOG_L(L_DEBUG, "%s", it->c_str());
	}

	return ret;
}

