#include "StdAfx.h"
#include "ArchiveScanner.h"
#include "ArchiveFactory.h"
#include <algorithm>
#include "TdfParser.h"
#include "Rendering/GL/myGL.h"
#include "FileHandler.h"
#include "Platform/filefunctions.h"

#include <sys/types.h>
#include <sys/stat.h>
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

#define INTERNAL_VER	3

CArchiveScanner* archiveScanner = NULL;

CArchiveScanner::CArchiveScanner(void) :
	isDirty(false)
{
	GenerateCRCTable();
}

CArchiveScanner::~CArchiveScanner(void)
{
	if (isDirty)
		WriteCacheData();
}

CArchiveScanner::ModData CArchiveScanner::GetModData(TdfParser* p, const string& section)
{
	ModData md;
	md.name = "";

	if (!p->SectionExist(section)) 
		return md;

	md.name = p->SGetValueDef("", (section + "\\Name").c_str());
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
//	intptr_t handle;
//	int moreFiles;

	char t[500];
	sprintf(t, "Scanning archives in %s", curPath.c_str());
	//PrintLoadMsg(t);

	isDirty = true;

	fs::path fn(curPath+"/");
	std::vector<fs::path> found = find_files(fn, "*", true, true);
	struct stat info;
	for (std::vector<fs::path>::iterator it = found.begin(); it != found.end(); it++) {
		stat(it->native_file_string().c_str(),&info);

		string fullName = it->native_file_string();
		string fn = it->leaf();
		string fpath = it->branch_path().string();
		string lcfn = fn;
		transform(lcfn.begin(), lcfn.end(), lcfn.begin(), (int (*)(int))tolower); 

		// Is this an archive we should look into?
		if (CArchiveFactory::IsArchive(fullName)) {

			// Determine whether to rely on the cached info or not
			bool cached = false;

			map<string, ArchiveInfo>::iterator aii = archiveInfo.find(lcfn);
			if (aii != archiveInfo.end()) {

				// This archive may have been obsoleted, do not process it if so
				if (aii->second.replaced.length() > 0)
					continue;

				if (info.st_mtime == aii->second.modified) {
					cached = true;
					aii->second.updated = true;
				}

				// If we are here, we could have invalid info in the cache
				// Force a reread if it's a directory archive, as st_mtime only
				// reflects changes to the directory itself, not the contents.
				if (!cached || S_ISDIR(info.st_mode)) {
					cached = false;
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

						string ext = name.substr(name.find_last_of('.') + 1);
						transform(ext.begin(), ext.end(), ext.begin(), (int (*)(int))tolower);

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

								void* buf = malloc(fsize);
								ar->ReadFile(fh, buf, fsize);
								ar->CloseFile(fh);
								try {
									TdfParser p( reinterpret_cast<char*>(buf), fsize );
									ai.modData = GetModData(&p, "mod");
								} catch (const TdfParser::parse_error& e) {
									// Silently ignore mods with parse errors
								}
								free(buf);
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

			string lcname = *i;
			transform(lcname.begin(), lcname.end(), lcname.begin(), (int (*)(int))tolower);

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

/* Code taken from http://paul.rutgers.edu/~rhoads/Code/crc-32b.c */

void CArchiveScanner::GenerateCRCTable()
{
    unsigned int crc, poly;
    int	i, j;

    poly = 0xEDB88320L;
    for (i = 0; i < 256; i++)
        {
        crc = i;
        for (j = 8; j > 0; j--)
            {
            if (crc & 1)
                crc = (crc >> 1) ^ poly;
            else
                crc >>= 1;
            }
        crcTable[i] = crc;
        }
}

unsigned int CArchiveScanner::GetCRC(const string& filename)
{
	unsigned int crc;

	boost::filesystem::path fn(filename, boost::filesystem::native);
	FILE* fp = fopen(fn.native_file_string().c_str(), "rb");
	if (!fp)
		return 0;

    crc = 0xFFFFFFFF;
	char* buf = new char[100000];
	size_t bytes;
	do {
		bytes = fread((void*)buf, 1, 100000, fp);
		for (size_t i = 0; i < bytes; ++i)
			crc = (crc>>8) ^ crcTable[ (crc^(buf[i])) & 0xFF ];
	} while (bytes == 100000);

	delete[] buf;
	fclose(fp);

	crc = crc^0xFFFFFFFF;

	// A value of 0 is used to indicate no crc.. so never return that
	// Shouldn't happen all that often
	if (crc == 0)
		return 4711;
	else
		return crc;
}

void CArchiveScanner::ReadCacheData()
{
	// check if the file exists first
	CFileHandler file("archivecache.txt");
	if (!file.FileExists())
		return;

	TdfParser p("archivecache.txt");

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
		ai.modified = atol(p.SGetValueDef("0", key + "Modified").c_str());
		ai.checksum = atol(p.SGetValueDef("0", key + "Checksum").c_str());
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

		string lcname = ai.origName;
		transform(lcname.begin(), lcname.end(), lcname.begin(), (int (*)(int))tolower);

		archiveInfo[lcname] = ai;
	}

	isDirty = false;
}

void CArchiveScanner::WriteCacheData()
{
	if (!isDirty)
		return;

	boost::filesystem::path fn("archivecache.txt");
	FILE* out = fopen(fn.native_file_string().c_str(), "wt");
	//FILE* out = stdout;
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

vector<CArchiveScanner::ModData> CArchiveScanner::GetPrimaryMods()
{
	vector<ModData> ret;

	for (map<string, ArchiveInfo>::iterator i = archiveInfo.begin(); i != archiveInfo.end(); ++i) {
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

	string lcname = root;
	transform(lcname.begin(), lcname.end(), lcname.begin(), (int (*)(int))tolower);

	map<string, ArchiveInfo>::iterator aii = archiveInfo.find(lcname);
	if (aii == archiveInfo.end())
		return ret;

	// Check if this archive has been replaced
	while (aii->second.replaced.length() > 0) {
		aii = archiveInfo.find(aii->second.replaced);
		if (aii == archiveInfo.end())
			return ret;
	}

	ret.push_back(aii->second.path + "/" + aii->second.origName);

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

	transform(lcname.begin(), lcname.end(), lcname.begin(), (int (*)(int))tolower);

	map<string, ArchiveInfo>::iterator aii = archiveInfo.find(lcname);
	if (aii == archiveInfo.end())
		return 0;

	return aii->second.checksum;
}

/*void CArchiveScanner::WriteData(FILE* out, const Data& data)
{
	fprintf(out, "\t\t\tName=%s;\n", data.name.c_str());
	fprintf(out, "\t\t\tNumDependencies=%d;\n", data.dependencies.size());
	int cur = 0;
	for (vector<string>::const_iterator i = data.dependencies.begin(); i != data.dependencies.end(); ++i) {
		fprintf(out, "\t\t\tDependency%d=%s;\n", cur, (*i).c_str());
		cur++;
	}
}*/

/** Get checksum of all required archives depending on selected mod. */
unsigned int CArchiveScanner::GetChecksum(const string& root)
{
	unsigned int checksum = 0;
	vector<string> ars = archiveScanner->GetArchives(root);
	for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i)
		checksum  ^= archiveScanner->GetArchiveChecksum(*i);
	return checksum;
}

/** Get checksum of all required archives depending on selected map. */
unsigned int CArchiveScanner::GetChecksumForMap(const string& mapName)
{
	unsigned int checksum = 0;
	vector<string> ars = archiveScanner->GetArchivesForMap(mapName);
	for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i)
		checksum ^= archiveScanner->GetArchiveChecksum(*i);
	return checksum;
}
