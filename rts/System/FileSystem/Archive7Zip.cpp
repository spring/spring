#include "Archive7Zip.h"

#include <algorithm>
#include <boost/system/error_code.hpp>
#include <stdexcept>
#include <string.h>

extern "C" {
#include "lib/7z/Types.h"
#include "lib/7z/Archive/7z/7zAlloc.h"
#include "lib/7z/Archive/7z/7zExtract.h"
#include "lib/7z/7zCrc.h"
}

#include "Util.h"
#include "mmgr.h"
#include "LogOutput.h"

CArchive7Zip::CArchive7Zip(const std::string& name) :
	CArchiveBuffered(name),
	curSearchHandle(1),
	isOpen(false)
{
	blockIndex = 0xFFFFFFFF;
	outBuffer = NULL;
	outBufferSize = 0;

	allocImp.Alloc = SzAlloc;
	allocImp.Free = SzFree;

	allocTempImp.Alloc = SzAllocTemp;
	allocTempImp.Free = SzFreeTemp;

	SzArEx_Init(&db);

	WRes wres = InFile_Open(&archiveStream.file, name.c_str());
	if (wres) {
		boost::system::error_code e(wres, boost::system::get_system_category());
		LogObject() << "Error opening " << name << ": " << e.message() << " (" << e.value() << ")";
		return;
	}

	FileInStream_CreateVTable(&archiveStream);
	LookToRead_CreateVTable(&lookStream, False);

	lookStream.realStream = &archiveStream.s;
	LookToRead_Init(&lookStream);

	CrcGenerateTable();

	SRes res = SzArEx_Open(&db, &lookStream.s, &allocImp, &allocTempImp);
	if (res == SZ_OK) {
		isOpen = true;
	}
	else {
		isOpen = false;
		std::string error;
		switch (res) {
			case SZ_ERROR_FAIL:
				error = "Extracting failed";
				break;
			case SZ_ERROR_CRC:
				error = "CRC error (archive corrupted?)";
				break;
			case SZ_ERROR_INPUT_EOF:
				error = "Unexpected end of file (truncated?)";
				break;
			case SZ_ERROR_MEM:
				error = "Out of memory";
				break;
			case SZ_ERROR_UNSUPPORTED:
				error = "Unsupported archive";
				break;
			case SZ_ERROR_NO_ARCHIVE:
				error = "Archive not found";
				break;
			default:
				error = "Unknown error";
				break;
		}
		LogObject() << "Error opening " << name << ": " << error;
		return;
	}

	// Get contents of archive and store name->int mapping
	for (unsigned i = 0; i < db.db.NumFiles; ++i) {
		CSzFileItem *f = db.db.Files + i;
		if ((f->Size >= 0) && !f->IsDir) {
			std::string name = f->Name;

			FileData fd;
			fd.origName = name;
			fd.fp = i;
			fd.size = f->Size;
			fd.crc = (f->Size > 0) ? f->FileCRC : 0;

			StringToLowerInPlace(name);
			fileData[name] = fd;
		}
	}
}

CArchive7Zip::~CArchive7Zip(void)
{
	if (outBuffer) {
		IAlloc_Free(&allocImp, outBuffer);
	}
	if (isOpen) {
		File_Close(&archiveStream.file);
	}
	SzArEx_Free(&db, &allocImp);
}

unsigned int CArchive7Zip::GetCrc32 (const std::string& fileName)
{
	std::string lower = StringToLower(fileName);
	FileData fd = fileData[lower];
	return fd.crc;
}

ABOpenFile_t* CArchive7Zip::GetEntireFile(const std::string& fName)
{
	if (!isOpen)
		return NULL;

	// Figure out the file index
	std::string fileName = StringToLower(fName);

	if (fileData.find(fileName) == fileData.end())
		return NULL;

	FileData fd = fileData[fileName];

	// Get 7zip to decompress it
	size_t offset;
	size_t outSizeProcessed;

	SRes res;

	res = SzAr_Extract(&db, &lookStream.s, fd.fp, &blockIndex, &outBuffer, &outBufferSize, &offset, &outSizeProcessed, &allocImp, &allocTempImp);

	ABOpenFile_t* of = NULL;
	if (res == SZ_OK) {
		of = new ABOpenFile_t;
		of->pos = 0;
		of->size = outSizeProcessed;
		of->data = (char*)malloc(of->size);

		memcpy(of->data, outBuffer + offset, outSizeProcessed);
	}

	if (res != SZ_OK)
		return NULL;

	return of;
}

int CArchive7Zip::FindFiles(int cur, std::string* name, int* size)
{
	if (cur == 0) {
		curSearchHandle++;
		cur = curSearchHandle;
		searchHandles[cur] = fileData.begin();
	}

	if (searchHandles.find(cur) == searchHandles.end())
		throw std::runtime_error("Unregistered handle. Pass a handle returned by CArchive7Zip::FindFiles.");

	if (searchHandles[cur] == fileData.end()) {
		searchHandles.erase(cur);
		return 0;
	}

	*name = searchHandles[cur]->second.origName;
	*size = searchHandles[cur]->second.size;

	searchHandles[cur]++;
	return cur;
}

bool CArchive7Zip::IsOpen()
{
	return isOpen;
}
