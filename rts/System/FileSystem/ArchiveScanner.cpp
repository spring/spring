/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <list>
#include <algorithm>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <boost/scoped_ptr.hpp>

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
#include "System/Log/ILog.h"
#include "System/CRC.h"
#include "System/Util.h"
#include "System/Exceptions.h"
#include "System/ThreadPool.h"
#include "System/FileSystem/RapidHandler.h"

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

const int INTERNAL_VER = 10;
CArchiveScanner* archiveScanner = NULL;


/*
 * Engine known (and used?) tags in [map|mod]info.lua
 */
struct KnownInfoTag {
	std::string name;
	std::string desc;
	bool must_have;
};

const KnownInfoTag knownTags[] = {
	{"name",        "example: Original Total Annihilation", true},
	{"shortname",   "example: OTA", false},
	{"version",     "example: v2.3", false},
	{"mutator",     "example: deployment", false},
	{"game",        "example: Total Annihilation", false},
	{"shortgame",   "example: TA", false},
	{"description", "example: Little units blowing up other little units", false},
	{"mapfile",     "in case its a map, store location of smf/sm3 file", false}, //FIXME is this ever used in the engine?! or does it auto calc the location?
	{"modtype",     "0=hidden, 1=primary, (2=unused), 3=map, 4=base, 5=menu", true},
	{"depend",      "a table with all archives that needs to be loaded for this one", false},
	{"replace",     "a table with archives that got replaced with this one", false},
	{"onlyLocal",   "if true spring will not listen for incoming connections", false}
};

/*
 * CArchiveScanner::ArchiveData
 */
CArchiveScanner::ArchiveData::ArchiveData(const LuaTable& archiveTable, bool fromCache)
{
	if (!archiveTable.IsValid()) {
		return;
	}

	std::vector<std::string> keys;
	if (!archiveTable.GetKeys(keys)) {
		return;
	}

	for (std::string& key: keys) {
		const std::string keyLower = StringToLower(key);
		if (!ArchiveData::IsReservedKey(keyLower)) {
			if (keyLower == "modtype") {
				SetInfoItemValueInteger(key, archiveTable.GetInt(key, 0));
				continue;
			}

			const int luaType = archiveTable.GetType(key);
			switch (luaType) {
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
	}

	const LuaTable _dependencies = archiveTable.SubTable("depend");

	for (int dep = 1; _dependencies.KeyExists(dep); ++dep) {
		dependencies.push_back(_dependencies.GetString(dep, ""));
	}

	const LuaTable _replaces = archiveTable.SubTable("replace");
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
	static const int extCount = sizeof(knownTags) / sizeof(KnownInfoTag);
	for (int i = 0; i < extCount; ++i) {
		const KnownInfoTag tag = knownTags[i];
		if (keyLower == tag.name) {
			return tag.desc;
		}
	}

	return "<custom property>";
}


bool CArchiveScanner::ArchiveData::IsReservedKey(const std::string& keyLower)
{
	return ((keyLower == "depend") || (keyLower == "replace"));
}


bool CArchiveScanner::ArchiveData::IsValid(std::string& err) const
{
	std::string missingtag;

	static const int extCount = sizeof(knownTags) / sizeof(KnownInfoTag);
	for (int i = 0; i < extCount; ++i) {
		const KnownInfoTag tag = knownTags[i];
		if (tag.must_have && (info.find(tag.name) == info.end())) {
			missingtag = tag.name;
			break;
		}
	}

	if (missingtag.empty()) {
		return true;
	} else {
		err = "Missing tag \"" + missingtag+ "\".";
		return false;
	}
}


InfoItem* CArchiveScanner::ArchiveData::GetInfoItem(const std::string& key)
{
	InfoItem* infoItem = NULL;

	const std::map<std::string, InfoItem>::iterator ii = info.find(StringToLower(key));
	if (ii != info.end()) {
		infoItem = &(ii->second);
	}

	return infoItem;
}

const InfoItem* CArchiveScanner::ArchiveData::GetInfoItem(const std::string& key) const
{
	const InfoItem* infoItem = NULL;

	const std::map<std::string, InfoItem>::const_iterator ii = info.find(StringToLower(key));
	if (ii != info.end()) {
		infoItem = &(ii->second);
	}

	return infoItem;
}

InfoItem& CArchiveScanner::ArchiveData::EnsureInfoItem(const std::string& key)
{
	const std::string& keyLower = StringToLower(key);

	if (IsReservedKey(keyLower)) {
		throw content_error("You may not use key " + key + " in archive info, as it is reserved.");
	}

	const std::map<std::string, InfoItem>::iterator ii = info.find(keyLower);
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

	for (std::map<std::string, InfoItem>::const_iterator i = info.begin(); i != info.end(); ++i) {
		infoItems.push_back(i->second);
		infoItems.at(infoItems.size() - 1).desc = GetKeyDescription(i->first);
	}

	return infoItems;
}


std::string CArchiveScanner::ArchiveData::GetInfoValueString(const std::string& key) const
{
	std::string valueString = "";

	const InfoItem* infoItem = GetInfoItem(key);
	if (infoItem != NULL) {
		if (infoItem->valueType == INFO_VALUE_TYPE_STRING) {
			valueString = infoItem->valueTypeString;
		} else {
			valueString = infoItem->GetValueAsString();
		}
	}

	return valueString;
}

int CArchiveScanner::ArchiveData::GetInfoValueInteger(const std::string& key) const
{
	int value = 0;

	const InfoItem* infoItem = GetInfoItem(key);
	if ((infoItem != NULL) && (infoItem->valueType == INFO_VALUE_TYPE_INTEGER)) {
		value = infoItem->value.typeInteger;
	}

	return value;
}

float CArchiveScanner::ArchiveData::GetInfoValueFloat(const std::string& key) const
{
	float value = 0.0f;

	const InfoItem* infoItem = GetInfoItem(key);
	if ((infoItem != NULL) && (infoItem->valueType == INFO_VALUE_TYPE_FLOAT)) {
		value = infoItem->value.typeFloat;
	}

	return value;
}

bool CArchiveScanner::ArchiveData::GetInfoValueBool(const std::string& key) const
{
	bool value = false;

	const InfoItem* infoItem = GetInfoItem(key);
	if ((infoItem != NULL) && (infoItem->valueType == INFO_VALUE_TYPE_BOOL)) {
		value = infoItem->value.typeBool;
	}

	return value;
}





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


const std::string& CArchiveScanner::GetFilepath() const
{
	return cachefile;
}

void CArchiveScanner::ScanAllDirs()
{
	const std::vector<std::string>& datadirs = dataDirLocater.GetDataDirPaths();
	std::vector<std::string> scanDirs;
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
	isDirty = true;

	// scan for all archives
	std::list<std::string> foundArchives;
	for (const std::string& dir: scanDirs) {
		if (FileSystem::DirExists(dir)) {
			LOG("Scanning: %s", dir.c_str());
			ScanDir(dir, &foundArchives);
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

	// Create archiveInfos etc. when not being in cache already
	for (const std::string& archive: foundArchives) {
		ScanArchive(archive, false);
	#if !defined(DEDICATED) && !defined(UNITSYNC)
		Watchdog::ClearTimer(WDT_MAIN);
	#endif
	}

	// Now we'll have to parse the replaces-stuff found in the mods
	for (auto& aii: archiveInfos) {
		for (std::string& replaceName: aii.second.archiveData.GetReplaces()) {
			// Overwrite the info for this archive with a replaced pointer
			const std::string lcname = StringToLower(replaceName);
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


void CArchiveScanner::ScanDir(const std::string& curPath, std::list<std::string>* foundArchives)
{
	// check recursive dirs when NOT being sdd's!
	std::list<std::string> subDirs;
	subDirs.push_back(curPath);

	while (!subDirs.empty()) {
		FileSystem::EnsurePathSepAtEnd(subDirs.front());
		const std::vector<std::string>& found = dataDirsAccess.FindFiles(subDirs.front(), "*", FileQueryFlags::INCLUDE_DIRS);
		subDirs.pop_front();

		for (std::string fullName: found) {
			FileSystem::EnsureNoPathSepAtEnd(fullName);
			const std::string lcfpath = StringToLower(FileSystem::GetDirectory(fullName));

			// Exclude archivefiles found inside directory archives (.sdd)
			if (lcfpath.find(".sdd") != std::string::npos) {
				continue;
			}

			// Is this an archive we should look into?
			if (archiveLoader.IsArchiveFile(fullName)) {
				foundArchives->push_front(fullName); // push by reversed order!
			} else
			if (FileSystem::DirExists(fullName)) {
				subDirs.push_back(fullName);
			}
		}
	}
}

static void AddDependency(std::vector<std::string>& deps, const std::string& dependency)
{
	auto it = std::find(deps.begin(), deps.end(), dependency);
	if (it != deps.end()) return;
	deps.push_back(dependency);
}

bool CArchiveScanner::CheckCompression(const IArchive* ar,const std::string& fullName, std::string& error)
{
	if (!ar->CheckForSolid())
		return true;
	for (unsigned fid = 0; fid != ar->NumFiles(); ++fid) {
		std::string name;
		int size;
		ar->FileInfo(fid, name, size);
		const std::string lowerName = StringToLower(name);
		const auto metaFileClass = CArchiveScanner::GetMetaFileClass(lowerName);
		if ((metaFileClass == 0) || ar->HasLowReadingCost(fid))
			continue;

		// is a meta-file and not cheap to read
		if (metaFileClass == 1) {
			// 1st class
			error = "Unpacking/reading cost for meta file " + name
					+ " is too high, please repack the archive (make sure to use a non-solid algorithm, if applicable)";
			return false;
		} else if (metaFileClass == 2) {
			// 2nd class
			LOG_SL(LOG_SECTION_ARCHIVESCANNER, L_WARNING,
					"Archive %s: The cost for reading a 2nd-class meta-file is too high: %s",
					fullName.c_str(), name.c_str());
		}

	}
	return true;
}

std::string CArchiveScanner::SearchMapFile(const IArchive* ar, std::string& error)
{
	assert(ar!=NULL);

	// check for smf/sm3 and if the uncompression of important files is too costy
	for (unsigned fid = 0; fid != ar->NumFiles(); ++fid) {
		std::string name;
		int size;
		ar->FileInfo(fid, name, size);
		const std::string lowerName = StringToLower(name);
		const std::string ext = FileSystem::GetExtension(lowerName);

		if ((ext == "smf") || (ext == "sm3")) {
			return name;
		}

	}
	return "";
}

static bool IsBaseContent(const std::string& fileName)
{
	return ((fileName == "bitmaps.sdz")
		|| (fileName == "springcontent.sdz")
		|| (fileName == "maphelper.sdz")
		|| (fileName == "cursors.sdz"));
}

void CArchiveScanner::ScanArchive(const std::string& fullName, bool doChecksum)
{
	unsigned modifiedTime;
	if (CheckCachedData(fullName, &modifiedTime, doChecksum))
		return;
	isDirty = true;

	const std::string fn    = FileSystem::GetFilename(fullName);
	const std::string fpath = FileSystem::GetDirectory(fullName);
	const std::string lcfn  = StringToLower(fn);

	boost::scoped_ptr<IArchive> ar(archiveLoader.OpenArchive(fullName));
	if (!ar || !ar->IsOpen()) {
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
	auto& ad = ai.archiveData;
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
	CheckCompression(ar.get(), fullName, error);

	if (!error.empty()) {
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
		// it is a map
		if (ad.GetName().empty()) {
			// FIXME The name will never be empty, if version is set (see HACK in ArchiveData)
			ad.SetInfoItemValueString("name_pure", FileSystem::GetBasename(mapfile));
			ad.SetInfoItemValueString("name", FileSystem::GetBasename(mapfile));
		}
		if (ad.GetMapFile().empty()) {
			ad.SetInfoItemValueString("mapfile", mapfile);
		}

		AddDependency(ad.GetDependencies(), "Map Helper v1");
		ad.SetInfoItemValueInteger("modType", modtype::map);

		LOG_S(LOG_SECTION_ARCHIVESCANNER, "Found new map: %s", ad.GetNameVersioned().c_str());
	} else if (hasModinfo && ad.IsGame()) {
		// it is a game
		AddDependency(ad.GetDependencies(), "Spring content v1");

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
	const std::string fn    = FileSystem::GetFilename(fullName);
	const std::string fpath = FileSystem::GetDirectory(fullName);
	const std::string lcfn  = StringToLower(fn);

	// If stat fails, assume the archive is not broken nor cached
	struct stat info;
	const int statfailed = stat(fullName.c_str(), &info);
	*modified = info.st_mtime;
	if (statfailed)
		return false;

	// Determine whether this archive has earlier be found to be broken
	std::map<std::string, BrokenArchive>::iterator bai = brokenArchives.find(lcfn);
	if (bai != brokenArchives.end()) {
		if ((unsigned)info.st_mtime == bai->second.modified && fpath == bai->second.path) {
			bai->second.updated = true;
			return true;
		}
	}

	// Determine whether to rely on the cached info or not
	std::map<std::string, ArchiveInfo>::iterator aii = archiveInfos.find(lcfn);
	if (aii != archiveInfos.end()) {
		// This archive may have been obsoleted, do not process it if so
		if (!aii->second.replaced.empty()) {
			return true;
		}

		if ((unsigned)info.st_mtime == aii->second.modified && fpath == aii->second.path) {
			// cache found update checksum if wanted
			aii->second.updated = true;
			if (doChecksum && (aii->second.checksum == 0)) {
				aii->second.checksum = GetCRC(fullName);
			}
			return true;
		}

		if (aii->second.updated) {
			const std::string filename = aii->first;
			LOG_L(L_ERROR, "Found a \"%s\" already in \"%s\", ignoring.", fullName.c_str(), (aii->second.path + aii->second.origName).c_str());
			if (IsBaseContent(filename)) {
				throw user_error(std::string("duplicate base content detected:\n\t") + aii->second.path + std::string("\n\t") + fpath
					+ std::string("\nPlease fix your configuration/installation as this can cause desyncs!"));
			}
			return true; // ignore
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
	std::vector<boost::uint8_t> buf;
	if (!ar->GetFile(fileName, buf) || buf.empty()) {
		err = "Error reading " + fileName;
		if (ar->GetArchiveName().find(".sdp") != std::string::npos) {
			err += " (archive's rapid tag: " + GetRapidTagFromPackage(FileSystem::GetBasename(ar->GetArchiveName())) + ")";
		}
		return false;
	}

	LuaParser p(std::string((char*)(&buf[0]), buf.size()), SPRING_VFS_MOD_BASE);
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
	std::vector<boost::uint8_t> buf;
	if (ar->GetFile("springignore.txt", buf) && !buf.empty()) {
		// this automatically splits lines
		ignore->AddRule(std::string((char*)(&buf[0]), buf.size()));
	}
	return ignore;
}


/// used below
struct CRCPair {
	std::string* filename;
	unsigned int nameCRC;
	unsigned int dataCRC;
};



/**
 * Get CRC of the data in the specified archive.
 * Returns 0 if file could not be opened.
 */
unsigned int CArchiveScanner::GetCRC(const std::string& arcName)
{
	CRC crc;
	std::list<std::string> files;

	// Try to open an archive
	boost::scoped_ptr<IArchive> ar(archiveLoader.OpenArchive(arcName));
	if (!ar) {
		return 0; // It wasn't an archive
	}

	// Load ignore list.
	boost::scoped_ptr<IFileFilter> ignore(CreateIgnoreFilter(ar.get()));

	// Insert all files to check in lowercase format
	for (unsigned fid = 0; fid != ar->NumFiles(); ++fid) {
		std::string name;
		int size;
		ar->FileInfo(fid, name, size);

		if (ignore->Match(name)) {
			continue;
		}

		StringToLowerInPlace(name); // case insensitive hash
		files.push_back(name);
	}

	// Sort by FileName
	files.sort();

	// Push the filenames into a std::vector, cause OMP can better iterate over those
	std::vector<CRCPair> crcs;
	crcs.reserve(files.size());
	CRCPair crcp;
	for (std::string& f: files) {
		crcp.filename = &f;
		crcs.push_back(crcp);
	}

	// Compute CRCs of the files
	// Hint: Multithreading only speedups `.sdd` loading. For those the CRC generation is extremely slow -
	//       it has to load the full file to calc it! For the other formats (sd7, sdz, sdp) the CRC is saved
	//       in the metainformation of the container and so the loading is much faster. Neither does any of our
	//       current (2011) packing libraries support multithreading :/
	for_mt(0, crcs.size(), [&](const int i) {
		CRCPair& crcp = crcs[i];
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
	for (CRCPair& crcp: crcs) {
		crc.Update(crcp.nameCRC);
		crc.Update(crcp.dataCRC);
	#if !defined(DEDICATED) && !defined(UNITSYNC)
		Watchdog::ClearTimer();
	#endif
	}

	// A value of 0 is used to indicate no crc.. so never return that
	// Shouldn't happen all that often
	unsigned int digest = crc.GetDigest();
	if (digest == 0) digest = 4711;
	return digest;
}


void CArchiveScanner::ComputeChecksumForArchive(const std::string& filePath)
{
	ScanArchive(filePath, true);
}


void CArchiveScanner::ReadCacheData(const std::string& filename)
{
	if (!FileSystem::FileExists(filename)) {
		LOG_L(L_INFO, "Archive cache doesn't exist: %s", filename.c_str());
		return;
	}

	LuaParser p(filename, SPRING_VFS_RAW, SPRING_VFS_BASE);
	if (!p.Execute()) {
		LOG_L(L_ERROR, "Failed to parse archive cache: %s", p.GetErrorLog().c_str());
		return;
	}
	const LuaTable archiveCache = p.GetRoot();

	// Do not load old version caches
	const int ver = archiveCache.GetInt("internalVer", (INTERNAL_VER + 1));
	if (ver != INTERNAL_VER) {
		return;
	}

	const LuaTable archives = archiveCache.SubTable("archives");
	for (int i = 1; archives.KeyExists(i); ++i) {
		const LuaTable curArchive = archives.SubTable(i);
		const LuaTable archived = curArchive.SubTable("archivedata");
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
			AddDependency(ai.archiveData.GetDependencies(), "Map Helper v1");
		} else if (ai.archiveData.IsGame()) {
			AddDependency(ai.archiveData.GetDependencies(), "Spring content v1");
		}
	}

	const LuaTable brokenArchives = archiveCache.SubTable("brokenArchives");
	for (int i = 1; brokenArchives.KeyExists(i); ++i) {
		const LuaTable curArchive = brokenArchives.SubTable(i);
		std::string name = curArchive.GetString("name", "");
		StringToLowerInPlace(name);

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
	if (str.empty()) {
		return;
	}
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
	if (!isDirty) {
		return;
	}

	FILE* out = fopen(filename.c_str(), "wt");
	if (!out) {
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
				FilterDep(deps, "Map Helper v1");
			} else if (archData.IsGame()) {
				FilterDep(deps, "Spring content v1");
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
	std::sort(data.begin(), data.end(), [](const CArchiveScanner::ArchiveData& a, const CArchiveScanner::ArchiveData& b){
		return (a.GetNameVersioned() < b.GetNameVersioned());
	});
}

std::vector<CArchiveScanner::ArchiveData> CArchiveScanner::GetPrimaryMods() const
{
	std::vector<ArchiveData> ret;

	for (std::map<std::string, ArchiveInfo>::const_iterator i = archiveInfos.begin(); i != archiveInfos.end(); ++i) {
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

	for (std::map<std::string, ArchiveInfo>::const_iterator i = archiveInfos.begin(); i != archiveInfos.end(); ++i) {
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

std::vector<std::string> CArchiveScanner::GetAllArchivesUsedBy(const std::string& root, int depth) const
{
	LOG_S(LOG_SECTION_ARCHIVESCANNER, "GetArchives: %s (depth %u)", root.c_str(), depth);
	// Protect against circular dependencies
	// (worst case depth is if all archives form one huge dependency chain)
	if ((unsigned)depth > archiveInfos.size()) {
		throw content_error("Circular dependency");
	}

	std::vector<std::string> ret;
	std::string resolvedName = ArchiveNameResolver::GetGame(root);
	std::string lcname = StringToLower(ArchiveFromName(resolvedName));
	std::map<std::string, ArchiveInfo>::const_iterator aii = archiveInfos.find(lcname);
	if (aii == archiveInfos.end()) {
#ifdef UNITSYNC
		// unresolved dep, add it, so unitsync still shows this file
		ret.push_back(lcname);
		return ret;
#else
		throw content_error("Archive \"" + lcname + "\" not found");
#endif
	}

	// Check if this archive has been replaced
	while (aii->second.replaced.length() > 0) {
		// FIXME instead of this, call this function recursively, to get the propper error handling
		aii = archiveInfos.find(aii->second.replaced);
		if (aii == archiveInfos.end()) {
#ifdef UNITSYNC
			// unresolved dep, add it, so unitsync still shows this file
			ret.push_back(lcname);
			return ret;
#else
			throw content_error("Unknown error parsing archive replacements");
#endif
		}
	}

	// add depth-first
	ret.push_back(aii->second.archiveData.GetNameVersioned());
	for (const std::string& dep: aii->second.archiveData.GetDependencies()) {
		const std::vector<std::string>& deps = GetAllArchivesUsedBy(dep, depth + 1);
		for (const std::string& depSub: deps) {
			AddDependency(ret, depSub);
		}
	}

	return ret;
}


std::vector<std::string> CArchiveScanner::GetMaps() const
{
	std::vector<std::string> ret;

	for (std::map<std::string, ArchiveInfo>::const_iterator aii = archiveInfos.begin(); aii != archiveInfos.end(); ++aii) {
		if (!(aii->second.archiveData.GetName().empty()) && aii->second.archiveData.IsMap()) {
			ret.push_back(aii->second.archiveData.GetNameVersioned());
		}
	}

	return ret;
}

std::string CArchiveScanner::MapNameToMapFile(const std::string& s) const
{
	// Convert map name to map archive
	for (std::map<std::string, ArchiveInfo>::const_iterator aii = archiveInfos.begin(); aii != archiveInfos.end(); ++aii) {
		if (s == aii->second.archiveData.GetNameVersioned()) {
			return aii->second.archiveData.GetMapFile();
		}
	}
	LOG_SL(LOG_SECTION_ARCHIVESCANNER, L_WARNING, "map file of %s not found", s.c_str());
	return s;
}

unsigned int CArchiveScanner::GetSingleArchiveChecksum(const std::string& filePath)
{
	ComputeChecksumForArchive(filePath);
	std::string lcname = FileSystem::GetFilename(filePath);
	StringToLowerInPlace(lcname);

	std::map<std::string, ArchiveInfo>::const_iterator aii = archiveInfos.find(lcname);
	if (aii == archiveInfos.end()) {
		LOG_SL(LOG_SECTION_ARCHIVESCANNER, L_WARNING, "%s checksum: not found (0)", filePath.c_str());
		return 0;
	}

	LOG_S(LOG_SECTION_ARCHIVESCANNER,"%s checksum: %d/%u", filePath.c_str(), aii->second.checksum, aii->second.checksum);
	return aii->second.checksum;
}

unsigned int CArchiveScanner::GetArchiveCompleteChecksum(const std::string& name)
{
	const std::vector<std::string>& ars = GetAllArchivesUsedBy(name);
	unsigned int checksum = 0;

	for (const std::string& filePath: ars) {
		checksum ^= GetSingleArchiveChecksum(filePath);
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
	const std::string lcname = StringToLower(FileSystem::GetFilename(name));
	std::map<std::string, ArchiveInfo>::const_iterator aii = archiveInfos.find(lcname);
	if (aii == archiveInfos.end()) {
		return "";
	}
	return aii->second.path;
}

std::string CArchiveScanner::ArchiveFromName(const std::string& name) const
{
	for (std::map<std::string, ArchiveInfo>::const_iterator it = archiveInfos.begin(); it != archiveInfos.end(); ++it) {
		if (it->second.archiveData.GetNameVersioned() == name) {
			return it->second.origName;
		}
	}

	return name;
}

std::string CArchiveScanner::NameFromArchive(const std::string& archiveName) const
{
	const std::string lcArchiveName = StringToLower(archiveName);
	std::map<std::string, ArchiveInfo>::const_iterator aii = archiveInfos.find(lcArchiveName);
	if (aii != archiveInfos.end()) {
		return aii->second.archiveData.GetNameVersioned();
	}
	return archiveName;
}

CArchiveScanner::ArchiveData CArchiveScanner::GetArchiveData(const std::string& name) const
{
	for (std::map<std::string, ArchiveInfo>::const_iterator it = archiveInfos.begin(); it != archiveInfos.end(); ++it) {
		const ArchiveData& md = it->second.archiveData;
		if (md.GetNameVersioned() == name) {
			return md;
		}
	}
	return ArchiveData();
}

CArchiveScanner::ArchiveData CArchiveScanner::GetArchiveDataByArchive(const std::string& archive) const
{
	const std::string lcArchiveName = StringToLower(archive);
	std::map<std::string, ArchiveInfo>::const_iterator aii = archiveInfos.find(lcArchiveName);
	if (aii != archiveInfos.end()) {
		return aii->second.archiveData;
	}
	return ArchiveData();
}

unsigned char CArchiveScanner::GetMetaFileClass(const std::string& filePath)
{

	unsigned char metaFileClass = 0;

	const std::string lowerFilePath = StringToLower(filePath);
	const std::string ext = FileSystem::GetExtension(lowerFilePath);

	// 1: what is commonly read from all archives when scanning through them
	// 2: what is less commoonly used, or only used when looking
	//    at a specific archive (for example when hosting Game-X)
	if (lowerFilePath == "mapinfo.lua") {                      // basic archive info
		metaFileClass = 1;
	} else if (lowerFilePath == "modinfo.lua") {               // basic archive info
		metaFileClass = 1;
//	} else if ((ext == "smf") || (ext == "sm3")) {             // to generate minimap
//		metaFileClass = 1;
	} else if (lowerFilePath == "modoptions.lua") {            // used by lobbies
		metaFileClass = 2;
	} else if (lowerFilePath == "engineoptions.lua") {         // used by lobbies
		metaFileClass = 2;
	} else if (lowerFilePath == "validmaps.lua") {             // used by lobbies
		metaFileClass = 2;
	} else if (lowerFilePath == "luaai.lua") {                 // used by lobbies
		metaFileClass = 2;
	} else if (StringStartsWith(lowerFilePath, "sidepics/")) { // used by lobbies
		metaFileClass = 2;
	} else if (StringStartsWith(lowerFilePath, "gamedata/")) { // used by lobbies
		metaFileClass = 2;
	} else if (lowerFilePath == "armor.txt") {                 // used by lobbies (disabled units list)
		metaFileClass = 2;
	} else if (lowerFilePath == "springignore.txt") {          // used by lobbies (disabled units list)
		metaFileClass = 2;
	} else if (StringStartsWith(lowerFilePath, "units/")) {    // used by lobbies (disabled units list)
		metaFileClass = 2;
	} else if (StringStartsWith(lowerFilePath, "features/")) { // used by lobbies (disabled units list)
		metaFileClass = 2;
	} else if (StringStartsWith(lowerFilePath, "weapons/")) {  // used by lobbies (disabled units list)
		metaFileClass = 2;
	}
	// Lobbies get the unit list from unitsync. Unitsync gets it by executing
	// gamedata/defs.lua, which loads units, features, weapons, move types
	// and armors (that is why armor.txt is in the list).

	return metaFileClass;
}

