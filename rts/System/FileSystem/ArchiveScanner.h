#ifndef __ARCHIVE_SCANNER_H
#define __ARCHIVE_SCANNER_H

#include <string>
#include <vector>
#include <map>

#include <stdio.h>

using namespace std;

/*
 * This class searches through a given directory and its subdirectories looking for archive files.
 * When it finds one, it figures out what kind of archive it is (i.e. if it is a map or a mod currently).
 * This information is cached, so that only modified archives are actually opened. The information can
 * then be retreived by the mod and map selectors.
 *
 * The archive namespace is global, so it is not allowed to have an archive with the same name in more
 * than one folder.
 */

class TdfParser;

class CArchiveScanner
{
public:
	struct MapData {
		string name;
		string virtualPath;					// Where in the archive the map can be found
	};
	struct ModData {
		string name;        // ex:  Original Total Annihilation v2.3
		string shortName;   // ex:  OTA
		string version;     // ex:  v2.3
		string mutator;     // ex:  deployment
		string game;        // ex:  Total Annihilation
		string shortGame;   // ex:  TA
		string description; // ex:  Little units blowing up other little units
		int modType;
		vector<string> dependencies;		// Archives it depends on
		vector<string> replaces;			// This archive obsoletes these ones
	};
	CArchiveScanner(void);
	std::string GetFilename();
	void ReadCacheData(const std::string& filename);
	void WriteCacheData(const std::string& filename);
	virtual ~CArchiveScanner(void);
	void Scan(const string& curPath, bool checksum = false);
	vector<ModData> GetPrimaryMods() const;
	vector<ModData> GetAllMods() const;
	vector<string> GetArchives(const string& root, int depth = 0);
	vector<string> GetMaps();
	vector<string> GetArchivesForMap(const string& mapName);
	unsigned int GetArchiveChecksum(const string& name);
	std::string GetArchivePath(const string& name);
	unsigned int GetModChecksum(const string& root);
	unsigned int GetMapChecksum(const string& mapName);
	void CheckMod(const string& root, unsigned checksum); // these throw a content_error if checksum doesn't match
	void CheckMap(const string& mapName, unsigned checksum);
	std::string ModNameToModArchive(const std::string& s) const;
	std::string ModArchiveToModName(const std::string& s) const;
	ModData ModNameToModData(const std::string& s) const;
	ModData ModArchiveToModData(const std::string& s) const;

protected:
	struct ArchiveInfo {
		string path;
		string origName;					// Could be useful to have the non-lowercased name around
		unsigned int modified;
		vector<MapData> mapData;
		ModData modData;
		unsigned int checksum;
		bool updated;
		string replaced;					// If not empty, use that archive instead
	};
	map<string, ArchiveInfo> archiveInfo;
	ModData GetModData(TdfParser* p, const string& section);
	unsigned int GetCRC(const string& filename);
	unsigned int GetDirectoryCRC(const string& curPath);
	bool isDirty;
	//void WriteModData(FILE* out, const ModData& data);	// Helper to write out dependencies
};

extern CArchiveScanner* archiveScanner;

#endif
