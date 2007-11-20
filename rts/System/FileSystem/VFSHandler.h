#ifndef __VFS_HANDLER_H
#define __VFS_HANDLER_H

#include <map>
#include <string>
#include <vector>

class CArchiveBase;

using namespace std;

class CVFSHandler
{
protected:
	struct FileData{
		CArchiveBase *ar;
		int size;
	};
	map<string, FileData> files; 
	map<string, CArchiveBase*> archives;
public:
	CVFSHandler();
	virtual ~CVFSHandler();

	int LoadFile(string name, void* buffer);
	int GetFileSize(string name);

	vector<string> GetFilesInDir(string dir);
	vector<string> GetDirsInDir(string dir);

	bool AddArchive(string arName, bool override);
};

extern CVFSHandler* hpiHandler;

#endif
