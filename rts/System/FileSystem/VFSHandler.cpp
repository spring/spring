/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "VFSHandler.h"

#include <algorithm>
#include <cstring>

#include "ArchiveLoader.h"
#include "ArchiveScanner.h"
#include "FileSystem.h"
#include "System/FileSystem/Archives/IArchive.h"
#include "System/FileSystem/Archives/DirArchive.h"
#include "System/Threading/SpringThreading.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/SafeUtil.h"
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


static CVFSHandler* vfs = nullptr;


void CVFSHandler::GrabLock() { vfsMutex.lock(); }
void CVFSHandler::FreeLock() { vfsMutex.unlock(); }

void CVFSHandler::FreeGlobalInstance() { FreeInstance(vfs); }
void CVFSHandler::FreeInstance(CVFSHandler* handler)
{
	if (handler != vfs) {
		// never happens
		delete handler;
		return;
	}

	spring::SafeDelete(vfs);
}

void CVFSHandler::SetGlobalInstance(CVFSHandler* handler)
{
	GrabLock();
	SetGlobalInstanceRaw(handler);
	FreeLock();
}
void CVFSHandler::SetGlobalInstanceRaw(CVFSHandler* handler)
{
	const char* curHandlerName = (vfs != nullptr)? vfs->GetName(): "null";
	const char* newHandlerName = handler->GetName();

	LOG_L(L_INFO, "[VFSHandler::%s] handler=%p (%s) global=%p (%s)", __func__, handler, newHandlerName, vfs, curHandlerName);

	// assert(vfsMutex.locked());
	vfs = handler;
}

CVFSHandler* CVFSHandler::GetGlobalInstance() {
	std::lock_guard<decltype(vfsMutex)> lck(vfsMutex);
	return vfs;
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

CVFSHandler::Section CVFSHandler::GetArchiveSection(const std::string& archiveName)
{
	const CArchiveScanner::ArchiveData& archiveData = archiveScanner->GetArchiveData(archiveName);
	return GetModTypeSection(archiveData.GetModType());
}



bool CVFSHandler::HasArchive(const std::string& archiveName, Section archiveSection) const
{
	std::lock_guard<decltype(vfsMutex)> lck(vfsMutex);

	return (!archiveName.empty() && archives[archiveSection].find(GetArchivePath(archiveName)) != archives[archiveSection].end());
}



bool CVFSHandler::AddArchive(const std::string& archiveName, bool overwrite)
{
	std::lock_guard<decltype(vfsMutex)> lck(vfsMutex);

	if (!insertAllowed)
		return false;

	const CArchiveScanner::ArchiveData& archiveData = archiveScanner->GetArchiveData(archiveName);
	const std::string& archivePath = GetArchivePath(archiveName);
	const Section rawSection = GetModTypeSection(archiveData.GetModType());
	const Section tmpSection = Section(rawSection + (Section::TempMod - Section::Mod));

	assert(!archiveData.IsEmpty());
	assert(!archivePath.empty());
	assert(rawSection < Section::Count);

	// populate files from stashed archive if possible
	IArchive* ar = archives[tmpSection][archivePath];

	LOG_L(L_INFO, "[%s::%s<this=%p>(arName=\"%s\", overwrite=%s)] section=%d cached=%d", vfsName, __func__, this, archiveName.c_str(), overwrite ? "true" : "false", rawSection, ar != nullptr);

	if (ar == nullptr) {
		archives[tmpSection].erase(archivePath);

		if ((ar = archives[rawSection][archivePath]) == nullptr) {
			// remove the entry created by operator[]
			archives[rawSection].erase(archivePath);

			if ((ar = archiveLoader.OpenArchive(archivePath)) == nullptr) {
				LOG_L(L_ERROR, "[%s::%s<this=%p>] failed to open archive '%s' (path '%s', type %d)", vfsName, __func__, this, archiveName.c_str(), archivePath.c_str(), archiveData.GetModType());
				return false;
			}

			archives[rawSection].emplace(archivePath, ar);
		}
	}


	files[Section::Temp].clear();
	files[Section::Temp].reserve(ar->NumFiles());

	for (unsigned fid = 0; fid != ar->NumFiles(); ++fid) {
		std::pair<std::string, int> fi = ar->FileInfo(fid);
		std::string name = std::move(StringToLower(fi.first));

		if (!overwrite) {
			const auto pred = [](const FileEntry& a, const FileEntry& b) { return (a.first < b.first); };
			const auto iter = std::lower_bound(files[rawSection].begin(), files[rawSection].end(), FileEntry{name, FileData{}}, pred);

			if (iter != files[rawSection].end() && iter->first == name) {
				LOG_L(L_DEBUG, "[%s::%s<this=%p>] skipping \"%s\", exists", vfsName, __func__, this, name.c_str());
				continue;
			}

			LOG_L(L_DEBUG, "[%s::%s<this=%p>] adding \"%s\", does not exist", vfsName, __func__, this, name.c_str());
		} else {
			LOG_L(L_DEBUG, "[%s::%s<this=%p>] overriding \"%s\"", vfsName, __func__, this, name.c_str());
		}

		// can not add directly to files[section], would break lower_bound
		// note: this means an archive can *internally* contain duplicates
		files[Section::Temp].emplace_back(name, FileData{ar, fi.second});
	}

	for (FileEntry& fileEntry: files[Section::Temp]) {
		files[rawSection].emplace_back(std::move(fileEntry));
	}

	std::stable_sort(files[rawSection].begin(), files[rawSection].end(), [](const FileEntry& a, const FileEntry& b) { return (a.first < b.first); });
	return true;
}


bool CVFSHandler::AddArchiveWithDeps(const std::string& archiveName, bool overwrite)
{
	const std::vector<std::string> ars = std::move(archiveScanner->GetAllArchivesUsedBy(archiveName));

	if (ars.empty())
		throw content_error("[AddArchiveWithDeps] could not find any archives for '" + archiveName + "'.");

	for (const std::string& depArchiveName: ars) {
		if (!AddArchive(depArchiveName, overwrite))
			throw content_error("[AddArchiveWithDeps] failed loading archive '" + depArchiveName + "', dependency of '" + archiveName + "'.");
	}

	return true;
}


bool CVFSHandler::RemoveArchive(const std::string& archiveName)
{
	std::lock_guard<decltype(vfsMutex)> lck(vfsMutex);

	if (!removeAllowed)
		return false;

	const CArchiveScanner::ArchiveData& archiveData = archiveScanner->GetArchiveData(archiveName);
	const std::string& archivePath = GetArchivePath(archiveName);
	const Section section = GetModTypeSection(archiveData.GetModType());

	assert(!archiveData.IsEmpty());
	assert(!archivePath.empty());
	assert(section < Section::Count);

	LOG_L(L_INFO, "[%s::%s<this=%p>(archiveName=\"%s\")][1] section=%d", vfsName, __func__, this, archivePath.c_str(), section);

	const auto it = archives[section].find(archivePath);

	if (it == archives[section].end())
		return true;

	IArchive* ar = it->second;

	// archive is not loaded
	if (ar == nullptr)
		return true;


	for (auto& pair: files[section]) {
		auto& name = pair.first;

		if ((pair.second).ar != ar)
			continue;

		// mark entry for removal
		name.clear();
	}

	{
		const auto beg = files[section].begin();
		const auto end = files[section].end();
		const auto pos = std::remove_if(beg, end, [](const FileEntry& e) { return (e.first.empty()); });

		LOG_L(L_INFO, "[%s::%s<this=%p>][2] #files[section]=" _STPF_ "/" _STPF_ "", vfsName, __func__, this, end - beg, pos - beg);

		// wipe entries belonging to the to-be-deleted archive
		files[section].erase(pos, end);
	}


	delete ar;
	archives[section].erase(archivePath);
	return true;
}



void CVFSHandler::DeleteArchives()
{
	std::lock_guard<decltype(vfsMutex)> lck(vfsMutex);

	LOG_L(L_INFO, "[%s::%s<this=%p>]", vfsName, __func__, this);

	for (int section = Section::Mod; section <= Section::Temp; section++) {
		DeleteArchives(Section(section));
	}

	for (int section = Section::TempMod; section <= Section::TempMenu; section++) {
		DeleteArchives(Section(section));
	}
}

void CVFSHandler::DeleteArchives(Section section)
{
	LOG_L(L_INFO, "[%s::%s<this=%p>(section=%d)] #archives[section]=" _STPF_ " #files[section]=" _STPF_ "", vfsName, __func__, this, section, archives[section].size(), files[section].size());

	std::lock_guard<decltype(vfsMutex)> lck(vfsMutex);

	for (const auto& p: archives[section]) {
		LOG_L(L_INFO, "\tarchive=%s (%p)", (p.first).c_str(), p.second);
		delete p.second;
	}

	archives[section].clear();
	files[section].clear();
}

void CVFSHandler::ReserveArchives()
{
	LOG_L(L_INFO, "[%s::%s<this=%p>]", vfsName, __func__, this);

	std::lock_guard<decltype(vfsMutex)> lck(vfsMutex);

	for (int section = Section::Mod; section <= Section::Menu; section++) {
		assert(files[section].empty());

		archives[section].clear();
		archives[section].reserve(64);

		files[section].clear();
		files[section].reserve(2048);
	}

	// preload universal dependencies
	AddArchive(CArchiveScanner::GetSpringBaseContentName(), false);
}


void CVFSHandler::UnMapArchives(bool reload)
{
	std::lock_guard<decltype(vfsMutex)> lck(vfsMutex);

	LOG_L(L_INFO, "[%s::%s<this=%p>(reload=%d)] (#mod=" _STPF_ " #map=" _STPF_ " #menu=" _STPF_ ")", vfsName, __func__, this, reload, files[Section::Mod].size(), files[Section::Map].size(), files[Section::Menu].size());

	if (reload) {
		files[Section::Mod ].clear();
		files[Section::Map ].clear();
		// base is a dependency of most archives, leave it alone
		// files[Section::Base].clear();
		// menu persists reload, but controller is always reset
		files[Section::Menu].clear();

		// stash archives when reloading from game to menu
		for (const auto& pair: archives[Section::Mod]) {
			archives[Section::TempMod ].insert(pair);
		}
		for (const auto& pair: archives[Section::Map]) {
			archives[Section::TempMap ].insert(pair);
		}
		for (const auto& pair: archives[Section::Menu]) {
			archives[Section::TempMenu].insert(pair);
		}

		archives[Section::Mod ].clear();
		archives[Section::Map ].clear();
		archives[Section::Menu].clear();
	} else {
		std::swap(files[Section::Mod ], files[Section::TempMod ]);
		std::swap(files[Section::Map ], files[Section::TempMap ]);
		std::swap(files[Section::Menu], files[Section::TempMenu]);

		files[Section::Mod ].clear();
		files[Section::Map ].clear();
		files[Section::Menu].clear();
	}
}

void CVFSHandler::ReMapArchives(bool reload)
{
	std::lock_guard<decltype(vfsMutex)> lck(vfsMutex);

	LOG_L(L_INFO, "[%s::%s<this=%p>(reload=%d)] (#mod=" _STPF_ " #map=" _STPF_ " #menu=" _STPF_ ")", vfsName, __func__, this, reload, files[Section::Mod].size(), files[Section::Map].size(), files[Section::Menu].size());

	if (reload) {
		assert(false);
	} else {
		std::swap(files[Section::Mod ], files[Section::TempMod ]);
		std::swap(files[Section::Map ], files[Section::TempMap ]);
		std::swap(files[Section::Menu], files[Section::TempMenu]);

		files[Section::TempMod ].clear();
		files[Section::TempMap ].clear();
		files[Section::TempMenu].clear();
	}
}


void CVFSHandler::SwapArchiveSections(Section src, Section dst)
{
	std::lock_guard<decltype(vfsMutex)> lck(vfsMutex);

	LOG_L(L_INFO, "[%s::%s<this=%p>(src=%d dst=%d)]", vfsName, __func__, this, src, dst);

	std::swap(   files[src],    files[dst]);
	std::swap(archives[src], archives[dst]);
}



std::string CVFSHandler::GetNormalizedPath(const std::string& rawPath)
{
	std::string lcPath = std::move(StringToLower(rawPath));
	std::string nPath = std::move(FileSystem::ForwardSlashes(lcPath));
	return nPath;
}


CVFSHandler::FileData CVFSHandler::GetFileData(const std::string& normalizedFilePath, Section section) const
{
	assert(section < Section::Count);
	std::lock_guard<decltype(vfsMutex)> lck(vfsMutex);

	const auto& vect = files[section];
	const auto  cbeg = vect.cbegin();
	const auto  cend = vect.cend();
	const auto  file = FileEntry{normalizedFilePath, FileData{}};
	const auto  sane = [&]() -> bool {
		for (size_t i = 1, n = vect.size(); i < n; i++) {
			if (vect[i - 1].first > vect[i].first)
				return false;
		}
		return true;
	};

	{
		assert(sane());

		const auto pred = [ ](const FileEntry& a, const FileEntry& b) { return (a.first < b.first); };
		const auto iter = std::lower_bound(cbeg, cend, file, pred);

		if (iter != cend && iter->first == normalizedFilePath)
			return {iter->second.ar, iter->second.size};
	}

	// file does not exist in the VFS
	return {nullptr, 0};
}



int CVFSHandler::LoadFile(const std::string& filePath, std::vector<std::uint8_t>& buffer, Section section)
{
	LOG_L(L_DEBUG, "[%s::%s<this=%p>(filePath=\"%s\", section=%d)]", vfsName, __func__, this, filePath.c_str(), section);

	const std::string& normalizedPath = GetNormalizedPath(filePath);
	const FileData& fileData = GetFileData(normalizedPath, section);

	if (fileData.ar == nullptr)
		return -1;

	// 0 or 1
	return (fileData.ar->GetFile(normalizedPath, buffer));
}

int CVFSHandler::FileExists(const std::string& filePath, Section section)
{
	LOG_L(L_DEBUG, "[%s::%s<this=%p>(filePath=\"%s\", section=%d)]", vfsName, __func__, this, filePath.c_str(), section);

	const std::string& normalizedPath = GetNormalizedPath(filePath);
	const FileData& fileData = GetFileData(normalizedPath, section);

	if (fileData.ar == nullptr)
		return -1;

	// 0 or 1
	return (fileData.ar->FileExists(normalizedPath));
}

std::string CVFSHandler::GetFileAbsolutePath(const std::string& filePath, Section section)
{
	LOG_L(L_DEBUG, "[%s::%s<this=%p>(filePath=\"%s\", section=%d)]", vfsName, __func__, this, filePath.c_str(), section);

	const std::string& normalizedPath = GetNormalizedPath(filePath);
	const FileData& fileData = GetFileData(normalizedPath, section);

	// Only directory archives have an absolute path on disk
	const auto dirArchive = dynamic_cast<const CDirArchive*>(fileData.ar);

	if (dirArchive == nullptr)
		return "";

	const std::string& origFilePath = dirArchive->GetOrigFileName(dirArchive->FindFile(filePath));
	return (fileData.ar->GetArchiveFile() + "/" + origFilePath);
}

std::string CVFSHandler::GetFileArchiveName(const std::string& filePath, Section section)
{
	LOG_L(L_DEBUG, "[%s::%s<this=%p>(filePath=\"%s\", section=%d)]", vfsName, __func__, this, filePath.c_str(), section);

	const std::string& normalizedPath = GetNormalizedPath(filePath);
	const auto& fileData = GetFileData(normalizedPath, section);
	const auto& archiveFile = fileData.ar->GetArchiveFile();
	const auto& baseName = FileSystem::GetFilename(archiveFile);
	const auto& archiveName = archiveScanner->NameFromArchive(baseName);

	return archiveName;
}

std::vector<std::string> CVFSHandler::GetAllArchiveNames() const
{
	std::lock_guard<decltype(vfsMutex)> lck(vfsMutex);

	std::vector<std::string> ret;
	ret.reserve(archives[Section::Mod].size() + archives[Section::Map].size() + archives[Section::Base].size() + archives[Section::Menu].size());

	for (int section = Section::Mod; section <= Section::Menu; section++) {
		for (const auto& archive: archives[section]) {
			const auto& archiveFile = archive.second->GetArchiveFile();
			const auto& baseName = FileSystem::GetFilename(archiveFile);
			const auto& archiveName = archiveScanner->NameFromArchive(baseName);
			ret.push_back(archiveName);
		}
	}

	return ret;
}

std::vector<std::string> CVFSHandler::GetFilesInDir(const std::string& rawDir, Section section)
{
	std::lock_guard<decltype(vfsMutex)> lck(vfsMutex);

	assert(section < Section::Count);

	LOG_L(L_DEBUG, "[%s::%s<this=%p>(rawDir=\"%s\")] section=%d", vfsName, __func__, this, rawDir.c_str(), section);

	std::vector<std::string> dirFiles;
	std::string dir = std::move(GetNormalizedPath(rawDir));


	const auto filesPred = [](const FileEntry& a, const FileEntry& b) { return (a.first < b.first); };

	auto& filesVec = files[section];
	auto  filesBeg = filesVec.begin();
	auto  filesEnd = filesVec.end();

	// non-empty directories to look in should have a trailing backslash
	if (!dir.empty()) {
		if (dir.back() != '/')
			dir += "/";

		// limit the iterator range; turn '/' into '0' for filesEnd
		filesBeg = std::lower_bound(filesVec.begin(), filesVec.end(), FileEntry{dir, FileData{}}, filesPred); dir.back() += 1;
		filesEnd = std::upper_bound(filesVec.begin(), filesVec.end(), FileEntry{dir, FileData{}}, filesPred); dir.back() -= 1;
	}

	dirFiles.reserve(std::distance(filesBeg, filesEnd));

	for (; filesBeg != filesEnd; ++filesBeg) {
		const std::string& path = FileSystem::GetDirectory(filesBeg->first);

		// Check if this file starts with the dir path
		if (path.compare(0, dir.length(), dir) != 0)
			continue;

		// strip pathname
		std::string name = std::move(filesBeg->first.substr(dir.length()));

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

	LOG_L(L_DEBUG, "[%s::%s<this=%p>(rawDir=\"%s\")] section=%d", vfsName, __func__, this, rawDir.c_str(), section);

	std::vector<std::string> dirs;
	std::vector<std::string>::iterator iter;
	std::string dir = std::move(GetNormalizedPath(rawDir));


	const auto filesPred = [](const FileEntry& a, const FileEntry& b) { return (a.first < b.first); };

	auto& filesVec = files[section];
	auto  filesBeg = filesVec.begin();
	auto  filesEnd = filesVec.end();

	// non-empty directories to look in should have a trailing backslash
	if (!dir.empty()) {
		if (dir.back() != '/')
			dir += "/";

		// limit the iterator range (as in GetFilesInDir)
		filesBeg = std::lower_bound(filesVec.begin(), filesVec.end(), FileEntry{dir, FileData{}}, filesPred); dir.back() += 1;
		filesEnd = std::upper_bound(filesVec.begin(), filesVec.end(), FileEntry{dir, FileData{}}, filesPred); dir.back() -= 1;
	}

	dirs.reserve(std::distance(filesBeg, filesEnd));

	for (; filesBeg != filesEnd; ++filesBeg) {
		const std::string& path = FileSystem::GetDirectory(filesBeg->first);

		// test if this file starts with the dir path
		if (path.compare(0, dir.length(), dir) != 0)
			continue;

		// strip pathname
		const std::string& name = filesBeg->first.substr(dir.length());
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

