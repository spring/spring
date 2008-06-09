#include "ArchiveScanner.h"

#include <list>
#include <algorithm>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "StdAfx.h"
#include "Lua/LuaParser.h"
#include "System/LogOutput.h"
#include "System/FileSystem/ArchiveFactory.h"
#include "System/FileSystem/ArchiveBuffered.h"
#include "System/FileSystem/CRC.h"
#include "System/FileSystem/FileFilter.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/FileSystem.h"
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

#define INTERNAL_VER	7


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
	sprintf(buf, "ArchiveCacheV%i.lua", INTERNAL_VER);
	return std::string(buf);
}

CArchiveScanner::ModData CArchiveScanner::GetModData(const LuaTable* modTable)
{
	ModData md;
	md.name = "";

	if (!modTable->IsValid()) {
		return md;
	}

	md.name        = modTable->GetString("name", "");
	md.shortName   = modTable->GetString("shortName", "");
	md.version     = modTable->GetString("version", "");
	md.mutator     = modTable->GetString("mutator", "");
	md.game        = modTable->GetString("game", "");
	md.shortGame   = modTable->GetString("shortGame", "");
	md.description = modTable->GetString("description", "");
	md.modType     = modTable->GetInt("modType", 0);

	const LuaTable dependencies = modTable->SubTable("depend");
	for (int dep = 1; dependencies.KeyExists(dep); ++dep) {
		md.dependencies.push_back(dependencies.GetString(dep, ""));
	}

	const LuaTable replaces = modTable->SubTable("replace");
	for (int rep = 1; replaces.KeyExists(rep); ++rep) {
		md.replaces.push_back(replaces.GetString(rep, ""));
	}

	// HACK needed until lobbies, lobbyserver and unitsync are sorted out
	// so they can uniquely identify different versions of the same mod.
	// (at time of this writing they use name only)

	// NOTE when changing this, this function is used both by the code that
	// reads ArchiveCache.lua and the code that reads modinfo.lua from the mod.
	// so make sure it doesn't keep adding stuff to the name everytime
	// Spring/unitsync is loaded.

	if (md.name.find(md.version) == std::string::npos) {
		md.name += " " + md.version;
	}

	return md;
}

void CArchiveScanner::PreScan(const std::string& curPath)
{
	const int flags = (FileSystem::INCLUDE_DIRS | FileSystem::RECURSE);
	std::vector<std::string> found = filesystem.FindFiles(curPath, "springcontent.sdz", flags);
	if (!found.empty()) {
		CArchiveBase* ar = CArchiveFactory::OpenArchive(found[0]);
		if (ar) {
			int cur;
			std::string name;
			int size;

			cur = ar->FindFiles(0, &name, &size);
			while (cur != 0) {
				if (name.find("parse_tdf.lua") != string::npos) {
					int fh = ar->OpenFile(name);
					if (fh) {
						int fsize = ar->FileSize(fh);
						char* buf = SAFE_NEW char[fsize];
						ar->ReadFile(fh, buf, fsize);
						ar->CloseFile(fh);
						parse_tdf.append(buf,fsize);
						delete [] buf;
					}
				}
				if (name.find("scanutils.lua") != string::npos) {
					int fh = ar->OpenFile(name);
					if (fh) {
						int fsize = ar->FileSize(fh);
						char* buf = SAFE_NEW char[fsize];
						ar->ReadFile(fh, buf, fsize);
						ar->CloseFile(fh);
						scanutils.append(buf,fsize);
						delete [] buf;
					}
				}
				cur = ar->FindFiles(cur, &name, &size);
			}
			delete ar;
			parse_tdf.erase(parse_tdf.find_last_of("}") + 1); // we don't want to return the table
		}
	}
}

void CArchiveScanner::Scan(const std::string& curPath, bool checksum)
{
	isDirty = true;

	if (curPath.find("base") != string::npos) { // perform the scan for parse_tdf.lua only when scanning base files
	  PreScan(curPath);
	}
	
	const int flags = (FileSystem::INCLUDE_DIRS | FileSystem::RECURSE);
	std::vector<std::string> found = filesystem.FindFiles(curPath, "*", flags);

	for (std::vector<std::string>::iterator it = found.begin(); it != found.end(); ++it) {
		std::string fullName = *it;

		// Strip
		const char lastFullChar = fullName[fullName.size() - 1];
		if ((lastFullChar == '/') || (lastFullChar == '\\')) {
			fullName = fullName.substr(0, fullName.size() - 1);
		}

		const std::string fn    = filesystem.GetFilename(fullName);
		const std::string fpath = filesystem.GetDirectory(fullName);
		const std::string lcfn    = StringToLower(fn);
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

		// Is this an archive we should look into?
		if (CArchiveFactory::IsArchive(fullName)) {
			struct stat info;

			stat(fullName.c_str(), &info);

			// Determine whether to rely on the cached info or not
			bool cached = false;

			std::map<std::string, ArchiveInfo>::iterator aii = archiveInfo.find(lcfn);
			if (aii != archiveInfo.end()) {

				// This archive may have been obsoleted, do not process it if so
				if (aii->second.replaced.length() > 0)
					continue;

				/*
					For truely correct updating of .sdd archives, this code should
					be enabled. Unfortunately it has as side effect that all files
					in all .sdd's always need to be stat()'ed, which really slows
					down program startup.

					An update can be forced anyway by removing ArchiveCacheV*.lua
					or renaming the archive.
				*/

				/*if (S_ISDIR(info.st_mode)) {
					struct stat info2;
					std::vector<std::string> sddfiles = filesystem.FindFiles(fpath, "*", FileSystem::RECURSE | FileSystem::INCLUDE_DIRS);
					for (std::vector<std::string>::iterator sddit = found.begin(); sddit != found.end(); ++sddit) {
						stat(sddit->c_str(), &info2);
						if (info.st_mtime < info2.st_mtime) {
							info.st_mtime = info2.st_mtime;
						}
					}
				}*/

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
			if (!cached) {

				//printf("scanning archive: %s\n", fullName.c_str());

				CArchiveBase* ar = CArchiveFactory::OpenArchive(fullName);
				if (ar) {
					int cur;
					std::string name;
					int size;
					ArchiveInfo ai;

					cur = ar->FindFiles(0, &name, &size);
					while (cur != 0) {
						//printf("found %s %d\n", name.c_str(), size);
						std::string ext = StringToLower(name.substr(name.find_last_of('.') + 1));

						// only accept new format maps
						if (ext == "smf" || ext == "sm3") {
							MapData md;
							if (name.find_last_of('\\') == std::string::npos && name.find_last_of('/') == std::string::npos) {
								md.name = name;
								md.virtualPath = "/";
							}
							else {
								if (name.find_last_of('\\') == std::string::npos) {
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
								
						if (name == "modinfo.lua") {
							int fh = ar->OpenFile(name);
							if (fh) {
								int fsize = ar->FileSize(fh);

								char* buf = SAFE_NEW char[fsize];
								ar->ReadFile(fh, buf, fsize);
								ar->CloseFile(fh);
								
								std::string cleanbuf;
								cleanbuf.append(buf, fsize);
								delete [] buf;
								LuaParser p(cleanbuf, SPRING_VFS_MOD);
								if (!p.Execute()) {
									logOutput.Print(p.GetErrorLog());
								}
								const LuaTable modTable = p.GetRoot();
								ai.modData = GetModData(&modTable);
							}
						}
						
						if (name == "modinfo.tdf") {
						  int fh = ar->OpenFile(name);
							if (fh) {
								int fsize = ar->FileSize(fh);

								char* buf = SAFE_NEW char[fsize];
								ar->ReadFile(fh, buf, fsize);
								ar->CloseFile(fh);
								std::string cleanbuf;
								cleanbuf.append(buf, fsize);
								delete [] buf;
								if (parse_tdf.length() != 0 && scanutils.length() != 0) {
									std::string luaFile = parse_tdf + "\n\n" + scanutils + "\n\n"
																				+ "local tdfModinfo, err = TDFparser.ParseText([[\n" 
																				+ cleanbuf + "]])\n\n"
																				+ "if (tdfModinfo == nil) then\n"
																				+ "    error('Error parsing modinfo.tdf: ' .. err)\n"
																				+ "end\n\n"
																				+ "tdfModinfo.mod['depend'] = MakeArray(tdfModinfo.mod, 'depend')\n"
																				+ "tdfModinfo.mod['replace'] = MakeArray(tdfModinfo.mod, 'replace')\n\n"
																				+ "return tdfModinfo.mod\n";
									LuaParser p(luaFile, SPRING_VFS_MOD);
									if (!p.Execute()) {
										logOutput.Print(p.GetErrorLog());
									}
									const LuaTable modTable = p.GetRoot();
									ai.modData = GetModData(&modTable);
								}
								// silently ignore tdf mods if parse_tdf.lua  or scanutils.lua not found in base
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
	for (std::map<std::string, ArchiveInfo>::iterator aii = archiveInfo.begin(); aii != archiveInfo.end(); ++aii) {
		for (std::vector<std::string>::iterator i = aii->second.modData.replaces.begin(); i != aii->second.modData.replaces.end(); ++i) {

			std::string lcname = StringToLower(*i);
			std::map<std::string, ArchiveInfo>::iterator ar = archiveInfo.find(lcname);

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


IFileFilter* CArchiveScanner::CreateIgnoreFilter(CArchiveBase* ar)
{
	IFileFilter* ignore = IFileFilter::Create();
	int fh = ar->OpenFile("springignore.txt");

	if (fh) {
		int fsize = ar->FileSize(fh);
		char* buf = SAFE_NEW char[fsize];

		ar->ReadFile(fh, buf, fsize);
		ar->CloseFile(fh);

		// this automatically splits lines
		ignore->AddRule(std::string(buf, fsize));

		delete[] buf;
	}
	return ignore;
}


/** Get CRC of the data in the specified archive. Returns 0 if file could not be opened. */

unsigned int CArchiveScanner::GetCRC(const std::string& filename)
{
	CRC crc;
	unsigned int digest;
	CArchiveBase* ar;
	std::list<std::string> files;
	std::string innerName;
	std::string lowerName;
	int innerSize;
	int cur = 0;

	// Try to open an archive
	ar = CArchiveFactory::OpenArchive(filename);
	if (!ar)
		return 0; // It wasn't an archive

	// Load ignore list.
	IFileFilter* ignore = CreateIgnoreFilter(ar);

	// Sort all file paths for deterministic behaviour
	while (true) {
		cur = ar->FindFiles(cur, &innerName, &innerSize);
		if (cur == 0) break;
		if (ignore->Match(innerName)) continue;
		lowerName = StringToLower(innerName); // case insensitive hash
		files.push_back(lowerName);
	}
	files.sort();

	// Add all files in sorted order
	for (std::list<std::string>::iterator i = files.begin(); i != files.end(); i++ ) {
		digest = CRC().Update(i->data(), i->size()).GetDigest();
		crc.Update(digest);
		crc.Update(ar->GetCrc32(*i));
	}
	delete ignore;
	delete ar;

	digest = crc.GetDigest();

	// A value of 0 is used to indicate no crc.. so never return that
	// Shouldn't happen all that often
	if (digest == 0)
		return 4711;
	else
		return digest;
}


void CArchiveScanner::ReadCacheData(const std::string& filename)
{
  LuaParser p(filename, SPRING_VFS_RAW, SPRING_VFS_ALL);
	
	if (!p.Execute()) {
		logOutput.Print(p.GetErrorLog());
	}
	const LuaTable archiveCache = p.GetRoot();
	const LuaTable archives = archiveCache.SubTable("archives");
	
	// Do not load old version caches
	int ver = archiveCache.GetInt("internalVer", 0);
	if (ver != INTERNAL_VER)
		return;

	for (int i = 1; archives.KeyExists(i); ++i) {
	  const LuaTable curArchive = archives.SubTable(i);
		const LuaTable maps = curArchive.SubTable("maps");
		const LuaTable mod = curArchive.SubTable("modData");
		ArchiveInfo ai;

		ai.origName = curArchive.GetString("name", "");
		ai.path = curArchive.GetString("path", "");
		// don't use GetInt for modified and checksum as lua uses 32bit ints, no longs
		ai.modified = strtoul(curArchive.GetString("modified", "0").c_str(), 0, 10);
		ai.checksum = strtoul(curArchive.GetString("checksum", "0").c_str(), 0, 10);
		ai.updated = false;

		for (int m = 1; maps.KeyExists(m); ++m) {
			const LuaTable curMap = maps.SubTable(m);

			MapData md;
			md.name = curMap.GetString("name", "");
			md.virtualPath = curMap.GetString("virtualPath", "");

			ai.mapData.push_back(md);
		}

		ai.modData = GetModData(&mod);

		std::string lcname = StringToLower(ai.origName);

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
	for (std::map<std::string, ArchiveInfo>::iterator i = archiveInfo.begin(); i != archiveInfo.end(); ) {
		std::map<std::string, ArchiveInfo>::iterator next = i;
		next++;
		if (!i->second.updated) {
			archiveInfo.erase(i);
		}
		i = next;
	}

	fprintf(out, "local archiveCache = {\n");
	fprintf(out, "    internalVer = %d,\n", INTERNAL_VER);
	fprintf(out, "    archives = {\n");
	for (std::map<std::string, ArchiveInfo>::iterator i = archiveInfo.begin(); i != archiveInfo.end(); ++i) {
		fprintf(out, "        {\n");
		fprintf(out, "            name = \"%s\",\n", i->second.origName.c_str());
		fprintf(out, "            path = [[%s]],\n", i->second.path.c_str()); // path contains '.\'s, use [[...]] format
		fprintf(out, "            modified = \"%u\",\n", i->second.modified);
		fprintf(out, "            checksum = \"%u\",\n", i->second.checksum);
		fprintf(out, "            replaced = \"%s\",\n", i->second.replaced.c_str());

		fprintf(out, "            maps = {\n");
		for (std::vector<MapData>::iterator mi = i->second.mapData.begin(); mi != i->second.mapData.end(); ++mi) {
			fprintf(out, "                {\n");
			fprintf(out, "                    name = \"%s\",\n", (*mi).name.c_str());
			fprintf(out, "                    virtualPath = \"%s\",\n", (*mi).virtualPath.c_str());
			fprintf(out, "                },\n");
		}
		fprintf(out, "            },\n");

		// Any mod info? or just a map archive?
		if (i->second.modData.name != "") {
			const ModData& md = i->second.modData;
			fprintf(out, "            modData = {\n");
			fprintf(out, "                name = \"%s\",\n",          md.name.c_str());

			if (!md.shortName.empty()) {
				fprintf(out, "                shortName = \"%s\",\n",   md.shortName.c_str());
			}
			if (!md.version.empty()) {
				fprintf(out, "                version = \"%s\",\n",     md.version.c_str());
			}
			if (!md.mutator.empty()) {
				fprintf(out, "                mutator = \"%s\",\n",     md.mutator.c_str());
			}
			if (!md.game.empty()) {
				fprintf(out, "                game = \"%s\",\n",        md.game.c_str());
			}
			if (!md.shortGame.empty()) {
				fprintf(out, "                shortGame = \"%s\",\n",   md.shortGame.c_str());
			}
			if (!md.description.empty()) {
				fprintf(out, "                description = \"%s\",\n", md.description.c_str());
			}
			fprintf(out, "                modType = %d,\n",     md.modType);
			
			fprintf(out, "                depend = {\n");
			for (std::vector<std::string>::iterator dep = i->second.modData.dependencies.begin(); dep != i->second.modData.dependencies.end(); ++dep) {
				fprintf(out, "                    \"%s\",\n", (*dep).c_str());
			}
			fprintf(out, "                },\n");
			
			fprintf(out, "                replace = {\n");
			for (std::vector<std::string>::iterator rep = i->second.modData.replaces.begin(); rep != i->second.modData.replaces.end(); ++rep) {
				fprintf(out, "                    \"%s\",\n", (*rep).c_str());
			}
			
			fprintf(out, "                },\n");
			fprintf(out, "            },\n");
		}

		fprintf(out, "        },\n");
	}
	fprintf(out, "    }\n");
	fprintf(out, "}\n\n");
	fprintf(out, "return archiveCache");
	fclose(out);

	isDirty = false;
}


std::vector<CArchiveScanner::ModData> CArchiveScanner::GetPrimaryMods() const
{
	std::vector<ModData> ret;

	for (std::map<std::string, ArchiveInfo>::const_iterator i = archiveInfo.begin(); i != archiveInfo.end(); ++i) {
		if (i->second.modData.name != "") {

			if (i->second.modData.modType != 1) {
				continue;
			}

			// Add the archive the mod is in as the first dependency
			ModData md = i->second.modData;
			md.dependencies.insert(md.dependencies.begin(), i->second.origName);
			ret.push_back(md);
		}
	}

	return ret;
}


std::vector<CArchiveScanner::ModData> CArchiveScanner::GetAllMods() const
{
	std::vector<ModData> ret;

	for (std::map<std::string, ArchiveInfo>::const_iterator i = archiveInfo.begin(); i != archiveInfo.end(); ++i) {
		if (i->second.modData.name != "") {
			// Add the archive the mod is in as the first dependency
			ModData md = i->second.modData;
			md.dependencies.insert(md.dependencies.begin(), i->second.origName);
			ret.push_back(md);
		}
	}

	return ret;
}


std::vector<std::string> CArchiveScanner::GetArchives(const std::string& root, int depth)
{
	// Protect against circular dependencies
	// (worst case depth is if all archives form one huge dependency chain)
	if ((unsigned)depth > archiveInfo.size()) {
		throw content_error("Circular dependency");
	}

	std::vector<std::string> ret;
	std::string lcname = StringToLower(ModNameToModArchive(root));
	std::map<std::string, ArchiveInfo>::iterator aii = archiveInfo.find(lcname);
	if (aii == archiveInfo.end()) {
		return ret;
	}

	// Check if this archive has been replaced
	while (aii->second.replaced.length() > 0) {
		aii = archiveInfo.find(aii->second.replaced);
		if (aii == archiveInfo.end()) {
			return ret;
		}
	}

	ret.push_back(aii->second.path + aii->second.origName);

	if (aii->second.modData.name == "") {
		return ret;
	}

	// add depth-first
	for (std::vector<std::string>::iterator i = aii->second.modData.dependencies.begin(); i != aii->second.modData.dependencies.end(); ++i) {
		std::vector<std::string> dep = GetArchives(*i, depth + 1);
		for (std::vector<std::string>::iterator j = dep.begin(); j != dep.end(); ++j) {
			ret.push_back(*j);
		}
	}

	// add springcontent.sdz for primary mod archives
	if ((depth == 0) && (aii->second.modData.modType == 1)) {
		const std::string springContentPath = GetArchivePath("springcontent.sdz");
		if (springContentPath.empty()) {
			throw content_error("missing springcontent.sdz");
		} else {
			printf("Added springcontent.sdz for %s\n", root.c_str());
			ret.push_back(springContentPath + "springcontent.sdz");
		}
	}

	return ret;
}


std::vector<std::string> CArchiveScanner::GetMaps()
{
	std::vector<std::string> ret;

	for (std::map<std::string, ArchiveInfo>::iterator aii = archiveInfo.begin(); aii != archiveInfo.end(); ++aii) {
		for (std::vector<MapData>::iterator i = aii->second.mapData.begin(); i != aii->second.mapData.end(); ++i) {
			ret.push_back((*i).name);
		}
	}

	return ret;
}


std::vector<std::string> CArchiveScanner::GetArchivesForMap(const std::string& mapName)
{
	std::vector<std::string> ret;

	for (std::map<std::string, ArchiveInfo>::iterator aii = archiveInfo.begin(); aii != archiveInfo.end(); ++aii) {
		for (std::vector<MapData>::iterator i = aii->second.mapData.begin(); i != aii->second.mapData.end(); ++i) {
			if (mapName == (*i).name) {
				ret = GetArchives(aii->first);
				const std::string mapHelperPath = GetArchivePath("maphelper.sdz");
				if (mapHelperPath.empty()) {
					throw content_error("missing maphelper.sdz");
				} else {
					ret.push_back(mapHelperPath + "maphelper.sdz");
				}
				break;
			}
		}
	}

	return ret;
}


unsigned int CArchiveScanner::GetArchiveChecksum(const std::string& name)
{
	std::string lcname = name;

	// Strip path-info if present
	if (lcname.find_last_of('\\') != std::string::npos)
		lcname = lcname.substr(lcname.find_last_of('\\') + 1);
	if (lcname.find_last_of('/') != std::string::npos)
		lcname = lcname.substr(lcname.find_last_of('/') + 1);

	StringToLowerInPlace(lcname);

	std::map<std::string, ArchiveInfo>::iterator aii = archiveInfo.find(lcname);
	if (aii == archiveInfo.end()) {
		return 0;
	}

	return aii->second.checksum;
}


std::string CArchiveScanner::GetArchivePath(const std::string& name)
{
	std::string lcname = name;

	// Strip path-info if present
	if (lcname.find_last_of('\\') != std::string::npos)
		lcname = lcname.substr(lcname.find_last_of('\\') + 1);
	if (lcname.find_last_of('/') != std::string::npos)
		lcname = lcname.substr(lcname.find_last_of('/') + 1);

	StringToLowerInPlace(lcname);

	std::map<std::string, ArchiveInfo>::iterator aii = archiveInfo.find(lcname);
	if (aii == archiveInfo.end()) {
		return 0;
	}

	return aii->second.path;
}


/** Get checksum of all required archives depending on selected mod. */
unsigned int CArchiveScanner::GetModChecksum(const std::string& root)
{
	unsigned int checksum = 0;
	std::vector<std::string> ars = GetArchives(root);

	for (std::vector<std::string>::iterator i = ars.begin(); i != ars.end(); ++i)
	{
		unsigned tmp = GetArchiveChecksum(*i);
		logOutput.Print("mod checksum %s: %u/%d", i->c_str(), tmp, (int)tmp);
		checksum ^= tmp;
	}
	return checksum;
}


/** Get checksum of all required archives depending on selected map. */
unsigned int CArchiveScanner::GetMapChecksum(const std::string& mapName)
{
	unsigned int checksum = 0;
	std::vector<std::string> ars = GetArchivesForMap(mapName);

	for (std::vector<std::string>::iterator i = ars.begin(); i != ars.end(); ++i) {
		unsigned tmp = GetArchiveChecksum(*i);
		logOutput.Print("map checksum %s: %u/%d", i->c_str(), tmp, (int)tmp);
		checksum ^= tmp;
	}
	return checksum;
}


/** Check if calculated mod checksum equals given checksum. Throws content_error if not equal. */
void CArchiveScanner::CheckMod(const std::string& root, unsigned checksum)
{
	unsigned localChecksum = GetModChecksum(root);
	if (localChecksum != checksum) {
		char msg[1024];
		sprintf(
			msg,
			"Your mod (checksum 0x%x) differs from the host's mod (checksum 0x%x).\n"
			"This may be caused by a missing archive, a corrupted download, or there may even\n"
			"be 2 different versions in circulation. Make sure you and the host have installed\n"
			"the chosen mod and its dependencies and consider redownloading the mod.\n",
			localChecksum, checksum);

		throw content_error(msg);
	}
}


/** Check if calculated map checksum equals given checksum. Throws content_error if not equal. */
void CArchiveScanner::CheckMap(const std::string& mapName, unsigned checksum)
{
	unsigned localChecksum = GetMapChecksum(mapName);
	if (localChecksum != checksum) {
		char msg[1024];
		sprintf(
			msg,
			"Your map (checksum 0x%x) differs from the host's map (checksum 0x%x).\n"
			"This may be caused by a missing archive, a corrupted download, or there may even\n"
			"be 2 different versions in circulation. Make sure you and the host have installed\n"
			"the chosen map and its dependencies and consider redownloading the mod.\n",
			localChecksum, checksum);

		throw content_error(msg);
	}
}


/** Convert mod name to mod primary archive, e.g. ModNameToModArchive("XTA v8.1") returns "xtape.sd7". */
std::string CArchiveScanner::ModNameToModArchive(const std::string& s) const
{
	// Convert mod name to mod archive
	std::vector<ModData> found = GetAllMods();
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
	std::vector<ModData> found = GetAllMods();
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
	std::vector<ModData> found = GetAllMods();
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
	std::vector<ModData> found = GetAllMods();
	for (std::vector<ModData>::iterator it = found.begin(); it != found.end(); ++it) {
		const ModData& md = *it;
		if (md.dependencies.front() == s) {
			return md;
		}
	}
	return ModData();
}
