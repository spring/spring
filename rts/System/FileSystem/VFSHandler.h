/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _VFS_HANDLER_H
#define _VFS_HANDLER_H

#include <array>
#include <string>
#include <vector>
#include <cinttypes>

#include "System/UnorderedMap.hpp"

class IArchive;

/**
 * Main API for accessing the Virtual File System (VFS).
 * This only allows accessing the VFS (files in archives
 * registered with the VFS), NOT the real file system.
 */
class CVFSHandler
{
public:
	CVFSHandler(const char* s) { SetName(s); ReserveArchives(); }
	~CVFSHandler() { DeleteArchives(); }

	const char* GetName() const { return vfsName; }

	void SetName(const char* s) { vfsName = s; }

	void BlockInsertArchive() { insertAllowed = false; }
	void AllowInsertArchive() { insertAllowed =  true; }
	void BlockRemoveArchive() { removeAllowed = false; }
	void AllowRemoveArchive() { removeAllowed =  true; }


	enum Section: int {
		Mod,
		Map,
		Base,
		Menu,
		Temp,
		TempMod,
		TempMap,
		TempBase,
		TempMenu,
		Count,
		Error
	};

	static Section GetModeSection(char mode);
	static Section GetModTypeSection(int modtype);
	static Section GetArchiveSection(const std::string& archiveName);
	static Section GetTempArchiveSection(const std::string& archiveName) {
		return Section(GetArchiveSection(archiveName) + (Section::TempMod - Section::Mod));
	}


	static void GrabLock();
	static void FreeLock();
	static void FreeInstance(CVFSHandler* handler);
	static void FreeGlobalInstance();
	static void SetGlobalInstance(CVFSHandler* handler);
	static void SetGlobalInstanceRaw(CVFSHandler* handler);

	static CVFSHandler* GetGlobalInstance();


	/**
	 * Checks whether a file exists in the VFS (does not work for dirs).
	 * This is cheaper then calling LoadFile, if you do not require the contents
	 * of the file.
	 * @param filePath raw file path, for example "maps/myMap.smf",
	 *   case-insensitive
	 * @return 1 (or 0 if empty) if the file exists in the VFS, -1 otherwise
	 */
	int FileExists(const std::string& filePath, Section section);

	/**
	 * Returns the absolute path of a VFS file (does not work for dirs).
	 * @param filePath VFS relative file path, for example "maps/myMap.smf",
	 *   case-insensitive
	 * @return absoluteFilePath if the file exists in the VFS, "" otherwise
	 */
	std::string GetFileAbsolutePath(const std::string& filePath, Section section);

	/**
	 * Returns the archive name containing a VFS file (does not work for dirs).
	 * @param filePath VFS relative file path, for example "maps/myMap.smf",
	 *   case-insensitive
	 * @return archiveName if the file exists in the VFS, "" otherwise
	 */
	std::string GetFileArchiveName(const std::string& filePath, Section section);

	/**
	 * Returns a collection of all loaded archives.
	 */
	std::vector<std::string> GetAllArchiveNames() const;

	/**
	 * Reads the contents of a file from within the VFS.
	 * @param filePath raw file path, for example "maps/myMap.smf",
	 *   case-insensitive
	 * @return 1 if the file exists in the VFS and was successfully read
	 */
	int LoadFile(const std::string& filePath, std::vector<std::uint8_t>& buffer, Section section);


	/**
	 * Returns all the files in the given (virtual) directory without the
	 * preceeding pathname.
	 * @param dir raw directory path, for example "maps/" or "maps",
	 *   case-insensitive
	 */
	std::vector<std::string> GetFilesInDir(const std::string& dir, Section section);

	/**
	 * Returns all the sub-directories in the given (virtual) directory without
	 * the preceeding pathname.
	 * @param dir raw directory path, for example "maps/" or "maps",
	 *   case-insensitive
	 */
	std::vector<std::string> GetDirsInDir(const std::string& dir, Section section);


	bool HasTempArchive(const std::string& archiveName) const { return (HasArchive(archiveName, GetTempArchiveSection(archiveName))); }

	bool HasArchive(const std::string& archiveName) const { return (HasArchive(archiveName, GetArchiveSection(archiveName))); }
	bool HasArchive(const std::string& archiveName, Section archiveSection) const;

	/**
	 * Adds an archive to the VFS.
	 * @param override determines whether in case of a conflict, the existing
	 *   entry in the VFS is overwritten or not.
	 */
	bool AddArchive(const std::string& archiveName, bool overwrite);
	bool AddArchiveIf(const std::string& archiveName, bool overwrite) {
		return (!archiveName.empty() && !HasArchive(archiveName) && AddArchive(archiveName, overwrite));
	}

	/**
	 * Adds an archive and all of its dependencies to the VFS.
	 * @param override determines whether an existing entry in the VFS is overwritten or not.
	 */
	bool AddArchiveWithDeps(const std::string& archiveName, bool overwrite);

	/**
	 * Removes an archive from the VFS.
	 * @return true if the archive is not loaded anymore; it was not loaded
	 *   in the first place, or was unloaded successfully.
	 */
	bool RemoveArchive(const std::string& archiveName);

	void DeleteArchives();
	void DeleteArchives(Section section);
	void ReserveArchives();

	void UnMapArchives(bool reload = false);
	void ReMapArchives(bool reload = false);
	void SwapArchiveSections(Section src, Section dst);

private:
	struct FileData {
		IArchive* ar;
		int size;
	};
	typedef std::pair<std::string, FileData> FileEntry;

	std::string GetNormalizedPath(const std::string& rawPath);
	FileData GetFileData(const std::string& normalizedFilePath, Section section) const;

private:
	std::array<std::vector<FileEntry>, Section::Count> files;
	std::array<spring::unordered_map<std::string, IArchive*>, Section::Count> archives;

	const char* vfsName = "";

	bool insertAllowed = true;
	bool removeAllowed = true;
};

#define vfsHandler (CVFSHandler::GetGlobalInstance())

#endif // _VFS_HANDLER_H
