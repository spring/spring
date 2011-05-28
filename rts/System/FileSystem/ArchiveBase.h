/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __ARCHIVE_BASE_H
#define __ARCHIVE_BASE_H

// A general class for handling of file archives (such as hpi and zip files)

#include <string>
#include <vector>
#include <map>
#include <boost/cstdint.hpp>

/**
@brief Abstraction of different archive types

Loosely resembles STL container:
for (unsigned fid = 0; fid != NumFiles(); ++fid)
{
	//stuff
}
*/
class CArchiveBase
{
public:
	CArchiveBase(const std::string& archiveName);
	virtual ~CArchiveBase();

	virtual bool IsOpen() = 0;
	std::string GetArchiveName() const;
	
	///@return The amount of files in the archive, does not change during lifetime
	virtual unsigned NumFiles() const = 0;
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
	unsigned FindFile(const std::string& filePath) const;
	virtual bool GetFile(unsigned fid, std::vector<boost::uint8_t>& buffer) = 0;
	virtual void FileInfo(unsigned fid, std::string& name, int& size) const = 0;
	/**
	 * Returns true if the cost of reading the file is in qualitatively relative
	 * to its file-size.
	 * This is mainly usefull in the case of solid archives,
	 * which may make the reading of a single small file over proportionally
	 * expensive.
	 * The returned value is usually relative to certain arbitrary chosen
	 * constants.
	 * Most implementations may always return true.
	 * @return true if cost is is ~ relative to its file-size
	 */
	virtual bool HasLowReadingCost(unsigned fid) const;
	virtual unsigned GetCrc32(unsigned fid);

	/// for convenience
	bool GetFile(const std::string& name, std::vector<boost::uint8_t>& buffer);

protected:
	std::map<std::string, unsigned> lcNameIndex; ///< must be populated by the subclass

private:
	const std::string archiveFile; ///< "ExampleArchive.sdd"
};

#endif
