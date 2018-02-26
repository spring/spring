/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "VFSHandler.h"

#include <algorithm>
#include <cstring>

#include "ArchiveLoader.h"
#include "System/FileSystem/Archives/IArchive.h"
#include "System/Threading/SpringThreading.h"
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


// GetFileData can be called on a thread other than main (e.g. sound) via
// FileHandler::Open, while {Add,Remove}Archive are reached from multiple
// places including LuaVFS
static spring::recursive_mutex vfsMutex;


CVFSHandler* vfsHandlerGlobal = nullptr;


CVFSHandler::CVFSHandler()
{
	LOG_L(L_DEBUG, "[%s]", __func__);
}

CVFSHandler::~CVFSHandler()
{
	LOG_L(L_INFO, "[%s] #archives=%lu", __func__, (long unsigned) archives.size());
	DeleteArchives();
}



void CVFSHandler::GrabLock() { vfsMutex.lock(); }
void CVFSHandler::FreeLock() { vfsMutex.unlock(); }

void CVFSHandler::FreeGlobalInstance() { FreeInstance(vfsHandlerGlobal); }
void CVFSHandler::FreeInstance(CVFSHandler* handler)
{
	if (handler != vfsHandlerGlobal) {
		// never happens
		delete handler;
		return;
	}

	spring::SafeDelete(vfsHandlerGlobal);
}

void CVFSHandler::SetGlobalInstance(CVFSHandler* handler)
{
	GrabLock();
	SetGlobalInstanceRaw(handler);
	FreeLock();
}
void CVFSHandler::SetGlobalInstanceRaw(CVFSHandler* handler)
{
	assert(vfsMutex.locked());
	vfsHandlerGlobal = handler;
}

CVFSHandler* CVFSHandler::GetGlobalInstance() {
	std::lock_guard<decltype(vfsMutex)> lck(vfsMutex);
	return vfsHandlerGlobal;
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


static const std::string GetArchivePath(const std::string& name) {
	if (name.empty())
		return name;

	const std::string& filename = archiveScanner->ArchiveFromName(name);

	if (filename == name)
		return "";

	return (archiveScanner->GetArchivePath(filename) + filename);
}



bool CVFSHandler::AddArchive(const std::string& archiveName, bool overwrite)
{
	std::lock_guard<decltype(vfsMutex)> lck(vfsMutex);
	LOG_L(L_DEBUG, "[VFSH::%s(arName=\"%s\", overwrite=%s)]", __func__, archiveName.c_str(), overwrite ? "true" : "false");

	const CArchiveScanner::ArchiveData& archiveData = archiveScanner->GetArchiveData(archiveName);
	const std::string& archivePath = GetArchivePath(archiveName);
	const Section section = GetModTypeSection(archiveData.GetModType());

	assert(!archiveData.IsEmpty());
	assert(!archivePath.empty());
	assert(section < Section::Count);

	IArchive* ar = archives[archivePath];

	if (ar == nullptr) {
		if ((ar = archiveLoader.OpenArchive(archivePath)) == nullptr) {
			LOG_L(L_ERROR, "[VFSH::%s] failed to open archive '%s' (path '%s', type %d)", __func__, archiveName.c_str(), archivePath.c_str(), archiveData.GetModType());
			return false;
		}
		archives[archivePath] = ar;
	}

	for (unsigned fid = 0; fid != ar->NumFiles(); ++fid) {
		std::pair<std::string, int> fi = ar->FileInfo(fid);
		std::string name = std::move(StringToLower(fi.first));

		if (!overwrite) {
			if (files[section].find(name) != files[section].end()) {
				LOG_L(L_DEBUG, "[VFSH::%s] skipping \"%s\", exists", __func__, name.c_str());
				continue;
			}

			LOG_L(L_DEBUG, "[VFSH::%s] adding \"%s\", does not exist", __func__, name.c_str());
		} else {
			LOG_L(L_DEBUG, "[VFSH::%s] overriding \"%s\"", __func__, name.c_str());
		}

		files[section][name] = {ar, fi.second};
	}

	return true;
}

bool CVFSHandler::AddArchiveWithDeps(const std::string& archiveName, bool overwrite)
{
	const std::vector<std::string> ars = std::move(archiveScanner->GetAllArchivesUsedBy(archiveName));

	if (ars.empty())
		throw content_error("Could not find any archives for '" + archiveName + "'.");

	for (const std::string& depArchiveName: ars) {
		if (!AddArchive(depArchiveName, overwrite))
			throw content_error("Failed loading archive '" + depArchiveName + "', dependency of '" + archiveName + "'.");
	}

	return true;
}

bool CVFSHandler::RemoveArchive(const std::string& archiveName)
{
	std::lock_guard<decltype(vfsMutex)> lck(vfsMutex);

	const CArchiveScanner::ArchiveData& archiveData = archiveScanner->GetArchiveData(archiveName);
	const std::string& archivePath = GetArchivePath(archiveName);
	const Section section = GetModTypeSection(archiveData.GetModType());

	assert(!archiveData.IsEmpty());
	assert(!archivePath.empty());
	assert(section < Section::Count);

	LOG_L(L_DEBUG, "[VFHS::%s(archiveName=\"%s\")]", __func__, archivePath.c_str());

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
			LOG_L(L_DEBUG, "[VFHS::%s] removing \"%s\"", __func__, f->first.c_str());
			f = files[section].erase(f);
		} else {
			 ++f;
		}
	}

	delete ar;
	archives.erase(archivePath);
	return true;
}



void CVFSHandler::DeleteArchives()
{
	LOG_L(L_INFO, "[VFSH::%s]", __func__);

	for (const auto& p: archives) {
		LOG_L(L_INFO, "\tarchive=%s (%p)", (p.first).c_str(), p.second);
		delete p.second;
	}

	archives.clear();

	for (auto& fdm: files) {
		fdm.clear();
	}
}



std::string CVFSHandler::GetNormalizedPath(const std::string& rawPath)
{
	std::string lcPath = std::move(StringToLower(rawPath));
	std::string nPath = std::move(FileSystem::ForwardSlashes(lcPath));
	return nPath;
}


CVFSHandler::FileData CVFSHandler::GetFileData(const std::string& normalizedFilePath, Section section)
{
	assert(section < Section::Count);
	std::lock_guard<decltype(vfsMutex)> lck(vfsMutex);

	const auto fi = files[section].find(normalizedFilePath);

	if (fi != files[section].end())
		return fi->second;

	// file does not exist in the VFS
	return {nullptr, 0};
}



bool CVFSHandler::LoadFile(const std::string& filePath, std::vector<std::uint8_t>& buffer, Section section)
{
	LOG_L(L_DEBUG, "[VFSH::%s(filePath=\"%s\", section=%d)]", __func__, filePath.c_str(), section);

	const std::string& normalizedPath = GetNormalizedPath(filePath);
	const FileData& fileData = GetFileData(normalizedPath, section);

	if (fileData.ar == nullptr)
		return false;

	return (fileData.ar->GetFile(normalizedPath, buffer));
}

bool CVFSHandler::FileExists(const std::string& filePath, Section section)
{
	LOG_L(L_DEBUG, "[VFSH::%s(filePath=\"%s\", section=%d)]", __func__, filePath.c_str(), section);

	const std::string& normalizedPath = GetNormalizedPath(filePath);
	const FileData& fileData = GetFileData(normalizedPath, section);

	if (fileData.ar == nullptr)
		return false;

	return (fileData.ar->FileExists(normalizedPath));
}




std::vector<std::string> CVFSHandler::GetFilesInDir(const std::string& rawDir, Section section)
{
	std::lock_guard<decltype(vfsMutex)> lck(vfsMutex);

	assert(section < Section::Count);

	LOG_L(L_DEBUG, "[VFSH::%s(rawDir=\"%s\")]", __func__, rawDir.c_str());

	std::vector<std::string> dirFiles;
	std::string dir = std::move(GetNormalizedPath(rawDir));

	auto filesStart = files[section].begin();
	auto filesEnd   = files[section].end();

	// non-empty directories to look in should have a trailing backslash
	if (!dir.empty()) {
		if (dir.back() != '/')
			dir += "/";

		// limit the iterator range; turn '/' into '0' for filesEnd
		filesStart = files[section].lower_bound(dir); dir.back() += 1;
		filesEnd   = files[section].upper_bound(dir); dir.back() -= 1;
	}

	dirFiles.reserve(std::distance(filesStart, filesEnd));

	for (; filesStart != filesEnd; ++filesStart) {
		const std::string& path = FileSystem::GetDirectory(filesStart->first);

		// Check if this file starts with the dir path
		if (path.compare(0, dir.length(), dir) != 0)
			continue;

		// strip pathname
		std::string name = std::move(filesStart->first.substr(dir.length()));

		// do not return files in subfolders
		if ((name.find('/') != std::string::npos) || (name.find('\\') != std::string::npos))
			continue;

		dirFiles.emplace_back(std::move(name));
		LOG_L(L_DEBUG, "\t%s", dirFiles[dirFiles.size() - 1].c_str());
	}

	return dirFiles;
}


std::vector<std::string> CVFSHandler::GetDirsInDir(const std::string& rawDir, Section section)
{
	std::lock_guard<decltype(vfsMutex)> lck(vfsMutex);

	assert(section < Section::Count);

	LOG_L(L_DEBUG, "[VFSH::%s(rawDir=\"%s\")]", __func__, rawDir.c_str());

	std::vector<std::string> dirs;
	std::vector<std::string>::iterator iter;
	std::string dir = std::move(GetNormalizedPath(rawDir));

	auto filesStart = files[section].begin();
	auto filesEnd   = files[section].end();

	// non-empty directories to look in should have a trailing backslash
	if (!dir.empty()) {
		if (dir.back() != '/')
			dir += "/";

		// limit the iterator range (as in GetFilesInDir)
		filesStart = files[section].lower_bound(dir); dir.back() += 1;
		filesEnd   = files[section].upper_bound(dir); dir.back() -= 1;
	}

	dirs.reserve(std::distance(filesStart, filesEnd));

	for (; filesStart != filesEnd; ++filesStart) {
		const std::string& path = FileSystem::GetDirectory(filesStart->first);

		// test if this file starts with the dir path
		if (path.compare(0, dir.length(), dir) != 0)
			continue;

		// strip pathname
		const std::string& name = filesStart->first.substr(dir.length());
		const std::string::size_type slash = name.find_first_of("/\\");

		if (slash == std::string::npos)
			continue;

		dirs.emplace_back(std::move(name.substr(0, slash + 1)));
	}

	std::stable_sort(dirs.begin(), dirs.end());

	// filter duplicates
	if ((iter = std::unique(dirs.begin(), dirs.end())) != dirs.end())
		dirs.erase(iter, dirs.end());

	return dirs;
}

