/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <list>
#include <algorithm>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

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
#if       !defined(DEDICATED) && !defined(UNITSYNC)
#include "System/Platform/Watchdog.h"
#endif // !defined(DEDICATED) && !defined(UNITSYNC)


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

const int INTERNAL_VER = 9;
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
	{"modtype",     "1=primary, 0=hidden, 3=map", true},
	{"depend",      "a table with all archives that needs to be loaded for this one", false},
	{"replace",     "a table with archives that got replaced with this one", false}
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

	std::vector<std::string>::const_iterator key;
	for (key = keys.begin(); key != keys.end(); ++key) {
		const std::string& keyLower = StringToLower(*key);
		if (!ArchiveData::IsReservedKey(keyLower)) {
			if (keyLower == "modtype") {
				SetInfoItemValueInteger(*key, archiveTable.GetInt(*key, 0));
				continue;
			}

			const int luaType = archiveTable.GetType(*key);
			switch (luaType) {
				case LuaTable::STRING: {
					SetInfoItemValueString(*key, archiveTable.GetString(*key, ""));
				} break;
				case LuaTable::NUMBER: {
					SetInfoItemValueFloat(*key, archiveTable.GetFloat(*key, 0.0f));
				} break;
				case LuaTable::BOOLEAN: {
					SetInfoItemValueBool(*key, archiveTable.GetBool(*key, false));
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
		InfoItem infoItem;
		infoItem.key = key;
		infoItem.valueType = INFO_VALUE_TYPE_INTEGER;
		infoItem.value.typeInteger = 0;

		info[keyLower] = infoItem;
	}

	return info[keyLower];
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
			valueString = info_getValueAsString(infoItem);
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
	std::ostringstream file;

	// the "cache" dir is created in DataDirLocater
	file << FileSystem::GetCacheDir();
	file << FileSystem::GetNativePathSeparator();
	file << "ArchiveCache.lua";

	cachefile = file.str();
	ReadCacheData(dataDirLocater.GetWriteDirPath() + GetFilename());

	const std::vector<std::string>& datadirs = dataDirLocater.GetDataDirPaths();
	std::vector<std::string> scanDirs;
	for (std::vector<std::string>::const_reverse_iterator d = datadirs.rbegin(); d != datadirs.rend(); ++d) {
		scanDirs.push_back(*d + "maps");
		scanDirs.push_back(*d + "base");
		scanDirs.push_back(*d + "games");
		scanDirs.push_back(*d + "packages");
	}
	// ArchiveCache has been parsed at this point --> archiveInfos is populated
	ScanDirs(scanDirs, true);
	WriteCacheData(dataDirLocater.GetWriteDirPath() + GetFilename());
}


CArchiveScanner::~CArchiveScanner()
{
	if (isDirty) {
		WriteCacheData(dataDirsAccess.LocateFile(GetFilename(), FileQueryFlags::WRITE));
	}
}


const std::string& CArchiveScanner::GetFilename() const
{
	return cachefile;
}


void CArchiveScanner::ScanDirs(const std::vector<std::string>& scanDirs, bool doChecksum)
{
	// add the archives
	std::vector<std::string>::const_iterator dir;
	for (dir = scanDirs.begin(); dir != scanDirs.end(); ++dir) {
		if (FileSystem::DirExists(*dir)) {
			LOG("Scanning: %s", dir->c_str());
			Scan(*dir, doChecksum);
		}
	}
}


void CArchiveScanner::Scan(const std::string& curPath, bool doChecksum)
{
	isDirty = true;

	const int flags = (FileQueryFlags::INCLUDE_DIRS | FileQueryFlags::RECURSE);
	const std::vector<std::string> &found = dataDirsAccess.FindFiles(curPath, "*", flags);

	for (std::vector<std::string>::const_iterator it = found.begin(); it != found.end(); ++it) {
		std::string fullName = *it;

		// Strip
		const char lastFullChar = fullName[fullName.size() - 1];
		if ((lastFullChar == '/') || (lastFullChar == '\\')) {
			fullName = fullName.substr(0, fullName.size() - 1);
		}

		const std::string fpath = FileSystem::GetDirectory(fullName);
		const std::string lcfpath = StringToLower(fpath);

		// Exclude archivefiles found inside directory archives (.sdd)
		if (lcfpath.find(".sdd") != std::string::npos) {
			continue;
		}

		// Exclude archivefiles found inside hidden directories
		if ((lcfpath.find("/hidden/")   != std::string::npos) ||
		    (lcfpath.find("\\hidden\\") != std::string::npos)) {
			continue;
		}

#if       !defined(DEDICATED) && !defined(UNITSYNC)
		Watchdog::ClearTimer(WDT_MAIN);
#endif // !defined(DEDICATED) && !defined(UNITSYNC)
		// Is this an archive we should look into?
		if (archiveLoader.IsArchiveFile(fullName)) {
			ScanArchive(fullName, doChecksum);
		}
	}

	// Now we'll have to parse the replaces-stuff found in the mods
	for (std::map<std::string, ArchiveInfo>::iterator aii = archiveInfos.begin(); aii != archiveInfos.end(); ++aii) {
		for (std::vector<std::string>::const_iterator i = aii->second.archiveData.GetReplaces().begin(); i != aii->second.archiveData.GetReplaces().end(); ++i) {

			const std::string lcname = StringToLower(*i);
			std::map<std::string, ArchiveInfo>::iterator ar = archiveInfos.find(lcname);

			// If it's not there, we will create a new entry
			if (ar == archiveInfos.end()) {
				ArchiveInfo tmp;
				archiveInfos[lcname] = tmp;
				ar = archiveInfos.find(lcname);
			}

			// Overwrite the info for this archive with a replaced pointer
			ar->second.path = "";
			ar->second.origName = lcname;
			ar->second.modified = 1;

			ArchiveData empty;
			ar->second.archiveData = empty;
			ar->second.updated = true;
			ar->second.replaced = aii->first;
		}
	}
}

static void AddDependency(std::vector<std::string>& deps, const std::string& dependency)
{
	for (std::vector<std::string>::iterator it = deps.begin(); it != deps.end(); ++it) {
		if (*it == dependency) {
			return;
		}
	}

	deps.push_back(dependency);
}

void CArchiveScanner::ScanArchive(const std::string& fullName, bool doChecksum)
{
	const std::string fn    = FileSystem::GetFilename(fullName);
	const std::string fpath = FileSystem::GetDirectory(fullName);
	const std::string lcfn  = StringToLower(fn);

	// Cache variables
	std::map<std::string, ArchiveInfo>::iterator aii;
	bool cached = false;

	// Stat file
	struct stat info = {0};
	int statfailed = stat(fullName.c_str(), &info);

	// If stat fails, assume the archive is not broken nor cached
	if (!statfailed) {
		// Determine whether this archive has earlier be found to be broken
		std::map<std::string, BrokenArchive>::iterator bai = brokenArchives.find(lcfn);
		if (bai != brokenArchives.end()) {
			if ((unsigned)info.st_mtime == bai->second.modified && fpath == bai->second.path) {
				bai->second.updated = true;
				return;
			}
		}


		// Determine whether to rely on the cached info or not
		if ((aii = archiveInfos.find(lcfn)) != archiveInfos.end()) {
			// This archive may have been obsoleted, do not process it if so
			if (aii->second.replaced.length() > 0) {
				return;
			}

			if ((unsigned)info.st_mtime == aii->second.modified && fpath == aii->second.path) {
				cached = true;
				aii->second.updated = true;
			}

			// If we are here, we could have invalid info in the cache
			// Force a reread if it is a directory archive (.sdd), as
			// st_mtime only reflects changes to the directory itself,
			// not the contents.
			if (!cached) {
				archiveInfos.erase(aii);
			}
		}
	}

	// Time to parse the info we are interested in
	if (cached) {
		// If cached is true, aii will point to the archive
		if (doChecksum && (aii->second.checksum == 0))
			aii->second.checksum = GetCRC(fullName);
	} else {
		IArchive* ar = archiveLoader.OpenArchive(fullName);
		if (!ar || !ar->IsOpen()) {
			LOG("Unable to open archive: %s", fullName.c_str());
			return;
		}

		ArchiveInfo ai;

		std::string error;
		std::string mapfile;

		const bool hasModinfo = ar->FileExists("modinfo.lua");
		const bool hasMapinfo = ar->FileExists("mapinfo.lua");

		// check for smf/sm3 and if the uncompression of important files is too costy
		for (unsigned fid = 0; fid != ar->NumFiles(); ++fid) {
			std::string name;
			int size;
			ar->FileInfo(fid, name, size);
			const std::string lowerName = StringToLower(name);
			const std::string ext = FileSystem::GetExtension(lowerName);

			if ((ext == "smf") || (ext == "sm3")) {
				mapfile = name;
			}

			const unsigned char metaFileClass = GetMetaFileClass(lowerName);
			if ((metaFileClass != 0) && !(ar->HasLowReadingCost(fid))) {
				// is a meta-file and not cheap to read
				if (metaFileClass == 1) {
					// 1st class
					error = "Unpacking/reading cost for meta file " + name
							+ " is too high, please repack the archive (make sure to use a non-solid algorithm, if applicable)";
					break;
				} else if (metaFileClass == 2) {
					// 2nd class
					LOG_SL(LOG_SECTION_ARCHIVESCANNER, L_WARNING,
							"Archive %s: The cost for reading a 2nd-class meta-file is too high: %s",
							fullName.c_str(), name.c_str());
				}
			}
		}

		/*
		if (!error.empty()) {
			// we already have an error, no further evaluation required
		}
		*/
		if (hasMapinfo || !mapfile.empty()) {
			// it is a map
			if (hasMapinfo) {
				ScanArchiveLua(ar, "mapinfo.lua", ai, error);
			} else if (hasModinfo) {
				// backwards-compat for modinfo.lua in maps
				ScanArchiveLua(ar, "modinfo.lua", ai, error);
			}

			if (ai.archiveData.GetName().empty()) {
				// FIXME The name will never be empty, if version is set (see HACK in ArchiveData)
				ai.archiveData.SetInfoItemValueString("name_pure", FileSystem::GetBasename(mapfile));
				ai.archiveData.SetInfoItemValueString("name", FileSystem::GetBasename(mapfile));
			}
			if (ai.archiveData.GetMapFile().empty()) {
				ai.archiveData.SetInfoItemValueString("mapfile", mapfile);
			}

			AddDependency(ai.archiveData.GetDependencies(), "Map Helper v1");
			ai.archiveData.SetInfoItemValueInteger("modType", modtype::map);

			LOG_S(LOG_SECTION_ARCHIVESCANNER, "Found new map: %s",
					ai.archiveData.GetNameVersioned().c_str());
		} else if (hasModinfo) {
			// it is a mod
			ScanArchiveLua(ar, "modinfo.lua", ai, error);
			if (ai.archiveData.GetModType() == modtype::primary) {
				AddDependency(ai.archiveData.GetDependencies(), "Spring content v1");
			}

			LOG_S(LOG_SECTION_ARCHIVESCANNER, "Found new game: %s",
					ai.archiveData.GetNameVersioned().c_str());
		} else {
			// neither a map nor a mod: error
			error = "missing modinfo.lua/mapinfo.lua";
		}

		delete ar;

		if (!error.empty()) {
			// for some reason, the archive is marked as broken
			LOG_L(L_WARNING, "Failed to scan %s (%s)",
					fullName.c_str(), error.c_str());

			// record it as broken, so we don't need to look inside everytime
			BrokenArchive ba;
			ba.path = fpath;
			ba.modified = info.st_mtime;
			ba.updated = true;
			ba.problem = error;
			brokenArchives[lcfn] = ba;
			return;
		}

		ai.path = fpath;
		ai.modified = info.st_mtime;
		ai.origName = fn;
		ai.updated = true;

		// Optionally calculate a checksum for the file
		// To prevent reading all files in all directory (.sdd) archives
		// every time this function is called, directory archive checksums
		// are calculated on the fly.
		if (doChecksum) {
			ai.checksum = GetCRC(fullName);
		} else {
			ai.checksum = 0;
		}

		archiveInfos[lcfn] = ai;
	}
}

bool CArchiveScanner::ScanArchiveLua(IArchive* ar, const std::string& fileName, ArchiveInfo& ai, std::string& err)
{
	std::vector<boost::uint8_t> buf;
	if (!ar->GetFile(fileName, buf)) {
		return false;
	}

	const std::string cleanbuf((char*)(&buf[0]), buf.size());
	LuaParser p(cleanbuf, SPRING_VFS_MOD);

	if (!p.Execute()) {
		err = "Error in " + fileName + ": " + p.GetErrorLog();
		return false;
	}

	const LuaTable& archiveTable = p.GetRoot();

	try {
		ai.archiveData = CArchiveScanner::ArchiveData(archiveTable, false);

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
	if (ar->GetFile("springignore.txt", buf)) {
		// this automatically splits lines
		if (!buf.empty()) {
			const std::string cleanbuf((char*)(&buf[0]), buf.size());
			ignore->AddRule(cleanbuf);
		}
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
	IArchive* ar;
	std::list<std::string> files;

	// Try to open an archive
	ar = archiveLoader.OpenArchive(arcName);
	if (!ar) {
		return 0; // It wasn't an archive
	}

	// Load ignore list.
	IFileFilter* ignore = CreateIgnoreFilter(ar);

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
	for (std::list<std::string>::iterator it = files.begin(); it != files.end(); ++it) {
		crcp.filename = &(*it);
		crcs.push_back(crcp);
	}

	// Compute CRCs of the files
	// Hint: Multithreading only speedups `.sdd` loading. For those the CRC generation is extremely slow -
	//       it has to load the full file to calc it! For the other formats (sd7, sdz, sdp) the CRC is saved
	//       in the metainformation of the container and so the loading is much faster. Neither does any of our
	//       current (2011) packing libraries support multithreading :/
	for_mt(0, crcs.size(), [&](const int i) {
		CRCPair& crcp = crcs[i];
		const unsigned int nameCRC = CRC().Update(crcp.filename->data(), crcp.filename->size()).GetDigest();
		const unsigned fid = ar->FindFile(*crcp.filename);
		const unsigned int dataCRC = ar->GetCrc32(fid);
		crcp.nameCRC = nameCRC;
		crcp.dataCRC = dataCRC;
	#if !defined(DEDICATED) && !defined(UNITSYNC)
		Watchdog::ClearTimer(WDT_MAIN);
	#endif
	});

	// Add file CRCs to the main archive CRC
	for (std::vector<CRCPair>::iterator it = crcs.begin(); it != crcs.end(); ++it) {
		crc.Update(it->nameCRC);
		crc.Update(it->dataCRC);
	#if !defined(DEDICATED) && !defined(UNITSYNC)
		Watchdog::ClearTimer();
	#endif
	}

	delete ignore;
	delete ar;

	unsigned int digest = crc.GetDigest();

	// A value of 0 is used to indicate no crc.. so never return that
	// Shouldn't happen all that often
	if (digest == 0) {
		return 4711;
	} else {
		return digest;
	}
}

void CArchiveScanner::ReadCacheData(const std::string& filename)
{
	if (!FileSystem::FileExists(filename)) {
		LOG_L(L_INFO, "Archive cache doesn't exist: %s", filename.c_str());
		return;
	}

	LuaParser p(filename, SPRING_VFS_RAW, SPRING_VFS_BASE);

	if (!p.Execute()) {
		LOG_L(L_ERROR, "Failed to parse archive cache: %s",
				p.GetErrorLog().c_str());
		return;
	}
	const LuaTable archiveCache = p.GetRoot();
	const LuaTable archives = archiveCache.SubTable("archives");

	// Do not load old version caches
	const int ver = archiveCache.GetInt("internalVer", (INTERNAL_VER + 1));
	if (ver != INTERNAL_VER) {
		return;
	}

	for (int i = 1; archives.KeyExists(i); ++i) {
		const LuaTable curArchive = archives.SubTable(i);
		const LuaTable archived = curArchive.SubTable("archivedata");
		ArchiveInfo ai;

		ai.origName = curArchive.GetString("name", "");
		ai.path     = curArchive.GetString("path", "");

		// do not use LuaTable.GetInt() for 32-bit integers, the Spring lua
		// library uses 32-bit floats to represent numbers, which can only
		// represent 2^24 consecutive integers
		ai.modified = strtoul(curArchive.GetString("modified", "0").c_str(), 0, 10);
		ai.checksum = strtoul(curArchive.GetString("checksum", "0").c_str(), 0, 10);
		ai.updated = false;

		ai.archiveData = CArchiveScanner::ArchiveData(archived, true);
		if (ai.archiveData.GetModType() == modtype::map) {
			AddDependency(ai.archiveData.GetDependencies(), "Map Helper v1");
		} else if (ai.archiveData.GetModType() == modtype::primary) {
			AddDependency(ai.archiveData.GetDependencies(), "Spring content v1");
		}

		std::string lcname = StringToLower(ai.origName);
		archiveInfos[lcname] = ai;
	}

	const LuaTable brokenArchives = archiveCache.SubTable("brokenArchives");

	for (int i = 1; brokenArchives.KeyExists(i); ++i) {
		const LuaTable curArchive = brokenArchives.SubTable(i);
		BrokenArchive ba;
		std::string name = curArchive.GetString("name", "");

		ba.path = curArchive.GetString("path", "");
		ba.modified = strtoul(curArchive.GetString("modified", "0").c_str(), 0, 10);
		ba.updated = false;
		ba.problem = curArchive.GetString("problem", "unknown");

		StringToLowerInPlace(name);
		this->brokenArchives[name] = ba;
	}

	isDirty = false;
}

static inline void SafeStr(FILE* out, const char* prefix, const std::string& str)
{
	if (str.empty()) {
		return;
	}
	if (str.find_first_of("\\\"") == std::string::npos) {
		fprintf(out, "%s\"%s\",\n", prefix, str.c_str());
	} else {
		fprintf(out, "%s[[%s]],\n", prefix, str.c_str());
	}
}

void FilterDep(std::vector<std::string>& deps, const std::string& exclude)
{
	bool clean = true;
	do {
		clean = true;
		for (std::vector<std::string>::iterator it = deps.begin(); it != deps.end(); ++it) {
			if (*it == exclude) {
				deps.erase(it);
				clean = false;
				break;
			}
		}
	} while (!clean);
};

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
	// TODO: this pattern should be moved into an utility function..
	for (std::map<std::string, ArchiveInfo>::iterator i = archiveInfos.begin(); i != archiveInfos.end(); ) {
		if (!i->second.updated) {
			i = set_erase(archiveInfos, i);
		} else {
			++i;
		}
	}
	for (std::map<std::string, BrokenArchive>::iterator i = brokenArchives.begin(); i != brokenArchives.end(); ) {
		if (!i->second.updated) {
			i = set_erase(brokenArchives, i);
		} else {
			++i;
		}
	}

	fprintf(out, "local archiveCache = {\n\n");
	fprintf(out, "\tinternalver = %i,\n\n", INTERNAL_VER);
	fprintf(out, "\tarchives = {  -- count = " _STPF_ "\n", archiveInfos.size());

	std::map<std::string, ArchiveInfo>::const_iterator arcIt;
	for (arcIt = archiveInfos.begin(); arcIt != archiveInfos.end(); ++arcIt) {
		const ArchiveInfo& arcInfo = arcIt->second;

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

			const std::map<std::string, InfoItem>& info = archData.GetInfo();
			std::map<std::string, InfoItem>::const_iterator ii;
			for (ii = info.begin(); ii != info.end(); ++ii) {
				switch (ii->second.valueType) {
					case INFO_VALUE_TYPE_STRING: {
						SafeStr(out, std::string("\t\t\t\t" + ii->first + " = ").c_str(), ii->second.valueTypeString);
					} break;
					case INFO_VALUE_TYPE_INTEGER: {
						fprintf(out, "\t\t\t\t%s = %d,\n", ii->first.c_str(), ii->second.value.typeInteger);
					} break;
					case INFO_VALUE_TYPE_FLOAT: {
						fprintf(out, "\t\t\t\t%s = %f,\n", ii->first.c_str(), ii->second.value.typeFloat);
					} break;
					case INFO_VALUE_TYPE_BOOL: {
						fprintf(out, "\t\t\t\t%s = %d,\n", ii->first.c_str(), (int)ii->second.value.typeBool);
					} break;
				}
			}

			std::vector<std::string> deps = archData.GetDependencies();
			if (archData.GetModType() == modtype::map) {
				FilterDep(deps, "Map Helper v1");
			} else if (archData.GetModType() == modtype::primary) {
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

	fprintf(out, "\tbrokenArchives = {  -- count = " _STPF_ "\n", brokenArchives.size());

	std::map<std::string, BrokenArchive>::const_iterator bai;
	for (bai = brokenArchives.begin(); bai != brokenArchives.end(); ++bai) {
		const BrokenArchive& ba = bai->second;

		fprintf(out, "\t\t{\n");
		SafeStr(out, "\t\t\tname = ", bai->first);
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


static bool archNameCompare(const CArchiveScanner::ArchiveData& a, const CArchiveScanner::ArchiveData& b)
{
	return (a.GetNameVersioned() < b.GetNameVersioned());
}
static void sortByName(std::vector<CArchiveScanner::ArchiveData>& data)
{
	std::sort(data.begin(), data.end(), archNameCompare);
}

std::vector<CArchiveScanner::ArchiveData> CArchiveScanner::GetPrimaryMods() const
{
	std::vector<ArchiveData> ret;

	for (std::map<std::string, ArchiveInfo>::const_iterator i = archiveInfos.begin(); i != archiveInfos.end(); ++i) {
		if (!(i->second.archiveData.GetName().empty()) && (i->second.archiveData.GetModType() == modtype::primary)) {
			// Add the archive the mod is in as the first dependency
			ArchiveData md = i->second.archiveData;
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
		if (!(i->second.archiveData.GetName().empty()) && ((i->second.archiveData.GetModType() == modtype::primary) || (i->second.archiveData.GetModType() == modtype::hidden))) {
			// Add the archive the mod is in as the first dependency
			ArchiveData md = i->second.archiveData;
			md.GetDependencies().insert(md.GetDependencies().begin(), i->second.origName);
			ret.push_back(md);
		}
	}

	sortByName(ret);

	return ret;
}


std::vector<std::string> CArchiveScanner::GetArchives(const std::string& root, int depth) const
{
	LOG_S(LOG_SECTION_ARCHIVESCANNER, "GetArchives: %s (depth %u)",
			root.c_str(), depth);
	// Protect against circular dependencies
	// (worst case depth is if all archives form one huge dependency chain)
	if ((unsigned)depth > archiveInfos.size()) {
		throw content_error("Circular dependency");
	}

	std::vector<std::string> ret;
	std::string lcname = StringToLower(ArchiveFromName(root));
	std::map<std::string, ArchiveInfo>::const_iterator aii = archiveInfos.find(lcname);
	if (aii == archiveInfos.end()) {
#ifdef UNITSYNC
		// unresolved dep, add it, so unitsync still shows this file
		if (!ret.empty()) {
			ret.push_back(lcname);
		}
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
			throw content_error("Unknown error parsing archive replacements");
		}
	}

	ret.push_back(aii->second.path + aii->second.origName);

	// add depth-first
	for (std::vector<std::string>::const_iterator i = aii->second.archiveData.GetDependencies().begin(); i != aii->second.archiveData.GetDependencies().end(); ++i) {
		const std::vector<std::string>& deps = GetArchives(*i, depth + 1);

		for (std::vector<std::string>::const_iterator j = deps.begin(); j != deps.end(); ++j) {
			if (std::find(ret.begin(), ret.end(), *j) == ret.end()) {
				// add only if this dependency is not already somewhere
				// in the chain (which can happen if ArchiveCache.lua has
				// not been written yet) so its checksum is not XOR'ed
				// with the running one multiple times (Get*Checksum())
				ret.push_back(*j);
			}
		}
	}

	return ret;
}


std::vector<std::string> CArchiveScanner::GetMaps() const
{
	std::vector<std::string> ret;

	for (std::map<std::string, ArchiveInfo>::const_iterator aii = archiveInfos.begin(); aii != archiveInfos.end(); ++aii) {
		if (!(aii->second.archiveData.GetName().empty()) && aii->second.archiveData.GetModType() == modtype::map) {
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
	LOG_SL(LOG_SECTION_ARCHIVESCANNER, L_WARNING,
			"map file of %s not found", s.c_str());
	return s;
}

unsigned int CArchiveScanner::GetSingleArchiveChecksum(const std::string& name) const
{
	std::string lcname = FileSystem::GetFilename(name);
	StringToLowerInPlace(lcname);

	std::map<std::string, ArchiveInfo>::const_iterator aii = archiveInfos.find(lcname);
	if (aii == archiveInfos.end()) {
		LOG_SL(LOG_SECTION_ARCHIVESCANNER, L_WARNING,
				"%s checksum: not found (0)", name.c_str());
		return 0;
	}

	LOG_S(LOG_SECTION_ARCHIVESCANNER,"%s checksum: %d/%u",
			name.c_str(), aii->second.checksum, aii->second.checksum);
	return aii->second.checksum;
}

unsigned int CArchiveScanner::GetArchiveCompleteChecksum(const std::string& name) const
{
	const std::vector<std::string> &ars = GetArchives(name);
	unsigned int checksum = 0;

	for (unsigned int a = 0; a < ars.size(); a++) {
		checksum ^= GetSingleArchiveChecksum(ars[a]);
	}
	LOG_S(LOG_SECTION_ARCHIVESCANNER, "archive checksum %s: %d/%u",
			name.c_str(), checksum, checksum);
	return checksum;
}

void CArchiveScanner::CheckArchive(const std::string& name, unsigned checksum) const
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
	return GetArchiveData(NameFromArchive(archive));
}

unsigned char CArchiveScanner::GetMetaFileClass(const std::string& filePath) {

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

