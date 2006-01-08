#include "StdAfx.h"
#include "ArchiveZip.h"
#include <algorithm>
#include "mmgr.h"

//#pragma comment(lib, "../taspring/zlib/lib/zdll.lib")
//#pragma comment(lib, "../crashrpt/zlib/lib/zlib.lib")

// IMO, this is not flexible, it should just be specified in the building system options
//#pragma comment(lib, "../zlib/lib/zlibwapi.lib")

CArchiveZip::CArchiveZip(const string& name) :
	CArchiveBuffered(name),
	curSearchHandle(1)
{
#ifdef USEWIN32IOAPI
	zlib_filefunc_def ffunc;
	fill_win32_filefunc(&ffunc);
	zip = unzOpen2(name.c_str(),&ffunc);
#else
	zip = unzOpen(name.c_str());
#endif
	if (!zip)
		return;

	// We need to map file positions to speed up opening later
	for (int ret = unzGoToFirstFile(zip); ret == UNZ_OK; ret = unzGoToNextFile(zip)) {
		unz_file_info info;
		char fname[512];
		string name;

		unzGetCurrentFileInfo(zip, &info, fname, 512, NULL, 0, NULL, 0);

		if (info.uncompressed_size > 0) {
			name = fname;
			transform(name.begin(), name.end(), name.begin(), (int (*)(int))tolower);
//			SetSlashesForwardToBack(name);

			FileData fd;
			unzGetFilePos(zip, &fd.fp);
			fd.size = info.uncompressed_size;
			fd.origName = fname;
//			SetSlashesForwardToBack(fd.origName);

			fileData[name] = fd;
		}
	}
}

CArchiveZip::~CArchiveZip(void)
{
	if (zip)
		unzClose(zip);
}

bool CArchiveZip::IsOpen()
{
	return (zip != NULL);
}

class zip_exception : public exception {};

// To simplify things, files are always read completely into memory from the zipfile, since zlib does not
// provide any way of reading more than one file at a time
ABOpenFile_t* CArchiveZip::GetEntireFile(const string& fName)
{
	// Don't allow opening files on missing/invalid archives
	if (!zip)
		return NULL;

	string fileName = fName;
	transform(fileName.begin(), fileName.end(), fileName.begin(), (int (*)(int))tolower);

	//if (unzLocateFile(zip, fileName.c_str(), 2) != UNZ_OK) 
	//	return 0;

	if (fileData.find(fileName) == fileData.end())
		return NULL;

	FileData fd = fileData[fileName];
	unzGoToFilePos(zip, &fileData[fileName].fp);

	unz_file_info fi;
	unzGetCurrentFileInfo(zip, &fi, NULL, 0, NULL, 0, NULL, 0);

	ABOpenFile_t* of = new ABOpenFile_t;
	of->pos = 0;
	of->size = fi.uncompressed_size;
	of->data = (char*)malloc(of->size); 

	// If anything fails, we abort
	try {
		if (unzOpenCurrentFile(zip) != UNZ_OK)
			throw zip_exception();
		if (unzReadCurrentFile(zip, of->data, of->size) < 0) 
			throw zip_exception();
		if (unzCloseCurrentFile(zip) == UNZ_CRCERROR)
			throw zip_exception(); 
	}
	catch (zip_exception) {
		free(of->data);
		delete of;
		return NULL;
	}

	return of;
}

int CArchiveZip::FindFiles(int cur, string* name, int* size)
{
	if (cur == 0) {
		curSearchHandle++;
		cur = curSearchHandle;
		searchHandles[cur] = fileData.begin();
	}

	if (searchHandles[cur] == fileData.end()) {
		searchHandles.erase(cur);
		return 0;
	}

	*name = searchHandles[cur]->second.origName;
	*size = searchHandles[cur]->second.size;

	searchHandles[cur]++;
	return cur;
}

void CArchiveZip::SetSlashesForwardToBack(string& name)
{
	for (unsigned int i = 0; i < name.length(); ++i) {
		if (name[i] == '/')
			name[i] = '\\';
	}
}

void CArchiveZip::SetSlashesBackToForward(string& name)
{
	for (unsigned int i = 0; i < name.length(); ++i) {
		if (name[i] == '\\')
			name[i] = '/';
	}
}
