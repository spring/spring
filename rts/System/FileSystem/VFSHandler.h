#ifndef __VFS_HANDLER_H
#define __VFS_HANDLER_H

#include <map>
#include <string>
#include <vector>

class CArchiveBase;

class CVFSHandler
{
protected:
	struct FileData{
		CArchiveBase *ar;
		int size;
		bool dynamic;
	};
	std::map<std::string, FileData> files; 
	std::map<std::string, CArchiveBase*> archives;
public:
	CVFSHandler();
	virtual ~CVFSHandler();

	int LoadFile(std::string name, void* buffer);
	int GetFileSize(std::string name);

	std::vector<std::string> GetFilesInDir(std::string dir);
	std::vector<std::string> GetDirsInDir(std::string dir);

	bool AddArchive(std::string arName, bool override);
};

extern CVFSHandler* hpiHandler;

#endif
