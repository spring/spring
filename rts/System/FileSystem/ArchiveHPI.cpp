#include "StdAfx.h"
#include "ArchiveHPI.h"
#include <algorithm>
#include "mmgr.h"

using namespace hpiutil;

CArchiveHPI::CArchiveHPI(const string& name) :
	CArchiveBuffered(name),
	curSearchHandle(1)
{
	hpi = HPIOpen(name.c_str());
	if (hpi == NULL)
		return;

	std::vector<hpientry_ptr> ret = HPIGetFiles(*hpi);
	for (std::vector<hpientry_ptr>::iterator it = ret.begin(); it != ret.end(); it++) {
		if (!(*it)->directory) {
			string name = StringToLower((*it)->path());
			fileSizes[name] = (*it)->size;
		}
	}
}

CArchiveHPI::~CArchiveHPI(void)
{
	if (hpi)
		HPIClose(*hpi);
}

bool CArchiveHPI::IsOpen()
{
	return (hpi != NULL);
}

ABOpenFile_t* CArchiveHPI::GetEntireFile(const string& fileName)
{
	string name = StringToLower(fileName);

	hpientry_ptr f = HPIOpenFile(*hpi, (const char*)name.c_str());
	if (!f.get())
		return 0;

	ABOpenFile_t* of = SAFE_NEW ABOpenFile_t;
	of->pos = 0;
	of->size = f->size;
	of->data = (char*)malloc(of->size);

	if (HPIGet(of->data, f, 0, of->size) != of->size) {
		free(of->data);
		delete of;
		return 0;
	}

	HPICloseFile(f);

	return of;
}

int CArchiveHPI::FindFiles(int cur, string* name, int* size)
{
	if (cur == 0) {
		curSearchHandle++;
		cur = curSearchHandle;
		searchHandles[cur] = fileSizes.begin();
	}

	if (searchHandles[cur] == fileSizes.end()) {
		searchHandles.erase(cur);
		return 0;
	}

	*name = searchHandles[cur]->first;
	*size = searchHandles[cur]->second;

	searchHandles[cur]++;
	return cur;
}
