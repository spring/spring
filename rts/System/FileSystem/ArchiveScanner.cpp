/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <algorithm>
#include <array>
#include <cstdio>
#include <memory>

#include <sys/types.h>
#include <sys/stat.h>

#include "ArchiveNameResolver.h"
#include "ArchiveScanner.h"
#include "ArchiveLoader.h"
#include "DataDirLocater.h"
#include "Archives/IArchive.h"
#include "FileFilter.h"
#include "DataDirsAccess.h"
#include "FileSystem.h"
#include "FileQueryFlags.h"
#include "Lua/LuaParser.h"
#include "System/CRC.h"
#include "System/Util.h"
#include "System/Exceptions.h"
#include "System/Threading/ThreadPool.h"
#include "System/FileSystem/RapidHandler.h"
#include "System/Log/ILog.h"
#include "System/Threading/SpringThreading.h"
#include "System/UnorderedMap.hpp"

#if !defined(DEDICATED) && !defined(UNITSYNC)
	#include "System/TimeProfiler.h"
	#include "System/Platform/Watchdog.h"
#endif


#define LOG_SECTION_ARCHIVESCANNER "ArchiveScanner"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_ARCHIVESCANNER)


/*
 * The archive scanner is used to find stuff in archives
 * which are needed before building the virtual filesystem.
 * This currently includes maps and mods.
 * It uses caching to speed up the process.
 *
 * It only retrieves info that is used in an initial listing.
 * For detailed info when selecting a map for example,
 * the more specialized parsers will be used.
 * (mapping one archive when selecting a map is not slow,
 * but mapping them all, every time to make the list is)
 */

const int INTERNAL_VER = 11;

CArchiveScanner* archiveScanner = nullptr;



/*
 * Engine known (and used?) tags in [map|mod]info.lua
 */
struct KnownInfoTag {
	std::string name;
	std::string desc;
	bool required;
};

const std::array<KnownInfoTag, 12> knownTags = {
	KnownInfoTag{"name",        "example: Original Total Annihilation",                            true},
	KnownInfoTag{"shortname",   "example: OTA",                                                   false},
	KnownInfoTag{"version",     "example: v2.3",                                                  false},
	KnownInfoTag{"mutator",     "example: deployment",                                            false},
	KnownInfoTag{"game",        "example: Total Annihilation",                                    false},
	KnownInfoTag{"shortgame",   "example: TA",                                                    false},
	KnownInfoTag{"description", "example: Little units blowing up other little units",            false},
	KnownInfoTag{"mapfile",     "in case its a map, store location of smf/sm3 file",              false}, //FIXME is this ever used in the engine?! or does it auto calc the location?
	KnownInfoTag{"modtype",     "0=hidden, 1=primary, (2=unused), 3=map, 4=base, 5=menu",          true},
	KnownInfoTag{"depend",      "a table with all archives that needs to be loaded for this one", false},
	KnownInfoTag{"replace",     "a table with archives that got replaced with this one",          false},
	KnownInfoTag{"onlyLocal",   "if true spring will not listen for incoming connections",        false}
};

const spring::unordered_map<std::string, bool> baseContentArchives = {
	{      "bitmaps.sdz", true},
	{"springcontent.sdz", true},
	{    "maphelper.sdz", true},
	{      "cursors.sdz", true},
};

// 1: commonly read from all archives when scanning through them
// 2: less commonly used, or only when looking at a specific archive
//    (for example when hosting Game-X)
//
// Lobbies get the unit list from unitsync. Unitsync gets it by executing
// gamedata/defs.lua, which loads units, features, weapons, movetypes and
// armors (that is why armor.txt is in the list).
const spring::unordered_map<std::string, int> metaFileClasses = {
	{      "mapinfo.lua", 1},   // basic archive info
	{      "modinfo.lua", 1},   // basic archive info
	{   "modoptions.lua", 2},   // used by lobbies
	{"engineoptions.lua", 2},   // used by lobbies
	{    "validmaps.lua", 2},   // used by lobbies
	{        "luaai.lua", 2},   // used by lobbies
	{        "armor.txt", 2},   // used by lobbies (disabled units list)
	{ "springignore.txt", 2},   // used by lobbies (disabled units list)
};

const spring::unordered_map<std::string, int> metaDirClasses = {
	{"sidepics/", 2},   // used by lobbies
	{"gamedata/", 2},   // used by lobbies
	{   "units/", 2},   // used by lobbies (disabled units list)
	{"features/", 2},   // used by lobbies (disabled units list)
	{ "weapons/", 2},   // used by lobbies (disabled units list)
};



/*
 * CArchiveScanner::ArchiveData
 */
CArchiveScanner::ArchiveData::ArchiveData(const LuaTable& archiveTable, bool fromCache)
{
	if (!archiveTable.IsValid())
		return;

	std::vector<std::string> keys;
	if (!archiveTable.GetKeys(keys))
		return;

	for (std::string& key: keys) {
		const std::string& keyLower = StringToLower(key);

		if (ArchiveData::IsReservedKey(keyLower))
			continue;

		if (keyLower == "modtype") {
			SetInfoItemValueInteger(key, archiveTable.GetInt(key, 0));
			continue;
		}

		switch (archiveTable.GetType(key)) {
			case LuaTable::STRING: {
				SetInfoItemValueString(key, archiveTable.GetString(key, ""));
			} break;
			case LuaTable::NUMBER: {
				SetInfoItemValueFloat(key, archiveTable.GetFloat(key, 0.0f));
			} break;
			case LuaTable::BOOLEAN: {
				SetInfoItemValueBool(key, archiveTable.GetBool(key, false));
			} break;
			default: {
				// just ignore unsupported types (most likely to be lua-tables)
				//throw content_error("Lua-type " + IntToString(luaType) + " not supported in archive-info, but it is used on key \"" + *key + "\"");
			} break;
		}
	}

	const LuaTable& _dependencies = archiveTable.SubTable("depend");
	const LuaTable& _replaces = archiveTable.SubTable("replace");

	for (int dep = 1; _dependencies.KeyExists(dep); ++dep) {
		dependencies.push_back(_dependencies.GetString(dep, ""));
	}
	for (int rep = 1; _replaces.KeyExists(rep); ++rep) {
		replaces.push_back(_replaces.GetString(rep, ""));
	}

	// FIXME
	// XXX HACK needed until lobbies, lobbyserver and unitsync are sorted out
	// so they can uniquely identify different versions of the same mod.
	// (at time of this writing they use name only)

	// NOTE when changing this, this function is used both by the code that
	// reads ArchiveCache.lua and the code that reads modinfo.lua from the mod.
	// so make sure it doesn't keep adding stuff to the name everytime
	// Spring/unitsync is loaded.

	const std::string& name = GetNameVersioned();
	const std::string& version = GetVersion();
	if (!version.empty()) {
		if (name.find(version) == std::string::npos) {
			SetInfoItemValueString("name", name + " " + version);
		} else if (!fromCache) {
			LOG_L(L_WARNING, "Invalid Name detected, please contact the author of the archive to remove the Version from the Name: %s, Version: %s", name.c_str(), version.c_str());
		}
	}

	if (GetName().empty())
		SetInfoItemValueString("name_pure", name);
}

std::string CArchiveScanner::ArchiveData::GetKeyDescription(const std::string& keyLower)
{
	const auto pred = [&keyLower](const KnownInfoTag& t) { return (t.name == keyLower); };
	const auto iter = std::find_if(knownTags.cbegin(), knownTags.cend(), pred);

	if (iter != knownTags.cend())
		return (iter->desc);

	return "<custom property>";
}


bool CArchiveScanner::ArchiveData::IsReservedKey(const std::string& keyLower)
{
	return ((keyLower == "depend") || (keyLower == "replace"));
}


bool CArchiveScanner::ArchiveData::IsValid(std::string& err) const
{
	const auto pred = [this](const KnownInfoTag& t) { return (t.required && info.find(t.name) == info.end()); };
	const auto iter = std::find_if(knownTags.cbegin(), knownTags.cend(), pred);

	if (iter == knownTags.cend())
		return true;

	err = "Missing tag \"" + iter->name + "\".";
	return false;
}


InfoItem* CArchiveScanner::ArchiveData::GetInfoItem(const std::string& key)
{
	const auto ii = info.find(StringToLower(key));

	if (ii != info.end())
		return &(ii->second);

	return nullptr;
}

const InfoItem* CArchiveScanner::ArchiveData::GetInfoItem(const std::string& key) const
{
	const auto ii = info.find(StringToLower(key));

	if (ii != info.end())
		return &(ii->second);

	return nullptr;
}

InfoItem& CArchiveScanner::ArchiveData::EnsureInfoItem(const std::string& key)
{
	const std::string& keyLower = StringToLower(key);

	if (IsReservedKey(keyLower))
		throw content_error("You may not use key " + key + " in archive info, as it is reserved.");

	const auto ii = info.find(keyLower);
	if (ii == info.end()) {
		// add a new info-item
		InfoItem& infoItem = info[keyLower];
		infoItem.key = key;
		infoItem.valueType = INFO_VALUE_TYPE_INTEGER;
		infoItem.value.typeInteger = 0;
		return infoItem;
	}

	return ii->second;
}

void CArchiveScanner::ArchiveData::SetInfoItemValueString(const std::string& key, const std::string& value)
{
	InfoItem& infoItem = EnsureInfoItem(key);
	infoItem.valueType = INFO_VALUE_TYPE_STRING;
	infoItem.valueTypeString = value;
}

void CArchiveScanner::ArchiveData::SetInfoItemValueInteger(const std::string& key, int value)
{
	InfoItem& infoItem = EnsureInfoItem(key);
	infoItem.valueType = INFO_VALUE_TYPE_INTEGER;
	infoItem.value.typeInteger = value;
}

void CArchiveScanner::ArchiveData::SetInfoItemValueFloat(const std::string& key, float value)
{
	InfoItem& infoItem = EnsureInfoItem(key);
	infoItem.valueType = INFO_VALUE_TYPE_FLOAT;
	infoItem.value.typeFloat = value;
}

void CArchiveScanner::ArchiveData::SetInfoItemValueBool(const std::string& key, bool value)
{
	InfoItem& infoItem = EnsureInfoItem(key);
	infoItem.valueType = INFO_VALUE_TYPE_BOOL;
	infoItem.value.typeBool = value;
}


std::vector<InfoItem> CArchiveScanner::ArchiveData::GetInfoItems() const
{
	std::vector<InfoItem> infoItems;

	for (auto i = info.cbegin(); i != info.cend(); ++i) {
		infoItems.push_back(i->second);
		infoItems.at(infoItems.size() - 1).desc = GetKeyDescription(i->first);
	}

	return infoItems;
}


std::string CArchiveScanner::ArchiveData::GetInfoValueString(const std::string& key) const
{
	const InfoItem* infoItem = GetInfoItem(key);

	if (infoItem != nullptr) {
		if (infoItem->valueType == INFO_VALUE_TYPE_STRING)
			return infoItem->valueTypeString;

		return (infoItem->GetValueAsString());
	}

	return "";
}

int CArchiveScanner::ArchiveData::GetInfoValueInteger(const std::string& key) const
{
	const InfoItem* infoItem = GetInfoItem(key);

	if ((infoItem != nullptr) && (infoItem->valueType == INFO_VALUE_TYPE_INTEGER))
		return (infoItem->value.typeInteger);

	return 0;
}

float CArchiveScanner::ArchiveData::GetInfoValueFloat(const std::string& key) const
{
	const InfoItem* infoItem = GetInfoItem(key);

	if ((infoItem != nullptr) && (infoItem->valueType == INFO_VALUE_TYPE_FLOAT))
		return (infoItem->value.typeFloat);

	return 0.0f;
}

bool CArchiveScanner::ArchiveData::GetInfoValueBool(const std::string& key) const
{
	const InfoItem* infoItem = GetInfoItem(key);

	if ((infoItem != nullptr) && (infoItem->valueType == INFO_VALUE_TYPE_BOOL))
		return (infoItem->value.typeBool);

	return false;
}



spring::recursive_mutex mutex;

/*
 * CArchiveScanner
 */

CArchiveScanner::CArchiveScanner()
: isDirty(false)
{

	// the "cache" dir is created in DataDirLocater
	cachefile = FileSystem::EnsurePathSepAtEnd(FileSystem::GetCacheDir()) + IntToString(INTERNAL_VER, "ArchiveCache%i.lua");
	ReadCacheData(GetFilepath());
	ScanAllDirs();
}


CArchiveScanner::~CArchiveScanner()
{
	if (isDirty) {
		WriteCacheData(GetFilepath());
	}
}


void CArchiveScanner::ScanAllDirs()
{
	std::lock_guard<spring::recursive_mutex> lck(mutex);
	const std::vector<std::string>& datadirs = dataDirLocater.GetDataDirPaths();
	std::vector<std::string> scanDirs;
	scanDirs.reserve(datadirs.size());
	for (auto d = datadirs.rbegin(); d != datadirs.rend(); ++d) {
		scanDirs.push_back(*d + "maps");
		scanDirs.push_back(*d + "base");
		scanDirs.push_back(*d + "games");
		scanDirs.push_back(*d + "packages");
	}
	// ArchiveCache has been parsed at this point --> archiveInfos is populated
#if !defined(DEDICATED) && !defined(UNITSYNC)
	ScopedOnceTimer foo("CArchiveScanner");
#endif
	ScanDirs(scanDirs);
	WriteCacheData(GetFilepath());
}


void CArchiveScanner::ScanDirs(const std::vector<std::string>& scanDirs)
{
	std::lock_guard<spring::recursive_mutex> lck(mutex);
	std::deque<std::string> foundArchives;

	isDirty = true;

	// scan for all archives
	for (const std::string& dir: scanDirs) {
		if (FileSystem::DirExists(dir)) {
			LOG("Scanning: %s", dir.c_str());
			ScanDir(dir, foundArchives);
		}
	}

	// check for duplicates reached by links
	//XXX too slow also ScanArchive() skips duplicates itself, too
	/*for (auto it = foundArchives.begin(); it != foundArchives.end(); ++it) {
		auto jt = it;
		++jt;
		while (jt != foundArchives.end()) {
			std::string f1 = StringToLower(FileSystem::GetFilename(*it));
			std::string f2 = StringToLower(FileSystem::GetFilename(*jt));
			if ((f1 == f2) || FileSystem::ComparePaths(*it, *jt)) {
				jt = foundArchives.erase(jt);
			} else {
				++jt;
			}
		}
	}*/

	// Create archiveInfos etc. if not in cache already
	for (const std::string& archive: foundArchives) {
		ScanArchive(archive, false);
	#if !defined(DEDICATED) && !defined(UNITSYNC)
		Watchdog::ClearTimer(WDT_MAIN);
	#endif
	}

	// Now we'll have to parse the replaces-stuff found in the mods
	for (auto& aii: archiveInfos) {
		for (const std::string& replaceName: aii.second.archiveData.GetReplaces()) {
			// Overwrite the info for this archive with a replaced pointer
			const std::string& lcname = StringToLower(replaceName);
			ArchiveInfo& ai = archiveInfos[lcname];
			ai.path = "";
			ai.origName = lcname;
			ai.modified = 1;
			ai.archiveData = ArchiveData();
			ai.updated = true;
			ai.replaced = aii.first;
		}
	}
}


void CArchiveScanner::ScanDir(const std::string& curPath, std::deque<std::string>& foundArchives)
{
	std::deque<std::string> subDirs = {curPath};

	while (!subDirs.empty()) {
		const std::string& subDir = FileSystem::EnsurePathSepAtEnd(subDirs.front());
		const std::vector<std::string>& foundFiles = dataDirsAccess.FindFiles(subDir, "*", FileQueryFlags::INCLUDE_DIRS);

		subDirs.pop_front();

		for (const std::string& fileName: foundFiles) {
			const std::string& fileNameNoSep = FileSystem::EnsureNoPathSepAtEnd(fileName);
			const std::string& lcFilePath = StringToLower(FileSystem::GetDirectory(fileNameNoSep));

			// Exclude archive files found inside directory archives (.sdd)
			if (lcFilePath.find(".sdd") != std::string::npos)
				continue;

			// Is this an archive we should look into?
			if (archiveLoader.IsArchiveFile(fileNameNoSep)) {
				foundArchives.push_front(fileNameNoSep); // push in reverse order!
				continue;
			}
			if (FileSystem::DirExists(fileNameNoSep)) {
				subDirs.push_back(fileNameNoSep);
			}
		}
	}
}

static void AddDependency(std::vector<std::string>& deps, const std::string& dependency)
{
	VectorInsertUnique(deps, dependency, true);
}

bool CArchiveScanner::CheckCompression(const IArchive* ar, const std::string& fullName, std::string& error)
{
	if (!ar->CheckForSolid())
		return true;

	for (unsigned fid = 0; fid != ar->NumFiles(); ++fid) {
		if (ar->HasLowReadingCost(fid))
			continue;

		const std::pair<std::string, int>& info = ar->FileInfo(fid);

		switch (GetMetaFileClass(StringToLower(info.first))) {
			case 1: {
				error += "reading primary meta-file " + info.first + " too expensive; ";
				error += "please repack this archive with non-solid compression";
				return false;
			} break;
			case 2: {
				LOG_SL(LOG_SECTION_ARCHIVESCANNER, L_WARNING, "Archive %s: reading secondary meta-file %s too expensive", fullName.c_str(), info.first.c_str());
			} break;
			case 0:
			default: {
				continue;
			} break;
		}
	}

	return true;
}

std::string CArchiveScanner::SearchMapFile(const IArchive* ar, std::string& error)
{
	assert(ar != nullptr);

	// check for smf/sm3 and if the uncompression of important files is too costy
	for (unsigned fid = 0; fid != ar->NumFiles(); ++fid) {
		const std::pair<std::string, int>& info = ar->FileInfo(fid);
		const std::string& ext = FileSystem::GetExtension(StringToLower(info.first));

		if ((ext == "smf") || (ext == "sm3")) {
			return info.first;
		}
	}

	return "";
}

void CArchiveScanner::ScanArchive(const std::string& fullName, bool doChecksum)
{
	unsigned modifiedTime = 0;
	if (CheckCachedData(fullName, &modifiedTime, doChecksum))
		return;
	isDirty = true;

	const std::string& fn    = FileSystem::GetFilename(fullName);
	const std::string& fpath = FileSystem::GetDirectory(fullName);
	const std::string& lcfn  = StringToLower(fn);

	std::unique_ptr<IArchive> ar(archiveLoader.OpenArchive(fullName));
	if (ar == nullptr || !ar->IsOpen()) {
		LOG_L(L_WARNING, "Unable to open archive: %s", fullName.c_str());

		// record it as broken, so we don't need to look inside everytime
		BrokenArchive& ba = brokenArchives[lcfn];
		ba.path = fpath;
		ba.modified = modifiedTime;
		ba.updated = true;
		ba.problem = "Unable to open archive";
		return;
	}

	std::string error;
	std::string mapfile;

	const bool hasModinfo = ar->FileExists("modinfo.lua");
	const bool hasMapinfo = ar->FileExists("mapinfo.lua");


	ArchiveInfo ai;
	ArchiveData& ad = ai.archiveData;

	if (hasMapinfo) {
		ScanArchiveLua(ar.get(), "mapinfo.lua", ai, error);
		if (ad.GetMapFile().empty()) {
			LOG_L(L_WARNING, "%s: mapfile isn't set in mapinfo.lua, please set it for faster loading!", fullName.c_str());
			mapfile = SearchMapFile(ar.get(), error);
		}
	} else if (hasModinfo) {
		ScanArchiveLua(ar.get(), "modinfo.lua", ai, error);
	} else {
		mapfile = SearchMapFile(ar.get(), error);
	}

	if (!CheckCompression(ar.get(), fullName, error)) {
		// for some reason, the archive is marked as broken
		LOG_L(L_WARNING, "Failed to scan %s (%s)", fullName.c_str(), error.c_str());

		// record it as broken, so we don't need to look inside everytime
		BrokenArchive& ba = brokenArchives[lcfn];
		ba.path = fpath;
		ba.modified = modifiedTime;
		ba.updated = true;
		ba.problem = error;
		return;
	}

	if (hasMapinfo || !mapfile.empty()) {
		// map archive
		if (ad.GetName().empty()) {
			// FIXME The name will never be empty, if version is set (see HACK in ArchiveData)
			ad.SetInfoItemValueString("name_pure", FileSystem::GetBasename(mapfile));
			ad.SetInfoItemValueString("name", FileSystem::GetBasename(mapfile));
		}

		if (ad.GetMapFile().empty())
			ad.SetInfoItemValueString("mapfile", mapfile);

		AddDependency(ad.GetDependencies(), GetMapHelperContentName());
		ad.SetInfoItemValueInteger("modType", modtype::map);

		LOG_S(LOG_SECTION_ARCHIVESCANNER, "Found new map: %s", ad.GetNameVersioned().c_str());
	} else if (hasModinfo) {
		// game or base-type (cursors, bitmaps, ...) archive
		// babysitting like this is really no longer required
		if (ad.IsGame() || ad.IsMenu())
			AddDependency(ad.GetDependencies(), GetSpringBaseContentName());

		LOG_S(LOG_SECTION_ARCHIVESCANNER, "Found new game: %s", ad.GetNameVersioned().c_str());
	} else {
		// neither a map nor a mod: error
		error = "missing modinfo.lua/mapinfo.lua";
	}

	ai.path = fpath;
	ai.modified = modifiedTime;
	ai.origName = fn;
	ai.updated = true;
	ai.checksum = (doChecksum) ? GetCRC(fullName) : 0;
	archiveInfos[lcfn] = ai;
}


bool CArchiveScanner::CheckCachedData(const std::string& fullName, unsigned* modified, bool doChecksum)
{
	// If stat fails, assume the archive is not broken nor cached
	if ((*modified = FileSystemAbstraction::GetFileModificationTime(fullName)) == 0)
		return false;

	const std::string& fn    = FileSystem::GetFilename(fullName);
	const std::string& fpath = FileSystem::GetDirectory(fullName);
	const std::string& lcfn  = StringToLower(fn);


	// Determine whether this archive has earlier be found to be broken
	auto bai = brokenArchives.find(lcfn);

	if (bai != brokenArchives.end()) {
		BrokenArchive& ba = bai->second;

		if (*modified == ba.modified && fpath == ba.path) {
			return (ba.updated = true);
		}
	}


	// Determine whether to rely on the cached info or not
	auto aii = archiveInfos.find(lcfn);

	if (aii != archiveInfos.end()) {
		ArchiveInfo& ai = aii->second;

		// This archive may have been obsoleted, do not process it if so
		if (!ai.replaced.empty())
			return true;

		if (*modified == ai.modified && fpath == ai.path) {
			// cache found update checksum if wanted
			ai.updated = true;

			if (doChecksum && (ai.checksum == 0))
				ai.checksum = GetCRC(fullName);

			return true;
		}

		if (ai.updated) {
			LOG_L(L_ERROR, "Found a \"%s\" already in \"%s\", ignoring.", fullName.c_str(), (ai.path + ai.origName).c_str());

			if (baseContentArchives.find(aii->first) == baseContentArchives.end())
				return true; // ignore

			throw user_error(
				std::string("duplicate base content detected:\n\t") + ai.path +
				std::string("\n\t") + fpath +
				std::string("\nPlease fix your configuration/installation as this can cause desyncs!"));
		}

		// If we are here, we could have invalid info in the cache
		// Force a reread if it is a directory archive (.sdd), as
		// st_mtime only reflects changes to the directory itself,
		// not the contents.
		archiveInfos.erase(aii);
	}

	return false;
}


bool CArchiveScanner::ScanArchiveLua(IArchive* ar, const std::string& fileName, ArchiveInfo& ai, std::string& err)
{
	std::vector<std::uint8_t> buf;
	if (!ar->GetFile(fileName, buf) || buf.empty()) {
		err = "Error reading " + fileName;
		if (ar->GetArchiveName().find(".sdp") != std::string::npos)
			err += " (archive's rapid tag: " + GetRapidTagFromPackage(FileSystem::GetBasename(ar->GetArchiveName())) + ")";

		return false;
	}
	LuaParser p(std::string((char*)(&buf[0]), buf.size()), SPRING_VFS_ZIP);
	if (!p.Execute()) {
		err = "Error in " + fileName + ": " + p.GetErrorLog();
		return false;
	}

	try {
		ai.archiveData = CArchiveScanner::ArchiveData(p.GetRoot(), false);

		if (!ai.archiveData.IsValid(err)) {
			err = "Error in " + fileName + ": " + err;
			return false;
		}
	} catch (const content_error& contErr) {
		err = "Error in " + fileName + ": " + contErr.what();
		return false;
	}

	return true;
}

IFileFilter* CArchiveScanner::CreateIgnoreFilter(IArchive* ar)
{
	IFileFilter* ignore = IFileFilter::Create();
	std::vector<std::uint8_t> buf;
	if (ar->GetFile("springignore.txt", buf) && !buf.empty()) {
		// this automatically splits lines
		ignore->AddRule(std::string((char*)(&buf[0]), buf.size()));
	}
	return ignore;
}



/**
 * Get CRC of the data in the specified archive.
 * Returns 0 if file could not be opened.
 */
unsigned int CArchiveScanner::GetCRC(const std::string& arcName)
{
	CRC crc;

	struct CRCPair {
		std::string* filename;
		unsigned int nameCRC;
		unsigned int dataCRC;
	};

	// try to open an archive
	std::unique_ptr<IArchive> ar(archiveLoader.OpenArchive(arcName));

	if (ar == nullptr)
		return 0;

	// load ignore list, and insert all files to check in lowercase format
	std::unique_ptr<IFileFilter> ignore(CreateIgnoreFilter(ar.get()));
	std::vector<std::string> files;
	std::vector<CRCPair> crcs;

	files.reserve(ar->NumFiles());
	crcs.reserve(ar->NumFiles());

	for (unsigned fid = 0; fid != ar->NumFiles(); ++fid) {
		const std::pair<std::string, int>& info = ar->FileInfo(fid);

		if (ignore->Match(info.first))
			continue;

		// create case-insensitive hashes
		files.push_back(StringToLower(info.first));
	}

	// sort by filename
	std::stable_sort(files.begin(), files.end());

	for (std::string& f: files) {
		crcs.push_back(CRCPair{&f, 0, 0});
	}

	// compute CRCs of the files
	// Hint: Multithreading only speedups `.sdd` loading. For those the CRC generation is extremely slow -
	//       it has to load the full file to calc it! For the other formats (sd7, sdz, sdp) the CRC is saved
	//       in the metainformation of the container and so the loading is much faster. Neither does any of our
	//       current (2011) packing libraries support multithreading :/
	for_mt(0, crcs.size(), [&](const int i) {
		CRCPair& crcp = crcs[i];
		assert(crcp.filename == &files[i]);
		const unsigned int nameCRC = CRC::GetCRC(crcp.filename->data(), crcp.filename->size());
		const unsigned fid = ar->FindFile(*crcp.filename);
		const unsigned int dataCRC = ar->GetCrc32(fid);
		crcp.nameCRC = nameCRC;
		crcp.dataCRC = dataCRC;
	#if !defined(DEDICATED) && !defined(UNITSYNC)
		Watchdog::ClearTimer(WDT_MAIN);
	#endif
	});

	// Add file CRCs to the main archive CRC
	for (const CRCPair& crcp: crcs) {
		crc.Update(crcp.nameCRC);
		crc.Update(crcp.dataCRC);
	#if !defined(DEDICATED) && !defined(UNITSYNC)
		Watchdog::ClearTimer();
	#endif
	}

	// A value of 0 is used to indicate no crc.. so never return that
	// Shouldn't happen all that often
	const unsigned int digest = crc.GetDigest();
	return (digest == 0)? 4711: digest;
}


void CArchiveScanner::ComputeChecksumForArchive(const std::string& filePath)
{
	ScanArchive(filePath, true);
}


void CArchiveScanner::ReadCacheData(const std::string& filename)
{
	std::lock_guard<spring::recursive_mutex> lck(mutex);
	if (!FileSystem::FileExists(filename)) {
		LOG_L(L_INFO, "ArchiveCache %s doesn't exist", filename.c_str());
		return;
	}

	LuaParser p(filename, SPRING_VFS_RAW, SPRING_VFS_BASE);
	if (!p.Execute()) {
		LOG_L(L_ERROR, "Failed to parse ArchiveCache: %s", p.GetErrorLog().c_str());
		return;
	}

	const LuaTable& archiveCache = p.GetRoot();
	const LuaTable& archives = archiveCache.SubTable("archives");
	const LuaTable& brokenArchives = archiveCache.SubTable("brokenArchives");

	// Do not load old version caches
	const int ver = archiveCache.GetInt("internalVer", (INTERNAL_VER + 1));
	if (ver != INTERNAL_VER)
		return;

	for (int i = 1; archives.KeyExists(i); ++i) {
		const LuaTable& curArchive = archives.SubTable(i);
		const LuaTable& archived = curArchive.SubTable("archivedata");
		std::string name = curArchive.GetString("name", "");

		ArchiveInfo& ai = archiveInfos[StringToLower(name)];
		ai.origName = name;
		ai.path     = curArchive.GetString("path", "");

		// do not use LuaTable.GetInt() for 32-bit integers, the Spring lua
		// library uses 32-bit floats to represent numbers, which can only
		// represent 2^24 consecutive integers
		ai.modified = strtoul(curArchive.GetString("modified", "0").c_str(), 0, 10);
		ai.checksum = strtoul(curArchive.GetString("checksum", "0").c_str(), 0, 10);
		ai.updated = false;

		ai.archiveData = CArchiveScanner::ArchiveData(archived, true);
		if (ai.archiveData.IsMap()) {
			AddDependency(ai.archiveData.GetDependencies(), GetMapHelperContentName());
		} else if (ai.archiveData.IsGame()) {
			AddDependency(ai.archiveData.GetDependencies(), GetSpringBaseContentName());
		}
	}

	for (int i = 1; brokenArchives.KeyExists(i); ++i) {
		const LuaTable& curArchive = brokenArchives.SubTable(i);
		const std::string& name = StringToLower(curArchive.GetString("name", ""));

		BrokenArchive& ba = this->brokenArchives[name];
		ba.path = curArchive.GetString("path", "");
		ba.modified = strtoul(curArchive.GetString("modified", "0").c_str(), 0, 10);
		ba.updated = false;
		ba.problem = curArchive.GetString("problem", "unknown");
	}

	isDirty = false;
}

static inline void SafeStr(FILE* out, const char* prefix, const std::string& str)
{
	if (str.empty())
		return;

	if ( (str.find_first_of("\\\"") != std::string::npos) || (str.find_first_of("\n") != std::string::npos )) {
		fprintf(out, "%s[[%s]],\n", prefix, str.c_str());
	} else {
		fprintf(out, "%s\"%s\",\n", prefix, str.c_str());
	}
}

void FilterDep(std::vector<std::string>& deps, const std::string& exclude)
{
	auto it = std::remove_if(deps.begin(), deps.end(), [&](const std::string& dep) { return (dep == exclude); });
	deps.erase(it, deps.end());
}

void CArchiveScanner::WriteCacheData(const std::string& filename)
{
	std::lock_guard<spring::recursive_mutex> lck(mutex);
	if (!isDirty)
		return;

	FILE* out = fopen(filename.c_str(), "wt");
	if (out == nullptr) {
		LOG_L(L_ERROR, "Failed to write to \"%s\"!", filename.c_str());
		return;
	}

	// First delete all outdated information
	spring::map_erase_if(archiveInfos, [](const decltype(archiveInfos)::value_type& p) {
		return !p.second.updated;
	});
	spring::map_erase_if(brokenArchives, [](const decltype(brokenArchives)::value_type& p) {
		return !p.second.updated;
	});

	fprintf(out, "local archiveCache = {\n\n");
	fprintf(out, "\tinternalver = %i,\n\n", INTERNAL_VER);
	fprintf(out, "\tarchives = {  -- count = %u\n", unsigned(archiveInfos.size()));

	for (const auto& arcIt: archiveInfos) {
		const ArchiveInfo& arcInfo = arcIt.second;

		fprintf(out, "\t\t{\n");
		SafeStr(out, "\t\t\tname = ",              arcInfo.origName);
		SafeStr(out, "\t\t\tpath = ",              arcInfo.path);
		fprintf(out, "\t\t\tmodified = \"%u\",\n", arcInfo.modified);
		fprintf(out, "\t\t\tchecksum = \"%u\",\n", arcInfo.checksum);
		SafeStr(out, "\t\t\treplaced = ",          arcInfo.replaced);

		// mod info?
		const ArchiveData& archData = arcInfo.archiveData;
		if (!archData.GetName().empty()) {
			fprintf(out, "\t\t\tarchivedata = {\n");

			for (const auto& ii: archData.GetInfo()) {
				if (ii.second.valueType == INFO_VALUE_TYPE_STRING) {
					SafeStr(out, std::string("\t\t\t\t" + ii.first + " = ").c_str(), ii.second.valueTypeString);
				} else {
					fprintf(out, "\t\t\t\t%s = %s,\n", ii.first.c_str(), ii.second.GetValueAsString(false).c_str());
				}
			}

			std::vector<std::string> deps = archData.GetDependencies();
			if (archData.IsMap()) {
				FilterDep(deps, GetMapHelperContentName());
			} else if (archData.IsGame()) {
				FilterDep(deps, GetSpringBaseContentName());
			}

			if (!deps.empty()) {
				fprintf(out, "\t\t\t\tdepend = {\n");
				for (unsigned d = 0; d < deps.size(); d++) {
					SafeStr(out, "\t\t\t\t\t", deps[d]);
				}
				fprintf(out, "\t\t\t\t},\n");
			}
			fprintf(out, "\t\t\t},\n");
		}

		fprintf(out, "\t\t},\n");
	}

	fprintf(out, "\t},\n\n"); // close 'archives'
	fprintf(out, "\tbrokenArchives = {  -- count = %u\n", unsigned(brokenArchives.size()));

	for (const auto& bai: brokenArchives) {
		const BrokenArchive& ba = bai.second;

		fprintf(out, "\t\t{\n");
		SafeStr(out, "\t\t\tname = ", bai.first);
		SafeStr(out, "\t\t\tpath = ", ba.path);
		fprintf(out, "\t\t\tmodified = \"%u\",\n", ba.modified);
		SafeStr(out, "\t\t\tproblem = ", ba.problem);
		fprintf(out, "\t\t},\n");
	}

	fprintf(out, "\t},\n"); // close 'brokenArchives'
	fprintf(out, "}\n\n"); // close 'archiveCache'
	fprintf(out, "return archiveCache\n");

	if (fclose(out) == EOF)
		LOG_L(L_ERROR, "Failed to write to \"%s\"!", filename.c_str());

	isDirty = false;
}


static void sortByName(std::vector<CArchiveScanner::ArchiveData>& data)
{
	std::stable_sort(data.begin(), data.end(), [](const CArchiveScanner::ArchiveData& a, const CArchiveScanner::ArchiveData& b) {
		return (a.GetNameVersioned() < b.GetNameVersioned());
	});
}

std::vector<CArchiveScanner::ArchiveData> CArchiveScanner::GetPrimaryMods() const
{
	std::vector<ArchiveData> ret;

	for (auto i = archiveInfos.cbegin(); i != archiveInfos.cend(); ++i) {
		const ArchiveData& aid = i->second.archiveData;
		if ((!aid.GetName().empty()) && (aid.GetModType() == modtype::primary)) {
			// Add the archive the mod is in as the first dependency
			ArchiveData md = aid;
			md.GetDependencies().insert(md.GetDependencies().begin(), i->second.origName);
			ret.push_back(md);
		}
	}

	sortByName(ret);
	return ret;
}


std::vector<CArchiveScanner::ArchiveData> CArchiveScanner::GetAllMods() const
{
	std::vector<ArchiveData> ret;

	for (auto i = archiveInfos.cbegin(); i != archiveInfos.cend(); ++i) {
		const ArchiveData& aid = i->second.archiveData;
		if ((!aid.GetName().empty()) && aid.IsGame()) {
			// Add the archive the mod is in as the first dependency
			ArchiveData md = aid;
			md.GetDependencies().insert(md.GetDependencies().begin(), i->second.origName);
			ret.push_back(md);
		}
	}

	sortByName(ret);
	return ret;
}


std::vector<CArchiveScanner::ArchiveData> CArchiveScanner::GetAllArchives() const
{
	std::vector<ArchiveData> ret;

	for (const auto& pair: archiveInfos) {
		const ArchiveData& aid = pair.second.archiveData;

		// Add the archive the mod is in as the first dependency
		ArchiveData md = aid;
		md.GetDependencies().insert(md.GetDependencies().begin(), pair.second.origName);
		ret.push_back(md);
	}

	sortByName(ret);
	return ret;
}

std::vector<std::string> CArchiveScanner::GetAllArchivesUsedBy(const std::string& rootArchive) const
{
	LOG_S(LOG_SECTION_ARCHIVESCANNER, "GetArchives: %s", rootArchive.c_str());

	// VectorInsertUnique'ing via AddDependency can become a performance hog
	// for very long dependency chains, prefer to sort and remove duplicates
	std::vector<std::string> retArchives;
	std::vector<std::string> tmpArchives;
	std::deque<std::string> archiveQueue = {rootArchive};

	retArchives.reserve(8);
	tmpArchives.reserve(8);

	while (!archiveQueue.empty()) {
		// protect against circular dependencies; worst case is if all archives form one huge chain
		if (archiveQueue.size() > archiveInfos.size())
			break;

		const std::string& resolvedName = ArchiveNameResolver::GetGame(archiveQueue.front());
		const std::string& lowerCaseName = StringToLower(ArchiveFromName(resolvedName));

		archiveQueue.pop_front();

		const ArchiveInfo* archiveInfo = nullptr;

		const auto CanAddSubDependencies = [&](const std::string& lwrCaseName) -> const ArchiveInfo* {
			#ifdef UNITSYNC
			// add unresolved deps for unitsync so it still shows this file
			const auto HandleUnresolvedDep = [&tmpArchives](const std::string& archName) { tmpArchives.push_back(archName); return true; };
			#else
			const auto HandleUnresolvedDep = [&tmpArchives](const std::string& archName) { (void) archName; return false; };
			#endif

			auto aii = archiveInfos.find(lwrCaseName);
			auto aij = aii;

			const ArchiveInfo* ai = nullptr;

			if (aii == archiveInfos.end()) {
				if (!HandleUnresolvedDep(lwrCaseName))
					throw content_error("Archive \"" + lwrCaseName + "\" not found");

				return nullptr;
			}

			ai = &aii->second;

			// check if this archive has an unresolved replacement
			while (!ai->replaced.empty()) {
				if ((aii = archiveInfos.find(ai->replaced)) == archiveInfos.end()) {
					if (!HandleUnresolvedDep(lwrCaseName))
						throw content_error("Replacement \"" + ai->replaced + "\" for archive \"" + lwrCaseName + "\" not found");

					return nullptr;
				}

				aij = aii;
				ai = &aij->second;
			}

			return ai;
		};


		if ((archiveInfo = CanAddSubDependencies(lowerCaseName)) == nullptr)
			continue;

		tmpArchives.push_back(archiveInfo->archiveData.GetNameVersioned());

		// expand dependencies in depth-first order
		for (const std::string& archiveDep: archiveInfo->archiveData.GetDependencies()) {
			assert(archiveDep != rootArchive);
			assert(archiveDep != tmpArchives.back());
			archiveQueue.push_front(archiveDep);
		}
	}

	std::stable_sort(tmpArchives.begin(), tmpArchives.end());

	for (std::string& archiveName: tmpArchives) {
		if (retArchives.empty() || archiveName != retArchives.back()) {
			retArchives.emplace_back(std::move(archiveName));
		}
	}

	return retArchives;
}


std::vector<std::string> CArchiveScanner::GetMaps() const
{
	std::vector<std::string> ret;

	for (const auto& p: archiveInfos) {
		const ArchiveInfo& ai = p.second;
		const ArchiveData& ad = ai.archiveData;
		if (!(ad.GetName().empty()) && ad.IsMap())
			ret.push_back(ad.GetNameVersioned());
	}

	return ret;
}

std::string CArchiveScanner::MapNameToMapFile(const std::string& s) const
{
	// Convert map name to map archive
	const auto pred = [&s](const decltype(archiveInfos)::value_type& p) { return ((p.second).archiveData.GetNameVersioned() == s); };
	const auto iter = std::find_if(archiveInfos.cbegin(), archiveInfos.cend(), pred);

	if (iter != archiveInfos.cend())
		return ((iter->second).archiveData.GetMapFile());

	LOG_SL(LOG_SECTION_ARCHIVESCANNER, L_WARNING, "map file of %s not found", s.c_str());
	return s;
}

unsigned int CArchiveScanner::GetSingleArchiveChecksum(const std::string& filePath)
{
	ComputeChecksumForArchive(filePath);
	std::string lcname = FileSystem::GetFilename(filePath);
	StringToLowerInPlace(lcname);

	const auto aii = archiveInfos.find(lcname);
	if (aii == archiveInfos.end()) {
		LOG_SL(LOG_SECTION_ARCHIVESCANNER, L_WARNING, "%s checksum: not found (0)", filePath.c_str());
		return 0;
	}

	LOG_S(LOG_SECTION_ARCHIVESCANNER,"%s checksum: %d/%u", filePath.c_str(), aii->second.checksum, aii->second.checksum);
	return aii->second.checksum;
}

unsigned int CArchiveScanner::GetArchiveCompleteChecksum(const std::string& name)
{
	const std::vector<std::string> ars = GetAllArchivesUsedBy(name);

	unsigned int checksum = 0;

	for (const std::string& depName: ars) {
		const std::string& archive = ArchiveFromName(depName);
		checksum ^= GetSingleArchiveChecksum(GetArchivePath(archive) + archive);
	}
	LOG_S(LOG_SECTION_ARCHIVESCANNER, "archive checksum %s: %d/%u", name.c_str(), checksum, checksum);
	return checksum;
}

void CArchiveScanner::CheckArchive(const std::string& name, unsigned checksum)
{
	unsigned localChecksum = GetArchiveCompleteChecksum(name);

	if (localChecksum != checksum) {
		char msg[1024];
		sprintf(
			msg,
			"Checksum of %s (checksum 0x%x) differs from the host's copy (checksum 0x%x). "
			"This may be caused by a corrupted download or there may even "
			"be 2 different versions in circulation. Make sure you and the host have installed "
			"the chosen archive and its dependencies and consider redownloading it.",
			name.c_str(), localChecksum, checksum);

		throw content_error(msg);
	}
}

std::string CArchiveScanner::GetArchivePath(const std::string& name) const
{
	const auto aii = archiveInfos.find(StringToLower(FileSystem::GetFilename(name)));

	if (aii == archiveInfos.end())
		return "";

	return aii->second.path;
}

std::string CArchiveScanner::NameFromArchive(const std::string& archiveName) const
{
	const auto aii = archiveInfos.find(StringToLower(archiveName));

	if (aii != archiveInfos.end())
		return ((aii->second).archiveData.GetNameVersioned());

	return archiveName;
}


std::string CArchiveScanner::ArchiveFromName(const std::string& name) const
{
	// std::pair<std::string, ArchiveInfo>
	const auto pred = [&name](const decltype(archiveInfos)::value_type& p) { return ((p.second).archiveData.GetNameVersioned() == name); };
	const auto iter = std::find_if(archiveInfos.cbegin(), archiveInfos.cend(), pred);

	if (iter != archiveInfos.cend())
		return (iter->second).origName;

	return name;
}

CArchiveScanner::ArchiveData CArchiveScanner::GetArchiveData(const std::string& name) const
{
	const auto pred = [&name](const decltype(archiveInfos)::value_type& p) { return ((p.second).archiveData.GetNameVersioned() == name); };
	const auto iter = std::find_if(archiveInfos.cbegin(), archiveInfos.cend(), pred);

	if (iter != archiveInfos.cend())
		return (iter->second).archiveData;

	return ArchiveData();
}


CArchiveScanner::ArchiveData CArchiveScanner::GetArchiveDataByArchive(const std::string& archive) const
{
	const auto aii = archiveInfos.find(StringToLower(archive));

	if (aii != archiveInfos.end())
		return aii->second.archiveData;

	return ArchiveData();
}

int CArchiveScanner::GetMetaFileClass(const std::string& filePath)
{
	const std::string& lowerFilePath = StringToLower(filePath);
	// const std::string& ext = FileSystem::GetExtension(lowerFilePath);
	const auto it = metaFileClasses.find(lowerFilePath);

	if (it != metaFileClasses.end())
		return (it->second);

//	if ((ext == "smf") || (ext == "sm3")) // to generate minimap
//		return 1;

	for (const auto& p: metaDirClasses) {
		if (StringStartsWith(lowerFilePath, p.first))
			return (p.second);
	}

	return 0;
}

