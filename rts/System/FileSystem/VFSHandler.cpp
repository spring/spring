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
#include "System/StringUtil.h"


#define LOG_SECTION_VFS "VFS"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_VFS)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_VFS


CVFSHandler* vfsHandler = nullptr;


CVFSHandler::CVFSHandler()
{
	LOG_L(L_DEBUG, "CVFSHandler::CVFSHandler()");
}

CVFSHandler::~CVFSHandler()
{
	LOG_L(L_INFO, "[%s] #archives=%lu", __FUNCTION__, (long unsigned) archives.size());

	for (auto i = archives.begin(); i != archives.end(); ++i) {
		LOG_L(L_INFO, "\tarchive=%s (%p)", (i->first).c_str(), i->second);
		delete i->second;
	}
}

CVFSHandler::Section CVFSHandler::GetModeSection(char mode)
{
	switch (mode) {
		case SPRING_VFS_MOD[0]:  return Section::Mod;
		case SPRING_VFS_MAP[0]:  return Section::Map;
		case SPRING_VFS_BASE[0]: return Section::Base;
		case SPRING_VFS_MENU[0]: return Section::Menu;
		default:                 return Section::Error;
	}
}

CVFSHandler::Section CVFSHandler::GetModTypeSection(int mt)
{
	switch (mt) {
		case modtype::hidden:  return Section::Mod;
		case modtype::primary: return Section::Mod;
		case modtype::map:     return Section::Map;
		case modtype::base:    return Section::Base;
		case modtype::menu:    return Section::Menu;
		default:               return Section::Error;
	}
}


static const std::string GetArchivePath(const std::string& name){
	if (name.empty())
		return name;
	const std::string& filename = archiveScanner->ArchiveFromName(name);
	if (filename == name)
		return "";
	const std::string& path = archiveScanner->GetArchivePath(filename);
	return path + filename;
}


bool CVFSHandler::AddArchive(const std::string& archiveName, bool overwrite)
{
	LOG_L(L_DEBUG,
		"AddArchive(arName = \"%s\", overwrite = %s)",
		archiveName.c_str(), overwrite ? "true" : "false");

	const CArchiveScanner::ArchiveData& ad = archiveScanner->GetArchiveData(archiveName);
	const Section section = GetModTypeSection(ad.GetModType());
	const std::string& archivePath = GetArchivePath(archiveName);

	assert(!ad.IsEmpty());
	assert(section < Section::Count);
	assert(!archivePath.empty());

	IArchive* ar = archives[archivePath];

	if (ar == nullptr) {
		if ((ar = archiveLoader.OpenArchive(archivePath)) == nullptr) {
			LOG_L(L_ERROR, "AddArchive: Failed to open archive '%s' (path '%s', type %d).", archiveName.c_str(), archivePath.c_str(), ad.GetModType());
			return false;
		}
		archives[archivePath] = ar;
	}

	for (unsigned fid = 0; fid != ar->NumFiles(); ++fid) {
		std::string name;
		int size;
		ar->FileInfo(fid, name, size);
		StringToLowerInPlace(name);

		if (!overwrite) {
			if (files[section].find(name) != files[section].end()) {
				LOG_L(L_DEBUG, "%s (skipping, exists)", name.c_str());
				continue;
			}

			LOG_L(L_DEBUG, "%s (adding, does not exist)", name.c_str());
		} else {
			LOG_L(L_DEBUG, "%s (overriding)", name.c_str());
		}

		FileData d;
		d.ar = ar;
		d.size = size;
		files[section][name] = d;
	}

	return true;
}

bool CVFSHandler::AddArchiveWithDeps(const std::string& archiveName, bool overwrite)
{
	const std::vector<std::string> ars = std::move(archiveScanner->GetAllArchivesUsedBy(archiveName));

	if (ars.empty())
		throw content_error("Could not find any archives for '" + archiveName + "'.");

	for (auto it = ars.cbegin(); it != ars.cend(); ++it) {
		if (!AddArchive(*it, overwrite))
			throw content_error("Failed loading archive '" + *it + "', dependency of '" + archiveName + "'.");
	}

	return true;
}

bool CVFSHandler::RemoveArchive(const std::string& archiveName)
{
	const CArchiveScanner::ArchiveData& ad = archiveScanner->GetArchiveData(archiveName);
	const Section section = GetModTypeSection(ad.GetModType());
	const std::string& archivePath = GetArchivePath(archiveName);

	assert(!ad.IsEmpty());
	assert(section < Section::Count);
	assert(!archivePath.empty());

	LOG_L(L_DEBUG, "RemoveArchive(archiveName = \"%s\")", archivePath.c_str());

	const auto it = archives.find(archivePath);

	if (it == archives.end())
		return true;

	IArchive* ar = it->second;

	// archive is not loaded
	if (ar == nullptr)
		return true;

	// remove the files loaded from the archive-to-remove
	for (auto f = files[section].begin(); f != files[section].end(); ) {
		if (f->second.ar == ar) {
			LOG_L(L_DEBUG, "%s (removing)", f->first.c_str());
			f = files[section].erase(f);
		} else {
			 ++f;
		}
	}

	delete ar;
	archives.erase(archivePath);

	return true;
}


std::string CVFSHandler::GetNormalizedPath(const std::string& rawPath)
{
	std::string path = StringToLower(rawPath);
	FileSystem::ForwardSlashes(path);
	return path;
}


const CVFSHandler::FileData* CVFSHandler::GetFileData(const std::string& normalizedFilePath, Section section)
{
	assert(section < Section::Count);

	const auto fi = files[section].find(normalizedFilePath);

	if (fi != files[section].end())
		return &(fi->second);

	return nullptr;
}


bool CVFSHandler::LoadFile(const std::string& filePath, std::vector<std::uint8_t>& buffer, Section section)
{
	assert(section < Section::Count);

	LOG_L(L_DEBUG, "LoadFile(filePath = \"%s\", )", filePath.c_str());

	const std::string& normalizedPath = GetNormalizedPath(filePath);
	const FileData* fileData = GetFileData(normalizedPath, section);

	if (fileData == nullptr) {
		LOG_L(L_DEBUG, "LoadFile: File '%s' does not exist in VFS.", filePath.c_str());
		return false;
	}

	if (!fileData->ar->GetFile(normalizedPath, buffer)) {
		LOG_L(L_DEBUG, "LoadFile: File '%s' does not exist in archive.", filePath.c_str());
		return false;
	}

	return true;
}


bool CVFSHandler::FileExists(const std::string& filePath, Section section)
{
	assert(section < Section::Count);

	LOG_L(L_DEBUG, "FileExists(filePath = \"%s\", )", filePath.c_str());

	const std::string& normalizedPath = GetNormalizedPath(filePath);
	const FileData* fileData = GetFileData(normalizedPath, section);

	// the file does not exist in the VFS
	if (fileData == nullptr)
		return false;

	// the file does not exist in the archive
	if (!fileData->ar->FileExists(normalizedPath))
		return false;

	return true;
}


std::vector<std::string> CVFSHandler::GetFilesInDir(const std::string& rawDir, Section section)
{
	assert(section < Section::Count);

	LOG_L(L_DEBUG, "GetFilesInDir(rawDir = \"%s\")", rawDir.c_str());

	std::vector<std::string> ret;
	std::string dir = GetNormalizedPath(rawDir);

	auto filesStart = files[section].begin();
	auto filesEnd   = files[section].end();

	// Non-empty directories to look in should have a trailing backslash
	if (!dir.empty()) {
		if (dir.back() != '/')
			dir += "/";

		// limit the iterator range; turn '/' into '0' for filesEnd
		filesStart = files[section].lower_bound(dir); dir.back() += 1;
		filesEnd   = files[section].upper_bound(dir); dir.back() -= 1;
	}

	ret.reserve(std::distance(filesStart, filesEnd));

	for (; filesStart != filesEnd; ++filesStart) {
		const std::string& path = FileSystem::GetDirectory(filesStart->first);

		// Check if this file starts with the dir path
		if (path.compare(0, dir.length(), dir) != 0)
			continue;

		// Strip pathname
		const std::string& name = filesStart->first.substr(dir.length());

		// Do not return files in subfolders
		if ((name.find('/') != std::string::npos) || (name.find('\\') != std::string::npos))
			continue;

		ret.push_back(name);
		LOG_L(L_DEBUG, "%s", name.c_str());
	}

	return ret;
}


std::vector<std::string> CVFSHandler::GetDirsInDir(const std::string& rawDir, Section section)
{
	assert(section < Section::Count);

	LOG_L(L_DEBUG, "GetDirsInDir(rawDir = \"%s\")", rawDir.c_str());

	std::vector<std::string> ret;
	std::set<std::string> dirs;
	std::string dir = GetNormalizedPath(rawDir);

	auto filesStart = files[section].begin();
	auto filesEnd   = files[section].end();

	// Non-empty directories to look in should have a trailing backslash
	if (!dir.empty()) {
		if (dir.back() != '/')
			dir += "/";

		// limit the iterator range (as in GetFilesInDir)
		filesStart = files[section].lower_bound(dir); dir.back() += 1;
		filesEnd   = files[section].upper_bound(dir); dir.back() -= 1;
	}

	ret.reserve(std::distance(filesStart, filesEnd));

	for (; filesStart != filesEnd; ++filesStart) {
		const std::string& path = FileSystem::GetDirectory(filesStart->first);

		// Test to see if this file start with the dir path
		if (path.compare(0, dir.length(), dir) != 0)
			continue;

		// Strip pathname
		const std::string& name = filesStart->first.substr(dir.length());
		const std::string::size_type slash = name.find_first_of("/\\");

		if (slash != std::string::npos) {
			dirs.insert(name.substr(0, slash + 1));
		}
	}

	for (auto it = dirs.cbegin(); it != dirs.cend(); ++it) {
		ret.push_back(*it);
		LOG_L(L_DEBUG, "%s", it->c_str());
	}

	return ret;
}

