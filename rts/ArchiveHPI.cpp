#include "StdAfx.h"
#include "ArchiveHPI.h"
#include <algorithm>

using namespace hpiutil;

CArchiveHPI::CArchiveHPI(const string& name) :
	CArchiveBuffered(name),
	curSearchHandle(1)
{
	hpi = HPIOpen(name.c_str());
	if (hpi == NULL)
		return;

	std::vector<hpientry*> ret = HPIGetFiles(*hpi);
	for (std::vector<hpientry*>::iterator it = ret.begin(); it != ret.end(); it++) {
		if (!(*it)->directory) {
			string name = (*it)->path();
			transform(name.begin(), name.end(), name.begin(), (int (*)(int))tolower);
			fileSizes[name] = (*it)->size;
			printf("File %s size: %d\n",name.c_str(),fileSizes[name]);
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
	string name = fileName;
	transform(name.begin(), name.end(), name.begin(), (int (*)(int))tolower);

	hpientry* f = HPIOpenFile(*hpi, (const char*)name.c_str());
	if (f == NULL)
		return 0;

	printf("Loading %s\n",fileName.c_str());
	ABOpenFile_t* of = new ABOpenFile_t;
	of->pos = 0;
	of->size = f->size;
	of->data = (char*)malloc(of->size);

	HPIGet(of->data, *f, 0, of->size);
	HPICloseFile(*f);

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
