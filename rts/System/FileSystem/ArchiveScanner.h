/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ARCHIVE_SCANNER_H
#define _ARCHIVE_SCANNER_H

#include <string>
#include <vector>
#include <map>

class CArchiveBase;
class IFileFilter;
class LuaTable;

/*
 * This class searches through a given directory and its sub-directories looking
 * for archive files.
 * When it finds one, it figures out what kind of archive it is (i.e. if it is a
 * map or a mod currently). This information is cached, so that only modified
 * archives are actually opened. The information can then be retreived by the
 * mod and map selectors.
 *
 * The archive namespace is global, so it is not allowed to have an archive with
 * the same name in more than one folder.
 */

namespace modtype
{
	static const int primary = 1;
	static const int hidden = 0;
	static const int map = 3;
};

class CArchiveScanner
{
public:
	struct ArchiveData
	{
		std::string name;                      ///< ex:  Original Total Annihilation v2.3
		std::string shortName;                 ///< ex:  OTA
		std::string version;                   ///< ex:  v2.3
		std::string mutator;                   ///< ex:  deployment
		std::string game;                      ///< ex:  Total Annihilation
		std::string shortGame;                 ///< ex:  TA
		std::string description;               ///< ex:  Little units blowing up other little units
		std::string mapfile;                   ///< in case its a map, store location of smf/sm3 file
		int modType;                           ///< 1=primary, 0=hidden, 3=map
		std::vector<std::string> dependencies; ///< Archives it depends on
		std::vector<std::string> replaces;     ///< This archive obsoletes these ones
	};

	CArchiveScanner();
	~CArchiveScanner();

	const std::string& GetFilename() const;

	std::vector<ArchiveData> GetPrimaryMods() const;
	std::vector<ArchiveData> GetAllMods() const;
	std::vector<std::string> GetArchives(const std::string& root, int depth = 0) const;
	std::vector<std::string> GetMaps() const;

	/// checksum of the given archive (without dependencies)
	unsigned int GetSingleArchiveChecksum(const std::string& name) const;
	/// Calculate checksum of the given archive and all its dependencies
	unsigned int GetArchiveCompleteChecksum(const std::string& name) const;
	/// like GetArchiveCompleteChecksum, throws exception if mismatch
	void CheckArchive(const std::string& name, unsigned checksum) const;

	std::string ArchiveFromName(const std::string& s) const;
	std::string NameFromArchive(const std::string& s) const;
	std::string GetArchivePath(const std::string& name) const;
	std::string MapNameToMapFile(const std::string& name) const;
	ArchiveData GetArchiveData(const std::string& name) const;
	ArchiveData GetArchiveDataByArchive(const std::string& archive) const;

	/**
	 * Returns a value > 0 if the file is rated as a meta-file.
	 * First class means, it is essential for the archive, and will be read by
	 * pretty much every software that scanns through archives.
	 * Second class means, it is not essential for the archive, but it may be
	 * read by certain tools that generate spezialized indices while scanning
	 * through archives.
	 * Examples:
	 * "objects3d/ddm.s3o" -> 0
	 * "ModInfo.lua" -> 1
	 * "maps/dsd.smf" -> 1
	 * "LuaAI.lua" -> 2
	 * "sides/arm.bmp" -> 2
	 * @param filePath file path, relative to archive-root
	 * @return 0 if the file is not a meta-file,
	 *         1 if the file is a first class meta-file,
	 *         2 if the file is a second class meta-file
	 */
	static unsigned char GetMetaFileClass(const std::string& filePath);

private:
	struct ArchiveInfo
	{
		std::string path;
		std::string origName;     ///< Could be useful to have the non-lowercased name around
		unsigned int modified;
		ArchiveData archiveData;
		unsigned int checksum;
		bool updated;
		std::string replaced;     ///< If not empty, use that archive instead
	};
	struct BrokenArchive
	{
		std::string path;
		unsigned int modified;
		bool updated;
		std::string problem;
	};

	void ScanDirs(const std::vector<std::string>& dirs, bool checksum = false);
	void Scan(const std::string& curPath, bool doChecksum);

	void ScanArchive(const std::string& fullName, bool checksum = false);
	/// scan mapinfo / modinfo lua files
	bool ScanArchiveLua(CArchiveBase* ar, const std::string& fileName, ArchiveInfo& ai);

	void ReadCacheData(const std::string& filename);
	void WriteCacheData(const std::string& filename);

	std::map<std::string, ArchiveInfo> archiveInfo;
	std::map<std::string, BrokenArchive> brokenArchives;
	ArchiveData GetArchiveData(const LuaTable& archiveTable);
	IFileFilter* CreateIgnoreFilter(CArchiveBase* ar);
	/**
	 * Get CRC of the data in the specified archive.
     * Returns 0 if file could not be opened.
	 */
	unsigned int GetCRC(const std::string& filename);
	bool isDirty;
	std::string cachefile;
};

extern CArchiveScanner* archiveScanner;

#endif // _ARCHIVE_SCANNER_H
