#ifndef __VFS_HANDLER_H
#define __VFS_HANDLER_H

#include <map>
#include <string>
#include <vector>

class CArchiveBase;

class CVFSHandler
{
protected:
	struct FileData {
		CArchiveBase *ar;
		int size;
		bool dynamic;
	};
	std::map<std::string, FileData> files; 
	std::map<std::string, CArchiveBase*> archives;
public:
	CVFSHandler();
	virtual ~CVFSHandler();

	int LoadFile(const std::string& name, void* buffer);
	int GetFileSize(const std::string& name);

	std::vector<std::string> GetFilesInDir(const std::string& dir);
	std::vector<std::string> GetDirsInDir(const std::string& dir);

	/**
	 * Override determines whether if conflicts overwrites
	 * an existing entry in the virtual filesystem or not
	 */
	bool AddArchive(const std::string& arName, bool override,
	                const std::string& type = "");
	/**
	 * Returns true if the archive is not loaded,
	 * so it may was not loaded in the first place or was unloaded
	 * successfully.
	 */
	bool RemoveArchive(const std::string& arName);
};

extern CVFSHandler* vfsHandler;

#endif // __VFS_HANDLER_H
