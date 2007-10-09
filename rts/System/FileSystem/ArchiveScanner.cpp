#include "ArchiveScanner.h"

#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>

#include "ArchiveFactory.h"
#include "CRC.h"
#include "FileHandler.h"
#include "Platform/FileSystem.h"
#include "TdfParser.h"
#include "mmgr.h"

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

#define INTERNAL_VER	4

CArchiveScanner* archiveScanner = NULL;

CArchiveScanner::CArchiveScanner(void) :
	isDirty(false)
{
}

CArchiveScanner::~CArchiveScanner(void)
{
	if (isDirty)
		WriteCacheData(filesystem.LocateFile(GetFilename(), FileSystem::WRITE));
}

std::string CArchiveScanner::GetFilename()
{
	char buf[32];
	sprintf(buf, "ArchiveCacheV%i.txt", INTERNAL_VER);
	return string(buf);
}

CArchiveScanner::ModData CArchiveScanner::GetModData(TdfParser* p, const string& section)
{
	ModData md;
	md.name = "";

	if (!p->SectionExist(section)) {
		return md;
	}

	md.name        = p->SGetValueDef("", (section + "\\Name").c_str());
	md.shortName   = p->SGetValueDef("", (section + "\\ShortName").c_str());
	md.version     = p->SGetValueDef("", (section + "\\Version").c_str());
	md.mutator     = p->SGetValueDef("", (section + "\\Mutator").c_str());
	md.description = p->SGetValueDef("", (section + "\\Description").c_str());

	md.modType = atoi(p->SGetValueDef("0", (section + "\\ModType").c_str()).c_str());

	int numDep = atoi(p->SGetValueDef("0", (section + "\\NumDependencies").c_str()).c_str());
	for (int dep = 0; dep < numDep; ++dep) {
		char key[100];
		sprintf(key, "%s\\Depend%d", section.c_str(), dep);
		md.dependencies.push_back(p->SGetValueDef("", key));
	}

	int numReplace = atoi(p->SGetValueDef("0", (section + "\\NumReplaces").c_str()).c_str());
	for (int rep = 0; rep < numReplace; ++rep) {
		char key[100];
		sprintf(key, "%s\\Replace%d", section.c_str(), rep);
		md.replaces.push_back(p->SGetValueDef("", key));
	}

	return md;
}

void CArchiveScanner::Scan(const string& curPath, bool checksum)
{
	isDirty = true;

	std::vector<std::string> found = filesystem.FindFiles(curPath, "*", FileSystem::RECURSE | FileSystem::INCLUDE_DIRS);
	struct stat info;
	for (std::vector<std::string>::iterator it = found.begin(); it != found.end(); ++it) {
		stat(it->c_str(),&info);

		string fullName = *it;
		string fn = filesystem.GetFilename(fullName);
		string fpath = filesystem.GetDirectory(fullName);
		string lcfn = StringToLower(fn);
		string lcfpath = StringToLower(fpath);

		// Exclude archivefiles found inside directory (.sdd) archives.
		string::size_type sdd = lcfpath.find(".sdd");
		if (sdd != string::npos)
			continue;

		// Is this an archive we should look into?
		if (CArchiveFactory::IsArchive(fullName)) {

			// Determine whether to rely on the cached info or not
			bool cached = false;

			map<string, ArchiveInfo>::iterator aii = archiveInfo.find(lcfn);
			if (aii != archiveInfo.end()) {

				// This archive may have been obsoleted, do not process it if so
				if (aii->second.replaced.length() > 0)
					continue;

				if (S_ISDIR(info.st_mode)) {
					struct stat info2;
					std::vector<std::string> sddfiles = filesystem.FindFiles(fpath, "*", FileSystem::RECURSE | FileSystem::INCLUDE_DIRS);
					for (std::vector<std::string>::iterator sddit = found.begin(); sddit != found.end(); ++sddit) {
						stat(sddit->c_str(), &info2);
						if (info.st_mtime < info2.st_mtime) {
							info.st_mtime = info2.st_mtime;
						}
					}
				}

				if (info.st_mtime == aii->second.modified && fpath == aii->second.path) {
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
			if (!cached) {
				CArchiveBase* ar = CArchiveFactory::OpenArchive(fullName);
				if (ar) {
					int cur;
					string name;
					int size;
					ArchiveInfo ai;

					cur = ar->FindFiles(0, &name, &size);
					while (cur != 0) {
						//printf("found %s %d\n", name.c_str(), size);

						string ext = StringToLower(name.substr(name.find_last_of('.') + 1));

						// only accept new format maps
						if (ext == "smf" || ext == "sm3") {
							MapData md;
							if (name.find_last_of('\\') == string::npos && name.find_last_of('/') == string::npos) {
								md.name = name;
								md.virtualPath = "/";
							}
							else {
								if (name.find_last_of('\\') == string::npos) {
									md.name = name.substr(name.find_last_of('/') + 1);
									md.virtualPath = name.substr(0, name.find_last_of('/') + 1);	// include the backslash
								} else {
									md.name = name.substr(name.find_last_of('\\') + 1);
									md.virtualPath = name.substr(0, name.find_last_of('\\') + 1);	// include the backslash
								}
								//md.name = md.name.substr(0, md.name.find_last_of('.'));
							}
							ai.mapData.push_back(md);
						}

						if (name == "modinfo.tdf") {
							int fh = ar->OpenFile(name);
							if (fh) {
								int fsize = ar->FileSize(fh);

								char* buf = SAFE_NEW char[fsize];
								ar->ReadFile(fh, buf, fsize);
								ar->CloseFile(fh);
								try {
									TdfParser p( buf, fsize );
									ai.modData = GetModData(&p, "mod");
								} catch (const TdfParser::parse_error&) {
									// Silently ignore mods with parse errors
								}
								delete [] buf;
							}

						}

						cur = ar->FindFiles(cur, &name, &size);
					}

					ai.path = fpath;
					ai.modified = info.st_mtime;
					ai.origName = fn;
					ai.updated = true;

					delete ar;

					// Optionally calculate a checksum for the file
					// To prevent reading all files in all directory (.sdd) archives every time this function
					// is called, directory archive checksums are calculated on the fly.
					if (checksum) {
						ai.checksum = GetCRC(fullName);
					}
					else {
						ai.checksum = 0;
					}

					archiveInfo[lcfn] = ai;
				}
			}
			else {
				// If cached is true, aii will point to the archive
				if ((checksum) && (aii->second.checksum == 0)) {
					aii->second.checksum = GetCRC(fullName);
				}
			}
		}
	}

	// Now we'll have to parse the replaces-stuff found in the mods
	for (map<string, ArchiveInfo>::iterator aii = archiveInfo.begin(); aii != archiveInfo.end(); ++aii) {
		for (vector<string>::iterator i = aii->second.modData.replaces.begin(); i != aii->second.modData.replaces.end(); ++i) {

			string lcname = StringToLower(*i);

			map<string, ArchiveInfo>::iterator ar = archiveInfo.find(lcname);

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
			ar->second.mapData.clear();
			ar->second.modData.name = "";
			ar->second.modData.replaces.clear();
			ar->second.updated = true;
			ar->second.replaced = aii->first;
		}
	}
}

/** Get CRC of the data in the specified file. Returns 0 if file could not be opened. */
unsigned int CArchiveScanner::GetCRC(const string& filename)
{
	struct stat info;
	unsigned int crc;

	stat(filename.c_str(), &info);
	if (S_ISDIR(info.st_mode)) {

		crc = GetDirectoryCRC(filename);

	} else {

		CRC c;
		if (!c.UpdateFile(filename))
			return 0;
		crc = c.GetCRC();

	}

	// A value of 0 is used to indicate no crc.. so never return that
	// Shouldn't happen all that often
	if (crc == 0)
		return 4711;
	else
		return crc;
}

/** Get combined CRC of all files and filenames in a directory (recursively). */
unsigned int CArchiveScanner::GetDirectoryCRC(const string& curPath)
{
	CRC crc;

	// recurse but don't include directories
	std::vector<std::string> found = filesystem.FindFiles(curPath, "*", FileSystem::RECURSE);

	// sort because order in which find_files finds them is undetermined
	std::sort(found.begin(), found.end());

	for (std::vector<std::string>::iterator it = found.begin(); it != found.end(); it++) {
		const std::string& path(*it);

		// Don't CRC hidden files (starting with a dot).
		if (path.find("/.") != string::npos)
			continue;

		crc.UpdateData(path); // CRC the filename
		crc.UpdateFile(path); // CRC the contents
	}
	return crc.GetCRC();
}

void CArchiveScanner::ReadCacheData(const std::string& filename)
{
	TdfParser p;

	try {
		p.LoadFile(filename);
	} catch (const content_error&) {
		return;
	}

	// Do not load old version caches
	int ver = atoi(p.SGetValueDef("0", "archivecache\\internalver").c_str());
	if (ver != INTERNAL_VER)
		return;

	int numArs = atoi(p.SGetValueDef("0", "archivecache\\numarchives").c_str());
	for (int i = 0; i < numArs; ++i) {
		ArchiveInfo ai;
		char keyb[100];
		sprintf(keyb, "ArchiveCache\\Archive%d\\", i);
		string key = keyb;

		ai.origName = p.SGetValueDef("", key + "Name");

		ai.path = p.SGetValueDef("", key + "Path");
		ai.modified = strtoul(p.SGetValueDef("0", key + "Modified").c_str(), 0, 10);
		ai.checksum = strtoul(p.SGetValueDef("0", key + "Checksum").c_str(), 0, 10);
		ai.updated = false;

		int numMaps = atoi(p.SGetValueDef("0", key + "NumMaps").c_str());
		for (int m = 0; m < numMaps; ++m) {
			char mapb[100];
			sprintf(mapb, "%sMap%d\\", key.c_str(), m);
			string map = mapb;

			MapData md;
			md.name = p.SGetValueDef("", map + "Name");
			md.virtualPath = p.SGetValueDef("", map + "VirtualPath");

			ai.mapData.push_back(md);
		}

		if (p.SectionExist(key + "Mod")) {
			ai.modData = GetModData(&p, key + "Mod");
		}

		string lcname = StringToLower(ai.origName);

		archiveInfo[lcname] = ai;
	}

	isDirty = false;
}

void CArchiveScanner::WriteCacheData(const std::string& filename)
{
	if (!isDirty)
		return;

	FILE* out = fopen(filename.c_str(), "wt");
	if (!out)
		return;

	// First delete all outdated information
	for (map<string, ArchiveInfo>::iterator i = archiveInfo.begin(); i != archiveInfo.end(); ) {
		map<string, ArchiveInfo>::iterator next = i;
		next++;
		if (!i->second.updated) {
			archiveInfo.erase(i);
		}
		i = next;
	}

	fprintf(out, "[ARCHIVECACHE]\n{\n");
	fprintf(out, "\tNumArchives=%d;\n", archiveInfo.size());
	fprintf(out, "\tInternalVer=%d;\n", INTERNAL_VER);
	int cur = 0;
	for (map<string, ArchiveInfo>::iterator i = archiveInfo.begin(); i != archiveInfo.end(); ++i) {
		fprintf(out, "\t[ARCHIVE%d]\n\t{\n", cur);
		fprintf(out, "\t\tName=%s;\n", i->second.origName.c_str());
		fprintf(out, "\t\tPath=%s;\n", i->second.path.c_str());
		fprintf(out, "\t\tModified=%u;\n", i->second.modified);
		fprintf(out, "\t\tChecksum=%u;\n", i->second.checksum);
		fprintf(out, "\t\tReplaced=%s;\n", i->second.replaced.c_str());

		fprintf(out, "\t\tNumMaps=%d;\n", i->second.mapData.size());
		int curmap = 0;
		for (vector<MapData>::iterator mi = i->second.mapData.begin(); mi != i->second.mapData.end(); ++mi) {
			fprintf(out, "\t\t[MAP%d]\n\t\t{\n", curmap);
			//WriteData(out, *mi);
			fprintf(out, "\t\t\tName=%s;\n", (*mi).name.c_str());
			fprintf(out, "\t\t\tVirtualPath=%s;\n", (*mi).virtualPath.c_str());
			//fprintf(out, "\t\t\tDescription=%s;\n", (*mi).description.c_str());
			fprintf(out, "\t\t}\n");
			curmap++;
		}

		// Any mod info? or just a map archive?
		if (i->second.modData.name != "") {
			fprintf(out, "\t\t[MOD]\n\t\t{\n");
			fprintf(out, "\t\t\tName=%s;\n", i->second.modData.name.c_str());
			fprintf(out, "\t\t\tDescription=%s;\n", i->second.modData.description.c_str());
			fprintf(out, "\t\t\tModType=%d;\n", i->second.modData.modType);

			fprintf(out, "\t\t\tNumDependencies=%d;\n", i->second.modData.dependencies.size());
			int curdep = 0;
			for (vector<string>::iterator dep = i->second.modData.dependencies.begin(); dep != i->second.modData.dependencies.end(); ++dep) {
				fprintf(out, "\t\t\tDepend%d=%s;\n", curdep, (*dep).c_str());
				curdep++;
			}

			fprintf(out, "\t\t\tNumReplaces=%d;\n", i->second.modData.replaces.size());
			int currep = 0;
			for (vector<string>::iterator rep = i->second.modData.replaces.begin(); rep != i->second.modData.replaces.end(); ++rep) {
				fprintf(out, "\t\t\tReplace%d=%s;\n", currep++, (*rep).c_str());
			}

			fprintf(out, "\t\t}\n");
		}

		fprintf(out, "\t}\n");
		cur++;
	}
	fprintf(out, "}\n");
	fclose(out);

	isDirty = false;
}

vector<CArchiveScanner::ModData> CArchiveScanner::GetPrimaryMods() const
{
	vector<ModData> ret;

	for (map<string, ArchiveInfo>::const_iterator i = archiveInfo.begin(); i != archiveInfo.end(); ++i) {
		if (i->second.modData.name != "") {

			if (i->second.modData.modType != 1)
				continue;

			// Add the archive the mod is in as the first dependency
			ModData md = i->second.modData;
			md.dependencies.insert(md.dependencies.begin(), i->second.origName);
			ret.push_back(md);
		}
	}

	return ret;
}

vector<string> CArchiveScanner::GetArchives(const string& root)
{
	vector<string> ret;

	string lcname = StringToLower(root);

	map<string, ArchiveInfo>::iterator aii = archiveInfo.find(lcname);
	if (aii == archiveInfo.end())
		return ret;

	// Check if this archive has been replaced
	while (aii->second.replaced.length() > 0) {
		aii = archiveInfo.find(aii->second.replaced);
		if (aii == archiveInfo.end())
			return ret;
	}

	ret.push_back(aii->second.path + aii->second.origName);

	if (aii->second.modData.name == "")
		return ret;

	// add depth-first
	for (vector<string>::iterator i = aii->second.modData.dependencies.begin(); i != aii->second.modData.dependencies.end(); ++i) {
		vector<string> dep = GetArchives(*i);
		for (vector<string>::iterator j = dep.begin(); j != dep.end(); ++j) {
			ret.push_back(*j);
		}
	}

	return ret;
}

vector<string> CArchiveScanner::GetMaps()
{
	vector<string> ret;

	for (map<string, ArchiveInfo>::iterator aii = archiveInfo.begin(); aii != archiveInfo.end(); ++aii) {
		for (vector<MapData>::iterator i = aii->second.mapData.begin(); i != aii->second.mapData.end(); ++i) {
			ret.push_back((*i).name);
		}
	}

	return ret;
}

vector<string> CArchiveScanner::GetArchivesForMap(const string& mapName)
{
	vector<string> ret;

	for (map<string, ArchiveInfo>::iterator aii = archiveInfo.begin(); aii != archiveInfo.end(); ++aii) {
		for (vector<MapData>::iterator i = aii->second.mapData.begin(); i != aii->second.mapData.end(); ++i) {
			if (mapName == (*i).name) {
				return GetArchives(aii->first);
			}
		}
	}

	return ret;
}

unsigned int CArchiveScanner::GetArchiveChecksum(const string& name)
{
	string lcname = name;

	// Strip path-info if present
	if (lcname.find_last_of('\\') != string::npos)
		lcname = lcname.substr(lcname.find_last_of('\\') + 1);
	if (lcname.find_last_of('/') != string::npos)
		lcname = lcname.substr(lcname.find_last_of('/') + 1);

	StringToLowerInPlace(lcname);

	map<string, ArchiveInfo>::iterator aii = archiveInfo.find(lcname);
	if (aii == archiveInfo.end()) {
		return 0;
	}

	return aii->second.checksum;
}

std::string CArchiveScanner::GetArchivePath(const string& name)
{
	string lcname = name;

	// Strip path-info if present
	if (lcname.find_last_of('\\') != string::npos)
		lcname = lcname.substr(lcname.find_last_of('\\') + 1);
	if (lcname.find_last_of('/') != string::npos)
		lcname = lcname.substr(lcname.find_last_of('/') + 1);

	StringToLowerInPlace(lcname);

	map<string, ArchiveInfo>::iterator aii = archiveInfo.find(lcname);
	if (aii == archiveInfo.end()) {
		return 0;
	}

	return aii->second.path;
}

/** Get checksum of all required archives depending on selected mod. */
unsigned int CArchiveScanner::GetModChecksum(const string& root)
{
	unsigned int checksum = 0;
	vector<string> ars = GetArchives(root);

	// Do not include dependencies in base in the checksum:
	// it conflicts badly with the auto-updated default content files,
	// because .sdz files store timestamps too, which may differ
	// each time the file is generated.

	for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i) {
		if (i->find("base") == std::string::npos)
			checksum ^= GetArchiveChecksum(*i);
	}
	return checksum;
}

/** Get checksum of all required archives depending on selected map. */
unsigned int CArchiveScanner::GetMapChecksum(const string& mapName)
{
	unsigned int checksum = 0;
	vector<string> ars = GetArchivesForMap(mapName);
	for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i)
		checksum ^= GetArchiveChecksum(*i);
	return checksum;
}

/** Check if calculated mod checksum equals given checksum. Throws content_error if not equal. */
void CArchiveScanner::CheckMod(const string& root, unsigned checksum)
{
	unsigned local = GetModChecksum(root);
	if (local != checksum) {
		throw content_error(
				"Your mod differs from the host's mod. This may be caused by a\n"
				"missing archive, a corrupted download, or there may even be\n"
				"2 different versions in circulation. Make sure you and the host\n"
				"have installed the chosen mod and it's dependencies and\n"
				"consider redownloading the mod.");
	}
}

/** Check if calculated map checksum equals given checksum. Throws content_error if not equal. */
void CArchiveScanner::CheckMap(const string& mapName, unsigned checksum)
{
	unsigned local = GetMapChecksum(mapName);
	if (local != checksum) {
		throw content_error(
				"Your map differs from the host's map. This may be caused by a\n"
				"missing archive, a corrupted download, or there may even be\n"
				"2 different versions in circulation. Make sure you and the host\n"
				"have installed the chosen map and it's dependencies and\n"
				"consider redownloading the map.");
	}
}

/** Convert mod name to mod primary archive, e.g. ModNameToModArchive("XTA v8.1") returns "xtape.sd7". */
std::string CArchiveScanner::ModNameToModArchive(const std::string& s) const
{
	// Convert mod name to mod archive
	std::vector<ModData> found = GetPrimaryMods();
	for (std::vector<ModData>::iterator it = found.begin(); it != found.end(); ++it) {
		if (it->name == s)
			return it->dependencies.front();
	}
	return s;
}

/** The reverse of ModNameToModArchive() */
std::string CArchiveScanner::ModArchiveToModName(const std::string& s) const
{
	// Convert mod archive to mod name
	std::vector<ModData> found = GetPrimaryMods();
	for (std::vector<ModData>::iterator it = found.begin(); it != found.end(); ++it) {
		if (it->dependencies.front() == s) {
			return it->name;
		}
	}
	return s;
}

/** Convert mod name to mod data struct, can return empty ModData */
CArchiveScanner::ModData CArchiveScanner::ModNameToModData(const std::string& s) const
{
	// Convert mod name to mod archive
	std::vector<ModData> found = GetPrimaryMods();
	for (std::vector<ModData>::iterator it = found.begin(); it != found.end(); ++it) {
		const ModData& md = *it;
		if (md.name == s) {
			return md;
		}
	}
	return ModData();
}

/** Convert mod archive to mod data struct, can return empty ModData */
CArchiveScanner::ModData CArchiveScanner::ModArchiveToModData(const std::string& s) const
{
	// Convert mod archive to mod name
	std::vector<ModData> found = GetPrimaryMods();
	for (std::vector<ModData>::iterator it = found.begin(); it != found.end(); ++it) {
		const ModData& md = *it;
		if (md.dependencies.front() == s) {
			return md;
		}
	}
	return ModData();
}
