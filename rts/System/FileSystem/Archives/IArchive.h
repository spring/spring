/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ARCHIVE_BASE_H
#define _ARCHIVE_BASE_H

#include <string>
#include <vector>
#include <map>
#include <boost/cstdint.hpp>

/**
 * @brief Abstraction of different archive types
 *
 * Loosely resembles STL container:
 * for (unsigned int fid = 0; fid < NumFiles(); ++fid) {
 * 	//stuff
 * }
 */
class IArchive
{
protected:
	IArchive(const std::string& archiveName);

public:
	virtual ~IArchive();

	virtual bool IsOpen() = 0;
	const std::string& GetArchiveName() const;
	
	/**
	 * @return The amount of files in the archive, does not change during
	 * lifetime
	 */
	virtual unsigned int NumFiles() const = 0;
	/**
	 * Returns whether the supplied fileId is valid and available in this
	 * archive.
	 */
	inline bool IsFileId(unsigned int fileId) const {
		return (fileId < NumFiles());
	}
	/**
	 * Returns true if the file exists in this archive.
	 * @param normalizedFilePath VFS path to the file in lower-case,
	 *   using forward-slashes, for example "maps/mymap.smf"
	 * @return true if the file exists in this archive, false otherwise
	 */
	bool FileExists(const std::string& normalizedFilePath) const;
	/**
	 * Returns the fileID of a file.
	 * @param filePath VFS path to the file, for example "maps/myMap.smf"
	 * @return fileID of the file, NumFiles() if not found
	 */
	unsigned int FindFile(const std::string& filePath) const;
	/**
	 * Fetches the content of a file by its ID.
	 * @param fid file ID in [0, NumFiles())
	 * @param buffer on success, this will be filled with the contents
	 *   of the file
	 * @return true if the file was found, and its contents have been
	 *   successfully read into buffer
	 * @see GetFile(unsigned int fid, std::vector<boost::uint8_t>& buffer)
	 */
	virtual bool GetFile(unsigned int fid, std::vector<boost::uint8_t>& buffer) = 0;
	/**
	 * Fetches the content of a file by its name.
	 * @param name VFS path to the file, for example "maps/myMap.smf"
	 * @param buffer on success, this will be filled with the contents
	 *   of the file
	 * @return true if the file was found, and its contents have been
	 *   successfully read into buffer
	 * @see GetFile(unsigned int fid, std::vector<boost::uint8_t>& buffer)
	 */
	bool GetFile(const std::string& name, std::vector<boost::uint8_t>& buffer);
	/**
	 * Fetches the name and size in bytes of a file by its ID.
	 */
	virtual void FileInfo(unsigned int fid, std::string& name, int& size) const = 0;
	/**
	 * Returns true if the cost of reading the file is qualitatively relative
	 * to its file-size.
	 * This is mainly usefull in the case of solid archives,
	 * which may make the reading of a single small file over proportionally
	 * expensive.
	 * The returned value is usually relative to certain arbitrary chosen
	 * constants.
	 * Most implementations may always return true.
	 * @return true if cost is ~ relative to its file-size
	 */
	virtual bool HasLowReadingCost(unsigned int fid) const;

	/**
	 * @return true if archive type can be packed solid (which is VERY slow when reading)
	 */
	virtual bool CheckForSolid() const { return false; }
	/**
	 * Fetches the CRC32 hash of a file by its ID.
	 */
	virtual unsigned int GetCrc32(unsigned int fid);


protected:
	/// must be populated by the subclass
	std::map<std::string, unsigned int> lcNameIndex;

private:
	/// "ExampleArchive.sdd"
	const std::string archiveFile;
};

#endif // _ARCHIVE_BASE_H
