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
	CVFSHandler() { DeleteArchives(); }
	~CVFSHandler() { DeleteArchives(); }

	enum Section {
		Mod,
		Map,
		Base,
		Menu,
		Temp,
		Count,
		Error
	};

	static Section GetModeSection(char mode);
	static Section GetModTypeSection(int modtype);

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


	/**
	 * Adds an archive to the VFS.
	 * @param override determines whether in case of a  conflict, the existing
	 *   entry in the VFS is overwritten or not.
	 */
	bool AddArchive(const std::string& archiveName, bool overwrite);

	/**
	 * Adds an archive and all of its dependencies to the VFS.
	 * @param override determines whether in case of a  conflict, the existing
	 *   entry in the VFS is overwritten or not.
	 */
	bool AddArchiveWithDeps(const std::string& archiveName, bool overwrite);

	/**
	 * Removes an archive from the VFS.
	 * @return true if the archive is not loaded anymore; it was not loaded
	 *   in the first place, or was unloaded successfully.
	 */
	bool RemoveArchive(const std::string& archiveName);


	void DeleteArchives();

private:
	struct FileData {
		IArchive* ar;
		int size;
	};
	typedef std::pair<std::string, FileData> FileEntry;

	std::array<std::vector<FileEntry>, Section::Count> files;
	spring::unordered_map<std::string, IArchive*> archives;

private:
	std::string GetNormalizedPath(const std::string& rawPath);
	FileData GetFileData(const std::string& normalizedFilePath, Section section);
};

#define vfsHandler (CVFSHandler::GetGlobalInstance())

#endif // _VFS_HANDLER_H
