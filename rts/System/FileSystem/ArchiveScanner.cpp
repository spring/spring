#include "StdAfx.h"

#include <list>
#include <algorithm>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mmgr.h"

#include "ArchiveScanner.h"

#include "Lua/LuaParser.h"
#include "LogOutput.h"
#include "ArchiveFactory.h"
#include "CRC.h"
#include "FileFilter.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/FileSystemHandler.h"
#include "Util.h"
#include "Exceptions.h"

using std::string;
using std::vector;


CLogSubsystem LOG_ARCHIVESCANNER("ArchiveScanner");

// fix for windows
#ifndef S_ISDIR
#define S_ISDIR(x) (((x) & 0170000) == 0040000) /* directory */
#endif

/*
 * The archive scanner is used to find stuff in archives that are needed before building the virtual
 * filesystem. This currently includes maps and mods. It uses caching to speed up the process.
 *
 * It only retrieves info that is used in an initial listing.. For detailed info when selecting a map
 * for example the more specialised parsers will be used. (mapping one archive when selecting a map
 * is not slow, but mapping them all every time to make the list is)
 */

const int INTERNAL_VER = 9;


CArchiveScanner* archiveScanner = NULL;


CArchiveScanner::CArchiveScanner(void)
: isDirty(false)
{
	std::ostringstream file;
	file << "ArchiveCacheV" << INTERNAL_VER << ".lua";
	cachefile = file.str();
	FileSystemHandler& fsh = FileSystemHandler::GetInstance();
	ReadCacheData(fsh.GetWriteDir() + GetFilename());

	const std::vector<std::string>& datadirs = fsh.GetDataDirectories();
	std::vector<std::string> scanDirs;
	for (std::vector<std::string>::const_reverse_iterator d = datadirs.rbegin(); d != datadirs.rend(); ++d)
	{
		scanDirs.push_back(*d + "maps");
		scanDirs.push_back(*d + "base");
		scanDirs.push_back(*d + "mods");
		scanDirs.push_back(*d + "packages");
	}
	ScanDirs(scanDirs, true);
	WriteCacheData(fsh.GetWriteDir() + GetFilename());
}


CArchiveScanner::~CArchiveScanner(void)
{
	if (isDirty)
	{
		WriteCacheData(filesystem.LocateFile(GetFilename(), FileSystem::WRITE));
	}
}


const string& CArchiveScanner::GetFilename() const
{
	return cachefile;
}


CArchiveScanner::ArchiveData CArchiveScanner::GetArchiveData(const LuaTable& archiveTable)
{
	ArchiveData md;
	if (!archiveTable.IsValid())
		return md;

	md.name        = archiveTable.GetString("name",        "");
	md.shortName   = archiveTable.GetString("shortName",   "");
	md.version     = archiveTable.GetString("version",     "");
	md.mutator     = archiveTable.GetString("mutator",     "");
	md.game        = archiveTable.GetString("game",        "");
	md.shortGame   = archiveTable.GetString("shortGame",   "");
	md.description = archiveTable.GetString("description", "");
	md.modType     = archiveTable.GetInt("modType", 0);
	md.mapfile = archiveTable.GetString("mapfile", "");

	const LuaTable dependencies = archiveTable.SubTable("depend");
	for (int dep = 1; dependencies.KeyExists(dep); ++dep)
	{
		const std::string depend = dependencies.GetString(dep, "");
		md.dependencies.push_back(depend);
	}

	const LuaTable replaces = archiveTable.SubTable("replace");
	for (int rep = 1; replaces.KeyExists(rep); ++rep) {
		md.replaces.push_back(replaces.GetString(rep, ""));
	}

	// HACK needed until lobbies, lobbyserver and unitsync are sorted out
	// so they can uniquely identify different versions of the same mod.
	// (at time of this writing they use name only)

	// NOTE when changing this, this function is used both by the code that
	// reads ArchiveCacheV#.lua and the code that reads modinfo.lua from the mod.
	// so make sure it doesn't keep adding stuff to the name everytime
	// Spring/unitsync is loaded.

	if (md.name.find(md.version) == string::npos && !md.version.empty()) {
		md.name += " " + md.version;
	}

	return md;
}

void CArchiveScanner::ScanDirs(const vector<string>& scanDirs, bool doChecksum)
{
	// add the archives
	for (vector<string>::const_iterator it = scanDirs.begin(); it != scanDirs.end(); ++it)
	{
		if (FileSystemHandler::DirExists(*it))
		{
			logOutput.Print("Scanning: %s\n", it->c_str());
			Scan(*it, doChecksum);
		}
	}
}


void CArchiveScanner::Scan(const string& curPath, bool doChecksum)
{
	isDirty = true;

	const int flags = (FileSystem::INCLUDE_DIRS | FileSystem::RECURSE);
	vector<string> found = filesystem.FindFiles(curPath, "*", flags);

	for (vector<string>::iterator it = found.begin(); it != found.end(); ++it) {
		string fullName = *it;

		// Strip
		const char lastFullChar = fullName[fullName.size() - 1];
		if ((lastFullChar == '/') || (lastFullChar == '\\')) {
			fullName = fullName.substr(0, fullName.size() - 1);
		}

		const string fn    = filesystem.GetFilename(fullName);
		const string fpath = filesystem.GetDirectory(fullName);
		const string lcfn    = StringToLower(fn);
		const string lcfpath = StringToLower(fpath);

		// Exclude archivefiles found inside directory archives (.sdd)
		if (lcfpath.find(".sdd") != string::npos) {
			continue;
		}

		// Exclude archivefiles found inside hidden directories
		if ((lcfpath.find("/hidden/")   != string::npos) ||
		    (lcfpath.find("\\hidden\\") != string::npos)) {
			continue;
		}

		// Is this an archive we should look into?
		if (CArchiveFactory::IsScanArchive(fullName))
		{
			ScanArchive(fullName, doChecksum);
		}
	}

	// Now we'll have to parse the replaces-stuff found in the mods
	for (std::map<string, ArchiveInfo>::iterator aii = archiveInfo.begin(); aii != archiveInfo.end(); ++aii)
	{
		for (vector<string>::iterator i = aii->second.archiveData.replaces.begin(); i != aii->second.archiveData.replaces.end(); ++i)
		{

			const string lcname = StringToLower(*i);
			std::map<string, ArchiveInfo>::iterator ar = archiveInfo.find(lcname);

			// If it's not there, we will create a new entry
			if (ar == archiveInfo.end()) {
				ArchiveInfo tmp;
				archiveInfo[lcname] = tmp;
				ar = archiveInfo.find(lcname);
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

void AddDependency(vector<string>& deps, const std::string& dependency)
{
	for (vector<string>::iterator it = deps.begin(); it != deps.end(); ++it)
	{
		if (*it == dependency)
		{
			return;
		}
	}
	deps.push_back(dependency);
};

void CArchiveScanner::ScanArchive(const string& fullName, bool doChecksum)
{
	struct stat info;

	stat(fullName.c_str(), &info);

	const string fn    = filesystem.GetFilename(fullName);
	const string fpath = filesystem.GetDirectory(fullName);
	const string lcfn    = StringToLower(fn);
	const string lcfpath = StringToLower(fpath);

	// Determine whether to rely on the cached info or not
	bool cached = false;

	std::map<string, ArchiveInfo>::iterator aii = archiveInfo.find(lcfn);
	if (aii != archiveInfo.end()) {

		// This archive may have been obsoleted, do not process it if so
		if (aii->second.replaced.length() > 0)
			return;

		if ((unsigned)info.st_mtime == aii->second.modified && fpath == aii->second.path) {
			cached = true;
			aii->second.updated = true;
		}

		// If we are here, we could have invalid info in the cache
		// Force a reread if it's a directory archive, as st_mtime only
		// reflects changes to the directory itself, not the contents.
		if (!cached) {
			archiveInfo.erase(aii);
		}
	}

	// Time to parse the info we are interested in
	if (cached)
	{
		// If cached is true, aii will point to the archive
		if (doChecksum && (aii->second.checksum == 0))
			aii->second.checksum = GetCRC(fullName);
	}
	else
	{
		CArchiveBase* ar = CArchiveFactory::OpenArchive(fullName);
		if (ar)
		{
			ArchiveInfo ai;

			string name;
			int size;

			std::string mapfile;
			bool hasModinfo = false;
			bool hasMapinfo = false;
			for (int cur = 0; (cur = ar->FindFiles(cur, &name, &size)); /* no-op */)
			{
				const string lowerName = StringToLower(name);
				const string ext = lowerName.substr(lowerName.find_last_of('.') + 1);

				if ((ext == "smf") || (ext == "sm3"))
				{
					mapfile = name;
				}
				else if (lowerName == "modinfo.lua")
				{
					hasModinfo = true;
				}
				else if (lowerName == "mapinfo.lua")
				{
					hasMapinfo = true;
				}
			}

			if (hasMapinfo || !mapfile.empty())
			{ // its a map
				if (hasMapinfo)
				{
					ScanArchiveLua(ar, "mapinfo.lua", ai);
				}
				else if (hasModinfo) // backwards-compat for modinfo.lua in maps
				{
					ScanArchiveLua(ar, "modinfo.lua", ai);
				}
				if (ai.archiveData.name.empty())
					ai.archiveData.name = filesystem.GetFilename(mapfile);
				if (ai.archiveData.mapfile.empty())
					ai.archiveData.mapfile = mapfile;
				AddDependency(ai.archiveData.dependencies, "maphelper.sdz");
				ai.archiveData.modType = modtype::map;
				
			}
			else if (hasModinfo)
			{ // mod
				ScanArchiveLua(ar, "modinfo.lua", ai);
				if (ai.archiveData.modType == modtype::primary)
					AddDependency(ai.archiveData.dependencies, "Spring content v1");
			}
			else
			{ // error
			LogObject() << "Failed to read archive, files missing: " << fullName;
				delete ar;
				return;
			}

			ai.path = fpath;
			ai.modified = info.st_mtime;
			ai.origName = fn;
			ai.updated = true;

			delete ar;

			// Optionally calculate a checksum for the file
			// To prevent reading all files in all directory (.sdd) archives
			// every time this function is called, directory archive checksums
			// are calculated on the fly.
			if (doChecksum) {
				ai.checksum = GetCRC(fullName);
			} else {
				ai.checksum = 0;
			}

			archiveInfo[lcfn] = ai;
		}
	}
}

bool CArchiveScanner::ScanArchiveLua(CArchiveBase* ar, const std::string& fileName, ArchiveInfo& ai)
{
	const int fh = ar->OpenFile(fileName);
	if (fh == 0) {
		return false;
	}
	const int fsize = ar->FileSize(fh);

	char* buf = new char[fsize];
	ar->ReadFile(fh, buf, fsize);
	ar->CloseFile(fh);

	const string cleanbuf(buf, fsize);
	delete [] buf;
	LuaParser p(cleanbuf, SPRING_VFS_MOD);
	if (!p.Execute()) {
		logOutput.Print("ERROR in " + fileName + ": " + p.GetErrorLog());
		return false;
	}
	const LuaTable archiveTable = p.GetRoot();
	ai.archiveData = GetArchiveData(archiveTable);
	return true;
}

IFileFilter* CArchiveScanner::CreateIgnoreFilter(CArchiveBase* ar)
{
	IFileFilter* ignore = IFileFilter::Create();
	int fh = ar->OpenFile("springignore.txt");

	if (fh) {
		const int fsize = ar->FileSize(fh);
		char* buf = new char[fsize];

		const int read = ar->ReadFile(fh, buf, fsize);
		ar->CloseFile(fh);

		// this automatically splits lines
		if (read > 0)
			ignore->AddRule(string(buf, read));
		//TODO: figure out why read != fsize sometimes

		delete[] buf;
	}
	return ignore;
}


/** Get CRC of the data in the specified archive.
    Returns 0 if file could not be opened. */
unsigned int CArchiveScanner::GetCRC(const string& arcName)
{
	CRC crc;
	CArchiveBase* ar;
	std::list<string> files;

	// Try to open an archive
	ar = CArchiveFactory::OpenArchive(arcName);
	if (!ar) {
		return 0; // It wasn't an archive
	}

	// Load ignore list.
	IFileFilter* ignore = CreateIgnoreFilter(ar);

	string name;
	int size;
	// Sort all file paths for deterministic behaviour
	for (int cur = 0; (cur = ar->FindFiles(cur, &name, &size)); /* no-op */) {
		if (ignore->Match(name)) {
			continue;
		}
		const string lower = StringToLower(name); // case insensitive hash
		files.push_back(lower);
	}
	files.sort();

	// Add all files in sorted order
	for (std::list<string>::iterator i = files.begin(); i != files.end(); i++ ) {
		const unsigned int nameCRC = CRC().Update(i->data(), i->size()).GetDigest();
		const unsigned int dataCRC = ar->GetCrc32(*i);
		crc.Update(nameCRC);
		crc.Update(dataCRC);
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

void CArchiveScanner::ReadCacheData(const string& filename)
{
	LuaParser p(filename, SPRING_VFS_RAW, SPRING_VFS_BASE);

	if (!p.Execute()) {
		logOutput.Print("ERROR in " + filename + ": " + p.GetErrorLog());
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

		ai.archiveData = GetArchiveData(archived);
		if (ai.archiveData.modType == modtype::map)
			AddDependency(ai.archiveData.dependencies, "maphelper.sdz");
		else if (ai.archiveData.modType == modtype::primary)
			AddDependency(ai.archiveData.dependencies, "Spring content v1");

		string lcname = StringToLower(ai.origName);

		archiveInfo[lcname] = ai;
	}

	isDirty = false;
}

static inline void SafeStr(FILE* out, const char* prefix, const string& str)
{
	if (str.empty()) {
		return;
	}
	if (str.find_first_of("\\\"") == string::npos) {
		fprintf(out, "%s\"%s\",\n", prefix, str.c_str());
	} else {
		fprintf(out, "%s[[%s]],\n", prefix, str.c_str());
	}
}

void FilterDep(vector<string>& deps, const std::string& exclude)
{
	bool clean = true;
	do
	{
		clean = true;
		for (vector<string>::iterator it = deps.begin(); it != deps.end(); ++it)
		{
			if (*it == exclude)
			{
				deps.erase(it);
				clean = false;
				break;
			}
		}
	}
	while (!clean);
};

void CArchiveScanner::WriteCacheData(const string& filename)
{
	if (!isDirty) {
		return;
	}

	FILE* out = fopen(filename.c_str(), "wt");
	if (!out) {
		return;
	}

	// First delete all outdated information
	for (std::map<string, ArchiveInfo>::iterator i = archiveInfo.begin(); i != archiveInfo.end(); ) {
		std::map<string, ArchiveInfo>::iterator next = i;
		next++;
		if (!i->second.updated) {
			archiveInfo.erase(i);
		}
		i = next;
	}

	fprintf(out, "local archiveCache = {\n\n");
	fprintf(out, "\tinternalver = %i,\n\n", INTERNAL_VER);
	fprintf(out, "\tarchives = {  -- count = "_STPF_"\n", archiveInfo.size());

	std::map<string, ArchiveInfo>::const_iterator arcIt;
	for (arcIt = archiveInfo.begin(); arcIt != archiveInfo.end(); ++arcIt) {
		const ArchiveInfo& arcInfo = arcIt->second;

		fprintf(out, "\t\t{\n");
		SafeStr(out, "\t\t\tname = ",              arcInfo.origName);
		SafeStr(out, "\t\t\tpath = ",              arcInfo.path);
		fprintf(out, "\t\t\tmodified = \"%u\",\n", arcInfo.modified);
		fprintf(out, "\t\t\tchecksum = \"%u\",\n", arcInfo.checksum);
		SafeStr(out, "\t\t\treplaced = ",          arcInfo.replaced);

		// mod info?
		const ArchiveData& archData = arcInfo.archiveData;
		if (archData.name != "") {
			fprintf(out, "\t\t\tarchivedata = {\n");
			SafeStr(out, "\t\t\t\tname = ",         archData.name);
			SafeStr(out, "\t\t\t\tshortname = ",    archData.shortName);
			SafeStr(out, "\t\t\t\tversion = ",      archData.version);
			SafeStr(out, "\t\t\t\tmutator = ",      archData.mutator);
			SafeStr(out, "\t\t\t\tgame = ",         archData.game);
			SafeStr(out, "\t\t\t\tmapfile = ",      archData.mapfile);
			SafeStr(out, "\t\t\t\tshortgame = ",    archData.shortGame);
			SafeStr(out, "\t\t\t\tdescription = ",  archData.description);
			fprintf(out, "\t\t\t\tmodtype = %d,\n", archData.modType);

			vector<string> deps = archData.dependencies;
			if (archData.modType == modtype::map)
				FilterDep(deps, "maphelper.sdz");
			else if (archData.modType == modtype::primary)
				FilterDep(deps, "Spring content v1");
			
			if (!deps.empty())
			{
				fprintf(out, "\t\t\t\tdepend = {\n");
				for (unsigned d = 0; d < deps.size(); d++)
				{
					SafeStr(out, "\t\t\t\t\t", deps[d]);
				}
				fprintf(out, "\t\t\t\t},\n");
			}
			fprintf(out, "\t\t\t},\n");
		}

		fprintf(out, "\t\t},\n");
	}

	fprintf(out, "\t},\n"); // close 'archives'
	fprintf(out, "}\n\n"); // close 'archiveCache'
	fprintf(out, "return archiveCache\n");

	fclose(out);

	isDirty = false;
}


vector<CArchiveScanner::ArchiveData> CArchiveScanner::GetPrimaryMods() const
{
	vector<ArchiveData> ret;
	for (std::map<string, ArchiveInfo>::const_iterator i = archiveInfo.begin(); i != archiveInfo.end(); ++i)
	{
		if (i->second.archiveData.name != "" && i->second.archiveData.modType == modtype::primary)
		{
			// Add the archive the mod is in as the first dependency
			ArchiveData md = i->second.archiveData;
			md.dependencies.insert(md.dependencies.begin(), i->second.origName);
			ret.push_back(md);
		}
	}
	return ret;
}


vector<CArchiveScanner::ArchiveData> CArchiveScanner::GetAllMods() const
{
	vector<ArchiveData> ret;
	for (std::map<string, ArchiveInfo>::const_iterator i = archiveInfo.begin(); i != archiveInfo.end(); ++i) {
		if (i->second.archiveData.name != "" && (i->second.archiveData.modType == modtype::primary || i->second.archiveData.modType == modtype::hidden))
		{
			// Add the archive the mod is in as the first dependency
			ArchiveData md = i->second.archiveData;
			md.dependencies.insert(md.dependencies.begin(), i->second.origName);
			ret.push_back(md);
		}
	}

	return ret;
}


vector<string> CArchiveScanner::GetArchives(const string& root, int depth) const
{
	logOutput.Print(LOG_ARCHIVESCANNER, "GetArchives: %s (depth %u)\n", root.c_str(), depth);
	// Protect against circular dependencies
	// (worst case depth is if all archives form one huge dependency chain)
	if ((unsigned)depth > archiveInfo.size()) {
		throw content_error("Circular dependency");
	}

	vector<string> ret;
	string lcname = StringToLower(ArchiveFromName(root));
	std::map<string, ArchiveInfo>::const_iterator aii = archiveInfo.find(lcname);
	if (aii == archiveInfo.end())
	{ // unresolved dep, add anyway so we get propper errorhandling
		ret.push_back(lcname);
		return ret;
	}

	// Check if this archive has been replaced
	while (aii->second.replaced.length() > 0) {
		aii = archiveInfo.find(aii->second.replaced);
		if (aii == archiveInfo.end())
		{
			throw content_error("Unknown error parsing archive replacements");
		}
	}

	ret.push_back(aii->second.path + aii->second.origName);

	// add depth-first
	for (vector<string>::const_iterator i = aii->second.archiveData.dependencies.begin(); i != aii->second.archiveData.dependencies.end(); ++i)
	{
		vector<string> dep = GetArchives(*i, depth + 1);

		for (vector<string>::const_iterator j = dep.begin(); j != dep.end(); ++j)
		{
			if (std::find(ret.begin(), ret.end(), *j) == ret.end())
			{
				// add only if this dependency is not already somewhere
				// in the chain (which can happen if ArchiveCacheV* has
				// not been written yet) so its checksum is not XOR'ed
				// with the running one multiple times (Get*Checksum())
				ret.push_back(*j);
			}
		}
	}

	return ret;
}


vector<string> CArchiveScanner::GetMaps() const
{
	vector<string> ret;

	for (std::map<string, ArchiveInfo>::const_iterator aii = archiveInfo.begin(); aii != archiveInfo.end(); ++aii)
	{
		if (aii->second.archiveData.name != "" && aii->second.archiveData.modType == modtype::map)
			ret.push_back(aii->second.archiveData.name);
	}

	return ret;
}

std::string CArchiveScanner::MapNameToMapFile(const std::string& s) const
{
	// Convert map name to map archive
	for (std::map<string, ArchiveInfo>::const_iterator aii = archiveInfo.begin(); aii != archiveInfo.end(); ++aii)
	{
		if (s == aii->second.archiveData.name)
		{
			return aii->second.archiveData.mapfile;
		}
	}
	logOutput.Print(LOG_ARCHIVESCANNER, "mapfile of %s not found\n", s.c_str());
	return s;
}

unsigned int CArchiveScanner::GetSingleArchiveChecksum(const string& name) const
{
	string lcname = filesystem.GetFilename(name);
	StringToLowerInPlace(lcname);

	std::map<string, ArchiveInfo>::const_iterator aii = archiveInfo.find(lcname);
	if (aii == archiveInfo.end()) {
		logOutput.Print(LOG_ARCHIVESCANNER, "%s checksum: not found (0)\n", name.c_str());
		return 0;
	}

	logOutput.Print(LOG_ARCHIVESCANNER, "%s checksum: %d/%u\n", name.c_str(), aii->second.checksum, aii->second.checksum);
	return aii->second.checksum;
}

unsigned int CArchiveScanner::GetArchiveCompleteChecksum(const std::string& name) const
{
	const vector<string> ars = GetArchives(name);
	unsigned int checksum = 0;

	for (unsigned int a = 0; a < ars.size(); a++) {
		checksum ^= GetSingleArchiveChecksum(ars[a]);
	}
	logOutput.Print(LOG_ARCHIVESCANNER, "archive checksum %s: %d/%u\n", name.c_str(), checksum, checksum);
	return checksum;
}

void CArchiveScanner::CheckArchive(const std::string& name, unsigned checksum) const
{
	unsigned localChecksum = GetArchiveCompleteChecksum(name);

	if (localChecksum != checksum)
	{
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

string CArchiveScanner::GetArchivePath(const string& name) const
{
	string lcname = filesystem.GetFilename(name);
	StringToLowerInPlace(lcname);

	std::map<string, ArchiveInfo>::const_iterator aii = archiveInfo.find(lcname);
	if (aii == archiveInfo.end())
	{
		return "";
	}

	return aii->second.path;
}

std::string CArchiveScanner::ArchiveFromName(const std::string& name) const
{
	for (std::map<string, ArchiveInfo>::const_iterator it = archiveInfo.begin(); it != archiveInfo.end(); ++it)
	{
		if (it->second.archiveData.name == name)
		{
			return it->first;
		}
	}
	return name;
}

string CArchiveScanner::NameFromArchive(const string& s) const
{
	std::map<string, ArchiveInfo>::const_iterator aii = archiveInfo.find(s);
	if (aii != archiveInfo.end())
	{
		return aii->second.archiveData.name;
	}
	return s;
}

CArchiveScanner::ArchiveData CArchiveScanner::GetArchiveData(const std::string& name) const
{
	for (std::map<string, ArchiveInfo>::const_iterator it = archiveInfo.begin(); it != archiveInfo.end(); ++it)
	{
		const ArchiveData& md = it->second.archiveData;
		if (md.name == name)
		{
			return md;
		}
	}
	return ArchiveData();
}

CArchiveScanner::ArchiveData CArchiveScanner::GetArchiveDataByArchive(const std::string& archive) const
{
	return GetArchiveData(NameFromArchive(archive));
}

