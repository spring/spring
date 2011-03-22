/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __VFS_HANDLER_H
#define __VFS_HANDLER_H

#include <map>
#include <string>
#include <vector>
#include <boost/cstdint.hpp>

class CArchiveBase;

class CVFSHandler
{
public:
	CVFSHandler();
	~CVFSHandler();

	bool LoadFile(const std::string& name, std::vector<boost::uint8_t>& buffer);

	std::vector<std::string> GetFilesInDir(const std::string& dir);
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
	 * @return true if the archive is not loaded anymore; it may was not loaded
	 *   in the first place, or was unloaded successfully.
	 */
	bool RemoveArchive(const std::string& archiveName);

protected:
	struct FileData {
		CArchiveBase *ar;
		int size;
		bool dynamic;
	};
	std::map<std::string, FileData> files; 
	std::map<std::string, CArchiveBase*> archives;
};

extern CVFSHandler* vfsHandler;

#endif // __VFS_HANDLER_H
