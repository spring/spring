#ifndef __ARCHIVE_HPI_H
#define __ARCHIVE_HPI_H

#include "ArchiveBase.h"
#include "ArchiveBuffered.h"
#include "lib/hpiutil2/hpiutil.h"

class CArchiveHPI :
	public CArchiveBuffered
{
protected:
	hpiutil::hpifile *hpi;
	int curSearchHandle;
	// hpiutil doesn't provide a way to determine this for a given file
	std::map<std::string, int> fileSizes;
	std::map<int, std::map<std::string, int>::iterator> searchHandles;
	virtual ABOpenFile_t* GetEntireFile(const std::string& fileName);
public:
	CArchiveHPI(const std::string& name);
	virtual ~CArchiveHPI(void);
	virtual bool IsOpen();
	virtual int FindFiles(int cur, std::string* name, int* size);
};

#endif
