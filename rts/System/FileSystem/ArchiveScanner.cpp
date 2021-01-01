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
#include "Archives/DirArchive.h"
#include "FileFilter.h"
#include "DataDirsAccess.h"
#include "FileSystem.h"
#include "FileQueryFlags.h"
#include "Lua/LuaParser.h"
#include "System/ContainerUtil.h"
#include "System/StringUtil.h"
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

constexpr static int INTERNAL_VER = 16;


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
	KnownInfoTag{"mapfile",     "in case its a map, store location of smf file",                  false}, //FIXME is this ever used in the engine?! or does it auto calc the location?
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


CArchiveScanner* archiveScanner = nullptr;


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
	//
	// NOTE when changing this, this function is used both by the code that
	// reads ArchiveCache.lua and the code that reads modinfo.lua from the mod.
	// so make sure it doesn't keep adding stuff to the name everytime
	// Spring/unitsync is loaded.
	//
	const std::string& name = GetNameVersioned();
	const std::string& version = GetVersion();

	if (!version.empty()) {
		if (name.find(version) == std::string::npos) {
			SetInfoItemValueString("name", name + " " + version);
		} else if (!fromCache) {
			LOG_L(L_WARNING, "[%s] version \"%s\" included in name \"%s\"", __func__, version.c_str(), name.c_str());
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
	using P = decltype(infoItems)::value_type;

	const auto HasInfoItem = [this](const std::string& name) {
		const auto pred = [](const P& a, const P& b) { return (a.first < b.first); };
		const auto iter = std::lower_bound(infoItems.begin(), infoItems.end(), decltype(infoItems)::value_type{name, {}}, pred);
		return (iter != infoItems.end() && iter->first == name);
	};

	// look for any info-item that is required but not present
	// datums containing only optional items ("shortgame", etc)
	// are allowed
	const auto pred = [&](const KnownInfoTag& tag) { return (tag.required && !HasInfoItem(tag.name)); };
	const auto iter = std::find_if(knownTags.cbegin(), knownTags.cend(), pred);

	if (iter == knownTags.cend())
		return true;

	err = "Missing required tag \"" + iter->name + "\".";
	return false;
}



const InfoItem* CArchiveScanner::ArchiveData::GetInfoItem(const std::string& key) const
{
	using P = decltype(infoItems)::value_type;

	const std::string& lcKey = StringToLower(key);

	const auto pred = [](const P& a, const P& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(infoItems.begin(), infoItems.end(), P{lcKey, {}}, pred);

	if (iter != infoItems.end() && iter->first == lcKey)
		return &(iter->second);

	return nullptr;
}



InfoItem& CArchiveScanner::ArchiveData::GetAddInfoItem(const std::string& key)
{
	const std::string& keyLower = StringToLower(key);

	if (IsReservedKey(keyLower))
		throw content_error("You may not use key " + key + " in archive info, as it is reserved.");

	using P = decltype(infoItems)::value_type;

	const auto pred = [](const P& a, const P& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(infoItems.begin(), infoItems.end(), P{keyLower, {}}, pred);

	if (iter == infoItems.end() || iter->first != keyLower) {
		// add a new info-item, sorted by lower-case key
		InfoItem infoItem;
		infoItem.key = key;
		infoItem.valueType = INFO_VALUE_TYPE_INTEGER;
		infoItem.value.typeInteger = 0;
		infoItems.emplace_back(keyLower, std::move(infoItem));

		// swap into position
		for (size_t i = infoItems.size() - 1; i > 0; i--) {
			if (infoItems[i - 1].first < infoItems[i].first)
				return (infoItems[i].second);

			std::swap(infoItems[i - 1], infoItems[i]);
		}

		return (infoItems[0].second);
	}

	return (iter->second);
}

void CArchiveScanner::ArchiveData::SetInfoItemValueString(const std::string& key, const std::string& value)
{
	InfoItem& infoItem = GetAddInfoItem(key);
	infoItem.valueType = INFO_VALUE_TYPE_STRING;
	infoItem.valueTypeString = value;
}

void CArchiveScanner::ArchiveData::SetInfoItemValueInteger(const std::string& key, int value)
{
	InfoItem& infoItem = GetAddInfoItem(key);
	infoItem.valueType = INFO_VALUE_TYPE_INTEGER;
	infoItem.value.typeInteger = value;
}

void CArchiveScanner::ArchiveData::SetInfoItemValueFloat(const std::string& key, float value)
{
	InfoItem& infoItem = GetAddInfoItem(key);
	infoItem.valueType = INFO_VALUE_TYPE_FLOAT;
	infoItem.value.typeFloat = value;
}

void CArchiveScanner::ArchiveData::SetInfoItemValueBool(const std::string& key, bool value)
{
	InfoItem& infoItem = GetAddInfoItem(key);
	infoItem.valueType = INFO_VALUE_TYPE_BOOL;
	infoItem.value.typeBool = value;
}


std::vector<InfoItem> CArchiveScanner::ArchiveData::GetInfoItems() const
{
	std::vector<InfoItem> retInfoItems;

	retInfoItems.reserve(infoItems.size());

	for (const auto& pair: infoItems) {
		retInfoItems.push_back(pair.second);
		retInfoItems.back().desc = GetKeyDescription(pair.first);
	}

	return retInfoItems;
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



static spring::recursive_mutex scannerMutex;
static std::atomic<uint32_t> numScannedArchives{0};


/*
 * CArchiveScanner
 */

CArchiveScanner::CArchiveScanner()
{
	Clear();
	// the "cache" dir is created in DataDirLocater
	ReadCacheData(cachefile = FileSystem::EnsurePathSepAtEnd(FileSystem::GetCacheDir()) + IntToString(INTERNAL_VER, "ArchiveCache%i.lua"));
	ScanAllDirs();
}


CArchiveScanner::~CArchiveScanner()
{
	if (!isDirty)
		return;

	WriteCacheData(GetFilepath());
}

uint32_t CArchiveScanner::GetNumScannedArchives()
{
	// needs to be a static since archiveScanner remains null until ctor returns
	return (numScannedArchives.load());
}


void CArchiveScanner::Clear()
{
	archiveInfos.clear();
	archiveInfos.reserve(256);
	archiveInfosIndex.clear();
	archiveInfosIndex.reserve(256);
	brokenArchives.clear();
	brokenArchives.reserve(16);
	brokenArchivesIndex.clear();
	brokenArchivesIndex.reserve(16);
	cachefile.clear();
}

void CArchiveScanner::Reload()
{
	// {Read,Write,Scan}* all grab this too but we need the entire reloading-sequence to appear atomic
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	// dtor
	if (isDirty)
		WriteCacheData(GetFilepath());

	// ctor
	Clear();
	ReadCacheData(cachefile = FileSystem::EnsurePathSepAtEnd(FileSystem::GetCacheDir()) + IntToString(INTERNAL_VER, "ArchiveCache%i.lua"));
	ScanAllDirs();
}

void CArchiveScanner::ScanAllDirs()
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	const std::vector<std::string>& dataDirPaths = dataDirLocater.GetDataDirPaths();
	const std::array<std::string, 5>& dataDirRoots = dataDirLocater.GetDataDirRoots();

	std::vector<std::string> scanDirs;
	scanDirs.reserve(dataDirPaths.size() * dataDirRoots.size());

	// last specified is first scanned
	for (auto d = dataDirPaths.rbegin(); d != dataDirPaths.rend(); ++d) {
		for (const std::string& s: dataDirRoots) {
			if (s.empty())
				continue;

			scanDirs.emplace_back(*d + s);
		}
	}

	// ArchiveCache has been parsed at this point --> archiveInfos is populated
#if !defined(DEDICATED) && !defined(UNITSYNC)
	ScopedOnceTimer foo("CArchiveScanner::ScanAllDirs");
#endif

	ScanDirs(scanDirs);
	WriteCacheData(GetFilepath());
}


void CArchiveScanner::ScanDirs(const std::vector<std::string>& scanDirs)
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);
	std::deque<std::string> foundArchives;

	isDirty = true;

	// scan for all archives
	for (const std::string& dir: scanDirs) {
		if (!FileSystem::DirExists(dir))
			continue;

		LOG("Scanning: %s", dir.c_str());
		ScanDir(dir, foundArchives);
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
	for (const auto& archiveInfo: archiveInfos) {
		const std::string& lcOriginalName = StringToLower(archiveInfo.origName);

		for (const std::string& replaceName: archiveInfo.archiveData.GetReplaces()) {
			const std::string& lcReplaceName = StringToLower(replaceName);

			// Overwrite the info for this archive with a replaced pointer
			ArchiveInfo& ai = GetAddArchiveInfo(lcReplaceName);

			ai.path = "";
			ai.origName = replaceName;
			ai.modified = 1;
			ai.archiveData = {};
			ai.updated = true;
			ai.replaced = lcOriginalName;
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
	spring::VectorInsertUnique(deps, dependency, true);
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

	// check for smf and if the uncompression of important files is too costy
	for (unsigned fid = 0; fid != ar->NumFiles(); ++fid) {
		const std::pair<std::string, int>& info = ar->FileInfo(fid);
		const std::string& ext = FileSystem::GetExtension(StringToLower(info.first));

		if (ext == "smf")
			return info.first;
	}

	return "";
}


CArchiveScanner::ArchiveInfo& CArchiveScanner::GetAddArchiveInfo(const std::string& lcfn)
{
	auto aiIter = archiveInfosIndex.find(lcfn);
	auto aiPair = std::make_pair(aiIter, false);

	if (aiIter == archiveInfosIndex.end()) {
		aiPair = archiveInfosIndex.insert(lcfn, archiveInfos.size());
		aiIter = aiPair.first;
		archiveInfos.emplace_back();
	}

	return archiveInfos[aiIter->second];
}

CArchiveScanner::BrokenArchive& CArchiveScanner::GetAddBrokenArchive(const std::string& lcfn)
{
	auto baIter = brokenArchivesIndex.find(lcfn);
	auto baPair = std::make_pair(baIter, false);

	if (baIter == brokenArchivesIndex.end()) {
		baPair = brokenArchivesIndex.insert(lcfn, brokenArchives.size());
		baIter = baPair.first;
		brokenArchives.emplace_back();
	}

	return brokenArchives[baIter->second];
}


void CArchiveScanner::ScanArchive(const std::string& fullName, bool doChecksum)
{
	unsigned modifiedTime = 0;

	assert(!isInScan);

	if (CheckCachedData(fullName, modifiedTime, doChecksum))
		return;

	isDirty = true;
	isInScan = true;

	struct ScanScope {
		 ScanScope(bool* b) { p = b; *p =  true; }
		~ScanScope(       ) {        *p = false; }

		bool* p = nullptr;
	};

	const ScanScope scanScope(&isInScan);

	const std::string& fname = FileSystem::GetFilename(fullName);
	const std::string& fpath = FileSystem::GetDirectory(fullName);
	const std::string& lcfn  = StringToLower(fname);

	std::unique_ptr<IArchive> ar(archiveLoader.OpenArchive(fullName));

	if (ar == nullptr || !ar->IsOpen()) {
		LOG_L(L_WARNING, "[AS::%s] unable to open archive \"%s\"", __func__, fullName.c_str());

		// record it as broken, so we don't need to look inside everytime
		BrokenArchive& ba = GetAddBrokenArchive(lcfn);
		ba.name = lcfn;
		ba.path = fpath;
		ba.modified = modifiedTime;
		ba.updated = true;
		ba.problem = "Unable to open archive";

		// does not count as a scan
		// numScannedArchives += 1;
		return;
	}

	std::string error;
	std::string arMapFile; // file in archive with "smf" extension
	std::string miMapFile; // value for the 'mapfile' key parsed from mapinfo
	std::string luaInfoFile;

	const bool hasModInfo = ar->FileExists("modinfo.lua");
	const bool hasMapInfo = ar->FileExists("mapinfo.lua");


	ArchiveInfo ai;
	ArchiveData& ad = ai.archiveData;

	// execute the respective .lua, otherwise assume this archive is a map
	if (hasMapInfo) {
		ScanArchiveLua(ar.get(), luaInfoFile = "mapinfo.lua", ai, error);

		if ((miMapFile = ad.GetMapFile()).empty()) {
			if (ar->GetType() != ARCHIVE_TYPE_SDV)
				LOG_L(L_WARNING, "[AS::%s] set the 'mapfile' key in mapinfo.lua of archive \"%s\" for faster loading!", __func__, fullName.c_str());

			arMapFile = SearchMapFile(ar.get(), error);
		}
	} else if (hasModInfo) {
		ScanArchiveLua(ar.get(), luaInfoFile = "modinfo.lua", ai, error);
	} else {
		arMapFile = SearchMapFile(ar.get(), error);
	}

	if (!CheckCompression(ar.get(), fullName, error)) {
		LOG_L(L_WARNING, "[AS::%s] failed to scan \"%s\" (%s)", __func__, fullName.c_str(), error.c_str());

		// mark archive as broken, so we don't need to look inside everytime
		BrokenArchive& ba = GetAddBrokenArchive(lcfn);
		ba.name = lcfn;
		ba.path = fpath;
		ba.modified = modifiedTime;
		ba.updated = true;
		ba.problem = error;

		// does count as a scan
		numScannedArchives += 1;
		return;
	}

	if (hasMapInfo || !arMapFile.empty()) {
		// map archive
		// FIXME: name will never be empty if version is set (see HACK in ArchiveData)
		if ((ad.GetName()).empty()) {
			ad.SetInfoItemValueString("name_pure", FileSystem::GetBasename(arMapFile));
			ad.SetInfoItemValueString("name", FileSystem::GetBasename(arMapFile));
		}

		if (miMapFile.empty())
			ad.SetInfoItemValueString("mapfile", arMapFile);

		AddDependency(ad.GetDependencies(), GetMapHelperContentName());
		ad.SetInfoItemValueInteger("modType", modtype::map);

		LOG_S(LOG_SECTION_ARCHIVESCANNER, "Found new map: %s", ad.GetNameVersioned().c_str());
	} else if (hasModInfo) {
		// game or base-type (cursors, bitmaps, ...) archive
		// babysitting like this is really no longer required
		if (ad.IsGame() || ad.IsMenu())
			AddDependency(ad.GetDependencies(), GetSpringBaseContentName());

		LOG_S(LOG_SECTION_ARCHIVESCANNER, "Found new game: %s", ad.GetNameVersioned().c_str());
	} else {
		// neither a map nor a mod: error
		LOG_S(LOG_SECTION_ARCHIVESCANNER, "missing modinfo.lua/mapinfo.lua");
	}

	ai.path = fpath;
	ai.modified = modifiedTime;

	// Store modinfo.lua/mapinfo.lua modified timestamp for directory archives, as only they can change.
	if (ar->GetType() == ARCHIVE_TYPE_SDD && !luaInfoFile.empty()) {
		ai.archiveDataPath = ar->GetArchiveFile() + "/" + static_cast<const CDirArchive*>(ar.get())->GetOrigFileName(ar->FindFile(luaInfoFile));
		ai.modifiedArchiveData = FileSystemAbstraction::GetFileModificationTime(ai.archiveDataPath);
	}

	ai.origName = fname;
	ai.updated = true;
	ai.hashed = doChecksum && GetArchiveChecksum(fullName, ai);

	archiveInfosIndex.insert(lcfn, archiveInfos.size());
	archiveInfos.emplace_back(std::move(ai));

	numScannedArchives += 1;
}


bool CArchiveScanner::CheckCachedData(const std::string& fullName, unsigned& modified, bool doChecksum)
{
	// virtual archives do not exist on disk, and thus do not have a modification time
	// they should still be scanned as normal archives so we only skip the cache-check
	if (FileSystem::GetExtension(fullName) == "sva")
		return false;

	// if stat fails, assume the archive is not broken nor cached
	// it would also fail in the case of virtual archives and cause
	// warning-spam which is suppressed by the extension-test above
	if ((modified = FileSystemAbstraction::GetFileModificationTime(fullName)) == 0)
		return false;

	const std::string& fileName      = FileSystem::GetFilename(fullName);
	const std::string& filePath      = FileSystem::GetDirectory(fullName);
	const std::string& fileNameLower = StringToLower(fileName);


	const auto baIter = brokenArchivesIndex.find(fileNameLower);
	const auto aiIter = archiveInfosIndex.find(fileNameLower);

	// determine whether this archive has earlier be found to be broken
	if (baIter != brokenArchivesIndex.end()) {
		BrokenArchive& ba = brokenArchives[baIter->second];

		if (modified == ba.modified && filePath == ba.path)
			return (ba.updated = true);
	}


	// determine whether to rely on the cached info or not
	if (aiIter == archiveInfosIndex.end())
		return false;

	const size_t archiveIndex = aiIter->second;

	ArchiveInfo&  ai = archiveInfos[archiveIndex           ];
	ArchiveInfo& rai = archiveInfos[archiveInfos.size() - 1];

	// this archive may have been obsoleted, do not process it if so
	if (!ai.replaced.empty())
		return true;

	const bool haveValidCacheData = (modified == ai.modified && filePath == ai.path);
	// check if the archive data file (modinfo.lua/mapinfo.lua) has changed
	const bool archiveDataChanged = (!ai.archiveDataPath.empty() && FileSystemAbstraction::GetFileModificationTime(ai.archiveDataPath) != ai.modifiedArchiveData);

	if (haveValidCacheData && !archiveDataChanged) {
		// archive found in cache, update checksum if wanted
		// this also has to flag isDirty or ArchiveCache will
		// not be rewritten even if the hash silently changed,
		// e.g. after redownload
		ai.updated = true;

		if (doChecksum && !ai.hashed)
			isDirty |= (ai.hashed = GetArchiveChecksum(fullName, ai));

		return true;
	}

	if (ai.updated) {
		LOG_L(L_ERROR, "[AS::%s] found a \"%s\" already in \"%s\", ignoring.", __func__, fullName.c_str(), (ai.path + ai.origName).c_str());

		if (baseContentArchives.find(aiIter->first) == baseContentArchives.end())
			return true; // ignore

		throw user_error(
			std::string("duplicate base content detected:\n\t") + ai.path +
			std::string("\n\t") + filePath +
			std::string("\nPlease fix your configuration/installation as this can cause desyncs!")
		);
	}

	// if we are here, we could have invalid info in the cache
	// force a reread if it is a directory archive (.sdd), as
	// st_mtime only reflects changes to the directory itself
	// (not the contents)
	//
	// remap replacement archive
	if (archiveIndex != (archiveInfos.size() - 1)) {
		archiveInfosIndex[StringToLower(rai.origName)] = archiveIndex;
		archiveInfos[archiveIndex] = std::move(rai);
	}

	archiveInfosIndex.erase(fileNameLower);
	archiveInfos.pop_back();
	return false;
}


bool CArchiveScanner::ScanArchiveLua(IArchive* ar, const std::string& fileName, ArchiveInfo& ai, std::string& err)
{
	std::vector<std::uint8_t> buf;

	if (!ar->GetFile(fileName, buf) || buf.empty()) {
		err = "Error reading " + fileName;

		if (ar->GetArchiveFile().find(".sdp") != std::string::npos)
			err += " (archive's rapid tag: " + GetRapidTagFromPackage(FileSystem::GetBasename(ar->GetArchiveFile())) + ")";

		return false;
	}

	// NB: skips LuaConstGame::PushEntries(L) since that would invoke ScanArchive again
	LuaParser p(std::string((char*)(buf.data()), buf.size()), SPRING_VFS_ZIP);

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

	// this automatically splits lines
	if (ar->GetFile("springignore.txt", buf) && !buf.empty())
		ignore->AddRule(std::string((char*)(&buf[0]), buf.size()));

	return ignore;
}



/**
 * Get checksum of the data in the specified archive.
 * Returns 0 if file could not be opened.
 */
bool CArchiveScanner::GetArchiveChecksum(const std::string& archiveName, ArchiveInfo& archiveInfo)
{
	// try to open an archive
	std::unique_ptr<IArchive> ar(archiveLoader.OpenArchive(archiveName));

	if (ar == nullptr)
		return false;

	// load ignore list, and insert all files to check in lowercase format
	std::unique_ptr<IFileFilter> ignore(CreateIgnoreFilter(ar.get()));
	std::vector<std::string> fileNames;
	std::vector<sha512::raw_digest> fileHashes;
	std::array<std::vector<std::uint8_t>, ThreadPool::MAX_THREADS> fileBuffers;

	fileNames.reserve(ar->NumFiles());
	fileHashes.reserve(ar->NumFiles());

	for (unsigned fid = 0; fid != ar->NumFiles(); ++fid) {
		const std::pair<std::string, int>& info = ar->FileInfo(fid);

		if (ignore->Match(info.first))
			continue;

		// create case-insensitive hashes
		fileNames.push_back(StringToLower(info.first));
		fileHashes.emplace_back();
	}

	// sort by filename
	std::stable_sort(fileNames.begin(), fileNames.end());

	// compute hashes of the files
	for_mt(0, fileNames.size(), [&](const int i) {
		ar->CalcHash(ar->FindFile(fileNames[i]), fileHashes[i].data(), fileBuffers[ ThreadPool::GetThreadNum() ]);

		#if !defined(DEDICATED) && !defined(UNITSYNC)
		Watchdog::ClearTimer(WDT_MAIN);
		#endif
	});

	// combine individual hashes, initialize to hash(name)
	for (size_t i = 0; i < fileNames.size(); i++) {
		sha512::calc_digest(reinterpret_cast<const uint8_t*>(fileNames[i].c_str()), fileNames[i].size(), archiveInfo.checksum);

		for (uint8_t j = 0; j < sha512::SHA_LEN; j++) {
			archiveInfo.checksum[j] ^= fileHashes[i][j];
		}

		#if !defined(DEDICATED) && !defined(UNITSYNC)
		Watchdog::ClearTimer();
		#endif
	}

	return true;
}


void CArchiveScanner::ReadCacheData(const std::string& filename)
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);
	if (!FileSystem::FileExists(filename)) {
		LOG_L(L_INFO, "[AS::%s] ArchiveCache %s doesn't exist", __func__, filename.c_str());
		return;
	}

	LuaParser p(filename, SPRING_VFS_RAW, SPRING_VFS_BASE);
	if (!p.Execute()) {
		LOG_L(L_ERROR, "[AS::%s] failed to parse ArchiveCache: %s", __func__, p.GetErrorLog().c_str());
		return;
	}

	const LuaTable& archiveCacheTbl = p.GetRoot();
	const LuaTable& archivesTbl = archiveCacheTbl.SubTable("archives");
	const LuaTable& brokenArchivesTbl = archiveCacheTbl.SubTable("brokenArchives");

	// Do not load old version caches
	const int ver = archiveCacheTbl.GetInt("internalver", (INTERNAL_VER + 1));
	if (ver != INTERNAL_VER)
		return;

	for (int i = 1; archivesTbl.KeyExists(i); ++i) {
		const LuaTable& curArchiveTbl = archivesTbl.SubTable(i);
		const LuaTable& archivedTbl = curArchiveTbl.SubTable("archivedata");

		const std::string& curArchiveName = curArchiveTbl.GetString("name", "");
		const std::string& curArchiveNameLC = StringToLower(curArchiveName);
		const std::string& hexDigestStr = curArchiveTbl.GetString("checksum", "");

		ArchiveInfo& ai = GetAddArchiveInfo(curArchiveNameLC);
		ArchiveInfo tmp; // used to compare against all-zero hash

		ai.origName 	   = curArchiveName;
		ai.path     	   = curArchiveTbl.GetString("path", "");
		ai.archiveDataPath = curArchiveTbl.GetString("archiveDataPath", "");

		// do not use LuaTable.GetInt() for 32-bit integers: the Spring lua
		// library uses 32-bit floats to represent numbers, which can only
		// represent 2^24 consecutive integers
		ai.modified = strtoul(curArchiveTbl.GetString("modified", "0").c_str(), nullptr, 10);
		ai.modifiedArchiveData = strtoul(curArchiveTbl.GetString("modifiedArchiveData", "0").c_str(), nullptr, 10);

		// convert digest-string back to raw checksum
		if (hexDigestStr.size() == (sha512::SHA_LEN * 2)) {
			sha512::hex_digest hexDigest;
			sha512::raw_digest rawDigest;
			std::copy(hexDigestStr.begin(), hexDigestStr.end(), hexDigest.data());
			sha512::read_digest(hexDigest, rawDigest);
			std::memcpy(ai.checksum, rawDigest.data(), sha512::SHA_LEN);
		}

		ai.updated = false;
		ai.hashed = (memcmp(ai.checksum, tmp.checksum, sha512::SHA_LEN) != 0);


		ai.archiveData = CArchiveScanner::ArchiveData(archivedTbl, true);
		if (ai.archiveData.IsMap()) {
			AddDependency(ai.archiveData.GetDependencies(), GetMapHelperContentName());
		} else if (ai.archiveData.IsGame()) {
			AddDependency(ai.archiveData.GetDependencies(), GetSpringBaseContentName());
		}
	}

	for (int i = 1; brokenArchivesTbl.KeyExists(i); ++i) {
		const LuaTable& curArchive = brokenArchivesTbl.SubTable(i);
		const std::string& name = StringToLower(curArchive.GetString("name", ""));

		BrokenArchive& ba = GetAddBrokenArchive(name);
		ba.name = name;
		ba.path = curArchive.GetString("path", "");
		ba.modified = strtoul(curArchive.GetString("modified", "0").c_str(), nullptr, 10);
		ba.updated = false;
		ba.problem = curArchive.GetString("problem", "unknown");
	}

	isDirty = false;
}

static inline void SafeStr(FILE* out, const char* prefix, const std::string& str)
{
	if (str.empty())
		return;

	if ((str.find_first_of("\\\"") != std::string::npos) || (str.find_first_of('\n') != std::string::npos )) {
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
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);
	if (!isDirty)
		return;

	FILE* out = fopen(filename.c_str(), "wt");
	if (out == nullptr) {
		LOG_L(L_ERROR, "[AS::%s] failed to write to \"%s\"!", __func__, filename.c_str());
		return;
	}

	// First delete all outdated information
	{
		std::stable_sort(archiveInfos.begin(), archiveInfos.end(), [](const ArchiveInfo& a, const ArchiveInfo& b) { return (a.origName < b.origName); });
		std::stable_sort(brokenArchives.begin(), brokenArchives.end(), [](const BrokenArchive& a, const BrokenArchive& b) { return (a.name < b.name); });

		const auto it = std::remove_if(archiveInfos.begin(), archiveInfos.end(), [](const ArchiveInfo& i) { return (!i.updated); });
		const auto jt = std::remove_if(brokenArchives.begin(), brokenArchives.end(), [](const BrokenArchive& i) { return (!i.updated); });

		archiveInfos.erase(it, archiveInfos.end());
		brokenArchives.erase(jt, brokenArchives.end());

		archiveInfosIndex.clear();
		brokenArchivesIndex.clear();

		// rebuild index-maps
		for (const ArchiveInfo& ai: archiveInfos) {
			archiveInfosIndex.insert(StringToLower(ai.origName), &ai - &archiveInfos[0]);
		}
		for (const BrokenArchive& bi: brokenArchives) {
			brokenArchivesIndex.insert(bi.name, &bi - &brokenArchives[0]);
		}
	}


	fprintf(out, "local archiveCache = {\n\n");
	fprintf(out, "\tinternalver = %i,\n\n", INTERNAL_VER);
	fprintf(out, "\tarchives = {  -- count = %u\n", unsigned(archiveInfos.size()));

	for (const ArchiveInfo& arcInfo: archiveInfos) {
		sha512::raw_digest rawDigest;
		sha512::hex_digest hexDigest;

		std::memcpy(rawDigest.data(), arcInfo.checksum, sha512::SHA_LEN);
		sha512::dump_digest(rawDigest, hexDigest);

		fprintf(out, "\t\t{\n");
		SafeStr(out, "\t\t\tname = ",              arcInfo.origName);
		SafeStr(out, "\t\t\tpath = ",              arcInfo.path);
		fprintf(out, "\t\t\tmodified = \"%u\",\n", arcInfo.modified);
		fprintf(out, "\t\t\tchecksum = \"%s\",\n", hexDigest.data());
		SafeStr(out, "\t\t\treplaced = ",          arcInfo.replaced);

		if (!arcInfo.archiveDataPath.empty()) {
			SafeStr(out, "\t\t\tarchiveDataPath = ",              arcInfo.archiveDataPath);
			fprintf(out, "\t\t\tmodifiedArchiveData = \"%u\",\n", arcInfo.modifiedArchiveData);
		}

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
				for (const auto& dep: deps) {
					SafeStr(out, "\t\t\t\t\t", dep);
				}
				fprintf(out, "\t\t\t\t},\n");
			}
			fprintf(out, "\t\t\t},\n");
		}

		fprintf(out, "\t\t},\n");
	}

	fprintf(out, "\t},\n\n"); // close 'archives'
	fprintf(out, "\tbrokenArchives = {  -- count = %u\n", unsigned(brokenArchives.size()));

	for (const BrokenArchive& ba: brokenArchives) {
		fprintf(out, "\t\t{\n");
		SafeStr(out, "\t\t\tname = ", ba.name);
		SafeStr(out, "\t\t\tpath = ", ba.path);
		fprintf(out, "\t\t\tmodified = \"%u\",\n", ba.modified);
		SafeStr(out, "\t\t\tproblem = ", ba.problem);
		fprintf(out, "\t\t},\n");
	}

	fprintf(out, "\t},\n"); // close 'brokenArchives'
	fprintf(out, "}\n\n"); // close 'archiveCache'
	fprintf(out, "return archiveCache\n");

	if (fclose(out) == EOF)
		LOG_L(L_ERROR, "[AS::%s] failed to write to \"%s\"!", __func__, filename.c_str());

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
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	std::vector<ArchiveData> ret;
	ret.reserve(archiveInfos.size());

	for (const ArchiveInfo& ai: archiveInfos) {
		const ArchiveData& aid = ai.archiveData;

		if ((!aid.GetName().empty()) && (aid.GetModType() == modtype::primary)) {
			// Add the archive the mod is in as the first dependency
			ArchiveData md = aid;
			md.GetDependencies().insert(md.GetDependencies().begin(), ai.origName);
			ret.push_back(md);
		}
	}

	sortByName(ret);
	return ret;
}


std::vector<CArchiveScanner::ArchiveData> CArchiveScanner::GetAllMods() const
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	std::vector<ArchiveData> ret;
	ret.reserve(archiveInfos.size());

	for (const ArchiveInfo& ai: archiveInfos) {
		const ArchiveData& aid = ai.archiveData;

		if ((!aid.GetName().empty()) && aid.IsGame()) {
			// Add the archive the mod is in as the first dependency
			ArchiveData md = aid;
			md.GetDependencies().insert(md.GetDependencies().begin(), ai.origName);
			ret.push_back(md);
		}
	}

	sortByName(ret);
	return ret;
}


std::vector<CArchiveScanner::ArchiveData> CArchiveScanner::GetAllArchives() const
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	std::vector<ArchiveData> ret;
	ret.reserve(archiveInfos.size());

	for (const ArchiveInfo& ai: archiveInfos) {
		const ArchiveData& aid = ai.archiveData;

		// Add the archive the mod is in as the first dependency
		ArchiveData md = aid;
		md.GetDependencies().insert(md.GetDependencies().begin(), ai.origName);
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
	const auto& NameCmp = [](const std::pair<std::string, size_t>& a, const std::pair<std::string, size_t>& b) { return (a.first  < b.first ); };
	const auto& IndxCmp = [](const std::pair<std::string, size_t>& a, const std::pair<std::string, size_t>& b) { return (a.second < b.second); };

	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	std::vector<          std::string         > retArchives;
	std::vector<std::pair<std::string, size_t>> tmpArchives[2];

	std::deque<std::string> archiveQueue = {rootArchive};

	retArchives.reserve(8);
	tmpArchives[0].reserve(8);
	tmpArchives[1].reserve(8);

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
			const auto HandleUnresolvedDep = [&tmpArchives](const std::string& archName) { tmpArchives[0].emplace_back(archName, tmpArchives[0].size()); return true; };
			#else
			const auto HandleUnresolvedDep = [&tmpArchives](const std::string& archName) { (void) archName; return false; };
			#endif

			auto aii = archiveInfosIndex.find(lwrCaseName);
			auto aij = aii;

			const ArchiveInfo* ai = nullptr;

			if (aii == archiveInfosIndex.end()) {
				if (!HandleUnresolvedDep(lwrCaseName))
					throw content_error("Dependent archive \"" + lwrCaseName + "\" not found");

				return nullptr;
			}

			ai = &archiveInfos[aii->second];

			// check if this archive has an unresolved replacement
			while (!ai->replaced.empty()) {
				if ((aii = archiveInfosIndex.find(ai->replaced)) == archiveInfosIndex.end()) {
					if (!HandleUnresolvedDep(lwrCaseName))
						throw content_error("Replacement \"" + ai->replaced + "\" for archive \"" + lwrCaseName + "\" not found");

					return nullptr;
				}

				aij = aii;
				ai = &archiveInfos[aij->second];
			}

			return ai;
		};


		if ((archiveInfo = CanAddSubDependencies(lowerCaseName)) == nullptr)
			continue;

		tmpArchives[0].emplace_back(archiveInfo->archiveData.GetNameVersioned(), tmpArchives[0].size());

		// expand dependencies in depth-first order
		for (const std::string& archiveDep: archiveInfo->archiveData.GetDependencies()) {
			assert(archiveDep != rootArchive);
			assert(archiveDep != tmpArchives[0][tmpArchives[0].size() - 1].first);
			archiveQueue.push_front(archiveDep);
		}
	}

	std::stable_sort(tmpArchives[0].begin(), tmpArchives[0].end(), NameCmp);

	// filter out any duplicate dependencies
	for (auto& archiveEntry: tmpArchives[0]) {
		if (tmpArchives[1].empty() || archiveEntry.first != tmpArchives[1][tmpArchives[1].size() - 1].first) {
			tmpArchives[1].emplace_back(std::move(archiveEntry.first), archiveEntry.second);
		}
	}

	// resort in original traversal order so overrides work as expected
	std::stable_sort(tmpArchives[1].begin(), tmpArchives[1].end(), IndxCmp);

	for (auto& archiveEntry: tmpArchives[1]) {
		retArchives.emplace_back(std::move(archiveEntry.first));
	}

	return retArchives;
}


std::vector<std::string> CArchiveScanner::GetMaps() const
{
	std::vector<std::string> ret;

	for (const ArchiveInfo& ai: archiveInfos) {
		const ArchiveData& ad = ai.archiveData;

		if (!(ad.GetName().empty()) && ad.IsMap())
			ret.push_back(ad.GetNameVersioned());
	}

	return ret;
}

std::string CArchiveScanner::MapNameToMapFile(const std::string& versionedMapName) const
{
	// Convert map name to map archive
	const auto pred = [&](const decltype(archiveInfos)::value_type& p) { return (p.archiveData.GetNameVersioned() == versionedMapName); };
	const auto iter = std::find_if(archiveInfos.cbegin(), archiveInfos.cend(), pred);

	if (iter != archiveInfos.cend())
		return (iter->archiveData.GetMapFile());

	LOG_SL(LOG_SECTION_ARCHIVESCANNER, L_WARNING, "map file of %s not found", versionedMapName.c_str());
	return versionedMapName;
}



sha512::raw_digest CArchiveScanner::GetArchiveSingleChecksumBytes(const std::string& filePath)
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	// compute checksum for archive only when it is actually loaded by e.g. PreGame or LuaVFS
	// (this updates its ArchiveInfo iff !CheckCachedData and marks the scanner as dirty s.t.
	// cache will be rewritten on reload/shutdown)
	ScanArchive(filePath, true);

	const std::string& lcName = StringToLower(FileSystem::GetFilename(filePath));
	const auto aiIter = archiveInfosIndex.find(lcName);

	sha512::raw_digest checksum;
	std::fill(checksum.begin(), checksum.end(), 0);

	if (aiIter == archiveInfosIndex.end())
		return checksum;

	std::memcpy(checksum.data(), archiveInfos[aiIter->second].checksum, sha512::SHA_LEN);
	return checksum;
}

sha512::raw_digest CArchiveScanner::GetArchiveCompleteChecksumBytes(const std::string& name)
{
	sha512::raw_digest checksum;
	std::fill(checksum.begin(), checksum.end(), 0);

	for (const std::string& depName: GetAllArchivesUsedBy(name)) {
		const std::string& archiveName = ArchiveFromName(depName);
		const std::string  archivePath = GetArchivePath(archiveName) + archiveName;

		const sha512::raw_digest& archiveChecksum = GetArchiveSingleChecksumBytes(archivePath);

		for (uint8_t i = 0; i < sha512::SHA_LEN; i++) {
			checksum[i] ^= archiveChecksum[i];
		}
	}

	return checksum;
}


void CArchiveScanner::CheckArchive(
	const std::string& name,
	const sha512::raw_digest& serverChecksum,
	      sha512::raw_digest& clientChecksum
) {
	if ((clientChecksum = GetArchiveCompleteChecksumBytes(name)) == serverChecksum)
		return;

	sha512::hex_digest serverChecksumHex;
	sha512::hex_digest clientChecksumHex;
	sha512::dump_digest(serverChecksum, serverChecksumHex);
	sha512::dump_digest(clientChecksum, clientChecksumHex);

	char msg[1024];
	sprintf(
		msg,
		"Archive %s (checksum %s) differs from the host's copy (checksum %s). "
		"This may be caused by a corrupted download or there may even be two "
		"different versions in circulation. Make sure you and the host have installed "
		"the chosen archive and its dependencies and consider redownloading it.",
		name.c_str(), clientChecksumHex.data(), serverChecksumHex.data());

	throw content_error(msg);
}

std::string CArchiveScanner::GetArchivePath(const std::string& archiveName) const
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	const auto aii = archiveInfosIndex.find(StringToLower(FileSystem::GetFilename(archiveName)));

	if (aii == archiveInfosIndex.end())
		return "";

	return archiveInfos[aii->second].path;
}

std::string CArchiveScanner::NameFromArchive(const std::string& archiveName) const
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	const auto aii = archiveInfosIndex.find(StringToLower(archiveName));

	if (aii != archiveInfosIndex.end())
		return (archiveInfos[aii->second].archiveData.GetNameVersioned());

	return archiveName;
}


std::string CArchiveScanner::GameHumanNameFromArchive(const std::string& archiveName) const
{
	return (ArchiveNameResolver::GetGame(NameFromArchive(archiveName)));
}

std::string CArchiveScanner::MapHumanNameFromArchive(const std::string& archiveName) const
{
	return (ArchiveNameResolver::GetMap(NameFromArchive(archiveName)));
}


std::string CArchiveScanner::ArchiveFromName(const std::string& versionedName) const
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	const auto pred = [&](const decltype(archiveInfos)::value_type& p) { return (p.archiveData.GetNameVersioned() == versionedName); };
	const auto iter = std::find_if(archiveInfos.cbegin(), archiveInfos.cend(), pred);

	if (iter != archiveInfos.cend())
		return iter->origName;

	return versionedName;
}

CArchiveScanner::ArchiveData CArchiveScanner::GetArchiveData(const std::string& versionedName) const
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	const auto pred = [&](const decltype(archiveInfos)::value_type& p) { return (p.archiveData.GetNameVersioned() == versionedName); };
	const auto iter = std::find_if(archiveInfos.cbegin(), archiveInfos.cend(), pred);

	if (iter != archiveInfos.cend())
		return iter->archiveData;

	return {};
}


CArchiveScanner::ArchiveData CArchiveScanner::GetArchiveDataByArchive(const std::string& archive) const
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	const auto aii = archiveInfosIndex.find(StringToLower(archive));

	if (aii != archiveInfosIndex.end())
		return archiveInfos[aii->second].archiveData;

	return {};
}

int CArchiveScanner::GetMetaFileClass(const std::string& filePath)
{
	const std::string& lowerFilePath = StringToLower(filePath);
	// const std::string& ext = FileSystem::GetExtension(lowerFilePath);
	const auto it = metaFileClasses.find(lowerFilePath);

	if (it != metaFileClasses.end())
		return (it->second);

//	if (ext == "smf") // to generate minimap
//		return 1;

	for (const auto& p: metaDirClasses) {
		if (StringStartsWith(lowerFilePath, p.first))
			return (p.second);
	}

	return 0;
}

