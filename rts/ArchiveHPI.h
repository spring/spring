#ifndef __ARCHIVE_HPI_H
#define __ARCHIVE_HPI_H

#include "ArchiveBase.h"
#include "ArchiveBuffered.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include "hpiutil2/hpiutil.h"

class CArchiveHPI :
	public CArchiveBuffered
{
protected:
	hpiutil::hpifile *hpi;
	int curSearchHandle;
	map<string, int> fileSizes;			// hpiutil doesn't provide a way to determine this for a given file
	map<int, map<string, int>::iterator> searchHandles;
	virtual ABOpenFile_t* GetEntireFile(const string& fileName);
public:
	CArchiveHPI(const string& name);
	virtual ~CArchiveHPI(void);
	virtual bool IsOpen();
	virtual int FindFiles(int cur, string* name, int* size);
};

#endif
