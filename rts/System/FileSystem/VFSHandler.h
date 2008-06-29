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

	bool AddArchive(const std::string& arName, bool override,
	                const std::string& type = "");
};

extern CVFSHandler* vfsHandler;

#endif
