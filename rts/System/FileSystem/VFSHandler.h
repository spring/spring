/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _VFS_HANDLER_H
#define _VFS_HANDLER_H

#include <map>
#include <string>
#include <vector>
#include <boost/cstdint.hpp>

class IArchive;

/**
 * Main API for accessing the Virtual File System (VFS).
 * This only allows accessing the VFS (stuff within archives registered with the
 * VFS), NOT the real file system.
 */
class CVFSHandler
{
public:
	CVFSHandler();
	~CVFSHandler();

	/**
	 * Checks whether a file exists in the VFS (does not work for dirs).
	 * This is cheaper then calling LoadFile, if you do not require the contents
	 * of the file.
	 * @param filePath raw file path, for example "maps/myMap.smf",
	 *   case-insensitive
	 * @return true if the file exists in the VFS, false otherwise
	 */
	bool FileExists(const std::string& filePath);
	/**
	 * Reads the contents of a file from within the VFS.
	 * @param filePath raw file path, for example "maps/myMap.smf",
	 *   case-insensitive
	 * @return true if the file exists in the VFS and was successfully read
	 */
	bool LoadFile(const std::string& filePath, std::vector<boost::uint8_t>& buffer);

	/**
	 * Returns all the files in the given (virtual) directory without the
	 * preceeding pathname.
	 * @param dir raw directory path, for example "maps/" or "maps",
	 *   case-insensitive
	 */
	std::vector<std::string> GetFilesInDir(const std::string& dir);
	/**
	 * Returns all the sub-directories in the given (virtual) directory without
	 * the preceeding pathname.
	 * @param dir raw directory path, for example "maps/" or "maps",
	 *   case-insensitive
	 */
	std::vector<std::string> GetDirsInDir(const std::string& dir);

	/**
	 * Adds an archive to the VFS.
	 * @param override determines whether in case of a  conflict, the existing
	 *   entry in the VFS is overwritten or not.
	 */
	bool AddArchive(const std::string& archiveName, bool override, const std::string& type = "");
	/**
	 * Adds an archive and all of its dependencies to the VFS.
	 * @param override determines whether in case of a  conflict, the existing
	 *   entry in the VFS is overwritten or not.
	 */
	bool AddArchiveWithDeps(const std::string& archiveName, bool override, const std::string& type = "");

	/**
	 * Removes an archive from the VFS.
	 * @return true if the archive is not loaded anymore; it was not loaded
	 *   in the first place, or was unloaded successfully.
	 */
	bool RemoveArchive(const std::string& archiveName);

protected:
	struct FileData {
		IArchive* ar;
		int size;
	};
	std::map<std::string, FileData> files; 
	std::map<std::string, IArchive*> archives;

private:
	std::string GetNormalizedPath(const std::string& rawPath);
	const FileData* GetFileData(const std::string& normalizedFilePath);
};

extern CVFSHandler* vfsHandler;

#endif // _VFS_HANDLER_H
