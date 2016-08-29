/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ARCHIVE_SCANNER_H
#define _ARCHIVE_SCANNER_H

#include <string>
#include <vector>
#include <list>
#include <map>
#include "System/Info.h"

class IArchive;
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
	enum {
		hidden = 0,
		primary = 1,
		reserved = 2,
		map = 3,
		base = 4,
		menu = 5
	};
}

class CArchiveScanner
{
public:
	class ArchiveData
	{
	public:
		ArchiveData() {};
		ArchiveData(const LuaTable& archiveTable, bool fromCache);

		/*
		 * These methods are only here for convenience and compile time checks.
		 * These are all the properties used engine internally.
		 * With these methods, we prevent spreading key strings through the
		 * code, which could provoke runtime bugs when edited wrong.
		 * There may well be other info-times supplied by the archive.
		 */
		std::string GetName() const { return GetInfoValueString("name_pure"); }          /// ex:  Original Total Annihilation
		std::string GetNameVersioned() const { return GetInfoValueString("name"); }      /// ex:  Original Total Annihilation v2.3
		std::string GetShortName() const { return GetInfoValueString("shortName"); }     /// ex:  OTA
		std::string GetVersion() const { return GetInfoValueString("version"); }         /// ex:  v2.3
		std::string GetMutator() const { return GetInfoValueString("mutator"); }         /// ex:  deployment
		std::string GetGame() const { return GetInfoValueString("game"); }               /// ex:  Total Annihilation
		std::string GetShortGame() const { return GetInfoValueString("shortGame"); }     /// ex:  TA
		std::string GetDescription() const { return GetInfoValueString("description"); } /// ex:  Little units blowing up other little units
		std::string GetMapFile() const { return GetInfoValueString("mapFile"); }         /// in case its a map, store location of smf/sm3 file
		int GetModType() const { return GetInfoValueInteger("modType"); }                /// 1=primary, 0=hidden, 3=map
		bool GetOnlyLocal() const { return GetInfoValueBool("onlyLocal"); }              /// if true spring will not listen for incoming connections

		const std::map<std::string, InfoItem>& GetInfo() const { return info; }
		std::vector<InfoItem> GetInfoItems() const;

		const std::vector<std::string>& GetDependencies() const { return dependencies; }
		std::vector<std::string>& GetDependencies() { return dependencies; }

		const std::vector<std::string>& GetReplaces() const { return replaces; }
		std::vector<std::string>& GetReplaces() { return replaces; }

		void SetInfoItemValueString(const std::string& key, const std::string& value);
		void SetInfoItemValueInteger(const std::string& key, int value);
		void SetInfoItemValueFloat(const std::string& key, float value);
		void SetInfoItemValueBool(const std::string& key, bool value);

		bool IsValid(std::string& error) const;
		bool IsEmpty() const { return info.empty(); }

		bool IsGame() const { int mt = GetModType(); return mt == modtype::hidden || mt == modtype::primary; }
		bool IsMap() const { int mt = GetModType(); return mt == modtype::map; }

		static bool IsReservedKey(const std::string& keyLower);
		static std::string GetKeyDescription(const std::string& keyLower);

	private:
		InfoValueType GetInfoValueType(const std::string& key) const;
		std::string GetInfoValueString(const std::string& key) const;
		int GetInfoValueInteger(const std::string& key) const;
		float GetInfoValueFloat(const std::string& key) const;
		bool GetInfoValueBool(const std::string& key) const;

		InfoItem* GetInfoItem(const std::string& key);
		const InfoItem* GetInfoItem(const std::string& key) const;

		InfoItem& EnsureInfoItem(const std::string& key);

		std::map<std::string, InfoItem> info;

		std::vector<std::string> dependencies; /// Archives we depend on
		std::vector<std::string> replaces;     /// This archive obsoletes these archives
	};

	CArchiveScanner();
	~CArchiveScanner();

public:
	const std::string& GetFilepath() const;

	std::vector<std::string> GetMaps() const;
	std::vector<ArchiveData> GetPrimaryMods() const;
	std::vector<ArchiveData> GetAllMods() const;
	std::vector<ArchiveData> GetAllArchives() const;

	std::vector<std::string> GetAllArchivesUsedBy(const std::string& root, int depth = 0) const;

public:
	/// checksum of the given archive (without dependencies)
	unsigned int GetSingleArchiveChecksum(const std::string& name);
	/// Calculate checksum of the given archive and all its dependencies
	unsigned int GetArchiveCompleteChecksum(const std::string& name);
	/// like GetArchiveCompleteChecksum, throws exception if mismatch
	void CheckArchive(const std::string& name, unsigned checksum);
	void ScanArchive(const std::string& fullName, bool checksum = false);
	void ScanAllDirs();

	std::string ArchiveFromName(const std::string& s) const;
	std::string NameFromArchive(const std::string& s) const;
	std::string GetArchivePath(const std::string& name) const;
	std::string MapNameToMapFile(const std::string& name) const;
	ArchiveData GetArchiveData(const std::string& name) const;
	ArchiveData GetArchiveDataByArchive(const std::string& archive) const;


private:
	struct ArchiveInfo
	{
		ArchiveInfo()
			: modified(0)
			, checksum(0)
			, updated(false)
			{}
		std::string path;
		std::string origName;     ///< Could be useful to have the non-lowercased name around
		std::string replaced;     ///< If not empty, use that archive instead
		ArchiveData archiveData;
		unsigned int modified;
		unsigned int checksum;
		bool updated;
	};
	struct BrokenArchive
	{
		BrokenArchive()
			: modified(0)
			, updated(false)
			{}
		std::string path;
		unsigned int modified;
		bool updated;
		std::string problem;
	};

private:
	void ScanDirs(const std::vector<std::string>& dirs);
	void ScanDir(const std::string& curPath, std::list<std::string>* foundArchives);

	/// scan mapinfo / modinfo lua files
	bool ScanArchiveLua(IArchive* ar, const std::string& fileName, ArchiveInfo& ai, std::string& err);

	/**
	 * scan archive for map file
	 * @return file name if found, empty string if not
	 */
	std::string SearchMapFile(const IArchive* ar, std::string& error);


	void ReadCacheData(const std::string& filename);
	void WriteCacheData(const std::string& filename);

	IFileFilter* CreateIgnoreFilter(IArchive* ar);

	/**
	 * Get CRC of the data in the specified archive.
	 * Returns 0 if file could not be opened.
	 */
	unsigned int GetCRC(const std::string& filename);
	void ComputeChecksumForArchive(const std::string& filePath);

	bool CheckCachedData(const std::string& fullName, unsigned* modified, bool doChecksum);

	/**
	 * Returns a value > 0 if the file is rated as a meta-file.
	 * First class means, it is essential for the archive, and will be read by
	 * pretty much every software that scans through archives.
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
	static bool CheckCompression(const IArchive* ar,const std::string& fullName, std::string& error);

private:
	std::map<std::string, ArchiveInfo> archiveInfos;
	std::map<std::string, BrokenArchive> brokenArchives;

	bool isDirty;
	std::string cachefile;
};

extern CArchiveScanner* archiveScanner;

#endif // _ARCHIVE_SCANNER_H
