#ifndef __ARCHIVE_SCANNER_H
#define __ARCHIVE_SCANNER_H

#include <string>
#include <vector>
#include <map>

class CArchiveBase;
class IFileFilter;
class LuaParser;
class LuaTable;

/*
 * This class searches through a given directory and its subdirectories looking for archive files.
 * When it finds one, it figures out what kind of archive it is (i.e. if it is a map or a mod currently).
 * This information is cached, so that only modified archives are actually opened. The information can
 * then be retreived by the mod and map selectors.
 *
 * The archive namespace is global, so it is not allowed to have an archive with the same name in more
 * than one folder.
 */

class CArchiveScanner
{
public:
	struct MapData {
		std::string name;
		std::string virtualPath;					// Where in the archive the map can be found
	};

	struct ModData {
		std::string name;							// ex:  Original Total Annihilation v2.3
		std::string shortName;						// ex:  OTA
		std::string version;						// ex:  v2.3
		std::string mutator;						// ex:  deployment
		std::string game;							// ex:  Total Annihilation
		std::string shortGame;						// ex:  TA
		std::string description;					// ex:  Little units blowing up other little units
		int modType;
		std::vector<std::string> dependencies;		// Archives it depends on
		std::vector<std::string> replaces;			// This archive obsoletes these ones
	};

	CArchiveScanner(void);
	virtual ~CArchiveScanner(void);

	std::string GetFilename();

	void ScanDirs(const std::vector<std::string>& dirs, bool checksum = false);

	void ReadCacheData(const std::string& filename);
	void WriteCacheData(const std::string& filename);

	std::vector<ModData> GetPrimaryMods() const;
	std::vector<ModData> GetAllMods() const;
	std::vector<std::string> GetArchives(const std::string& root, int depth = 0);
	std::vector<std::string> GetMaps();
	std::vector<std::string> GetArchivesForMap(const std::string& mapName);
	unsigned int GetArchiveChecksum(const std::string& name);
	std::string GetArchivePath(const std::string& name);
	unsigned int GetModChecksum(const std::string& root);
	unsigned int GetMapChecksum(const std::string& mapName);
	void CheckMod(const std::string& root, unsigned checksum); // these throw a content_error if checksum doesn't match
	void CheckMap(const std::string& mapName, unsigned checksum);
	std::string ModNameToModArchive(const std::string& s) const;
	std::string ModArchiveToModName(const std::string& s) const;
	ModData ModNameToModData(const std::string& s) const;
	ModData ModArchiveToModData(const std::string& s) const;

protected:
	void Scan(const std::string& curPath, bool checksum = false);
	void PreScan(const std::string& curPath);

protected:
	struct ArchiveInfo {
		std::string path;
		std::string origName;					// Could be useful to have the non-lowercased name around
		unsigned int modified;
		std::vector<MapData> mapData;
		ModData modData;
		unsigned int checksum;
		bool updated;
		std::string replaced;					// If not empty, use that archive instead
	};
	std::map<std::string, ArchiveInfo> archiveInfo;
	ModData GetModData(const LuaTable& modTable);
	IFileFilter* CreateIgnoreFilter(CArchiveBase* ar);
	unsigned int GetCRC(const std::string& filename);
	bool isDirty;
	std::string parse_tdf;
	std::string scanutils;
};

extern CArchiveScanner* archiveScanner;

#endif
