/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Archive7Zip.h"

#include <algorithm>
#include <boost/system/error_code.hpp>
#include <stdexcept>

extern "C" {
#include "lib/7z/Types.h"
#include "lib/7z/Archive/7z/7zAlloc.h"
#include "lib/7z/Archive/7z/7zExtract.h"
#include "lib/7z/7zCrc.h"
}

#include "Util.h"
#include "mmgr.h"
#include "LogOutput.h"

#define CHECK_FILE_ID(fileId) \
	assert((fileId >= 0) && (fileId < NumFiles()));


CArchive7Zip::CArchive7Zip(const std::string& name) :
	CArchiveBase(name),
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
	} else {
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

	// In 7zip talk, folders are pack-units (solid blocks),
	// not related to file-system folders.
	UInt64* folderUnpackSizes = new UInt64[db.db.NumFolders];
	for (int fi = 0; fi < db.db.NumFolders; fi++) {
		folderUnpackSizes[fi] = SzFolder_GetUnpackSize(db.db.Folders + fi);
	}

	// Get contents of archive and store name->int mapping
	for (unsigned i = 0; i < db.db.NumFiles; ++i) {
		CSzFileItem* f = db.db.Files + i;
		if ((f->Size >= 0) && !f->IsDir) {
			std::string fileName = f->Name;

			FileData fd;
			fd.origName = fileName;
			fd.fp = i;
			fd.size = f->Size;
			fd.crc = (f->Size > 0) ? f->FileCRC : 0;
			const UInt32 folderIndex = db.FileIndexToFolderIndexMap[i];
			if (folderIndex == ((UInt32)-1)) {
				// file has no folder assigned
				fd.unpackedSize = f->Size;
				fd.packedSize   = f->Size;
			} else {
				fd.unpackedSize = folderUnpackSizes[folderIndex];
				fd.packedSize   = db.db.PackSizes[folderIndex];
			}

			StringToLowerInPlace(fileName);
			fileData.push_back(fd);
			lcNameIndex[fileName] = fileData.size()-1;
		}
	}

	delete [] folderUnpackSizes;
}

CArchive7Zip::~CArchive7Zip()
{
	if (outBuffer) {
		IAlloc_Free(&allocImp, outBuffer);
	}
	if (isOpen) {
		File_Close(&archiveStream.file);
	}
	SzArEx_Free(&db, &allocImp);
}

bool CArchive7Zip::IsOpen()
{
	return isOpen;
}

unsigned CArchive7Zip::NumFiles() const
{
	return fileData.size();
}

bool CArchive7Zip::GetFile(unsigned fid, std::vector<boost::uint8_t>& buffer)
{
	boost::mutex::scoped_lock lck(archiveLock);
	CHECK_FILE_ID(fid);
	
	// Get 7zip to decompress it
	size_t offset;
	size_t outSizeProcessed;
	SRes res;

	res = SzAr_Extract(&db, &lookStream.s, fileData[fid].fp, &blockIndex, &outBuffer, &outBufferSize, &offset, &outSizeProcessed, &allocImp, &allocTempImp);
	if (res == SZ_OK) {
		std::copy((char*)outBuffer+offset, (char*)outBuffer+offset+outSizeProcessed, std::back_inserter(buffer));
		return true;
	} else {
		return false;
	}
}

void CArchive7Zip::FileInfo(unsigned fid, std::string& name, int& size) const
{
	CHECK_FILE_ID(fid);
	name = fileData[fid].origName;
	size = fileData[fid].size;
}


const size_t CArchive7Zip::COST_LIMIT_UNPACK_OVERSIZE = 32 * 1024;
const size_t CArchive7Zip::COST_LIMIT_DISC_READ       = 32 * 1024;

bool CArchive7Zip::HasLowReadingCost(unsigned fid) const
{
	CHECK_FILE_ID(fid);
	const FileData& fd = fileData[fid];
	// The cost is high, if the to-be-unpacked data is
	// more then 32KB larger then the file alone,
	// and the to-be-read-from-disc data is larger then 32KB.
	// This should work well for:
	// * small meta-files in small solid blocks
	// * big ones in separate solid blocks
	// * for non-solid archives anyway
	return (((fd.unpackedSize - fd.size) <= COST_LIMIT_UNPACK_OVERSIZE)
			|| (fd.packedSize <= COST_LIMIT_DISC_READ));
}

unsigned CArchive7Zip::GetCrc32(unsigned fid)
{
	CHECK_FILE_ID(fid);
	return fileData[fid].crc;
}
