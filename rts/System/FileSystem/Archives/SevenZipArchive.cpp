/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SevenZipArchive.h"

#include <algorithm>
#include <boost/system/error_code.hpp>
#include <stdexcept>
#include <string.h> //memcpy

extern "C" {
#include "lib/7z/Types.h"
#include "lib/7z/7zAlloc.h"
#include "lib/7z/7zCrc.h"
}

#include "System/Util.h"
#include "System/Log/ILog.h"

static Byte kUtf8Limits[5] = { 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
static Bool Utf16_To_Utf8(char *dest, size_t *destLen, const UInt16 *src, size_t srcLen)
{
	size_t destPos = 0, srcPos = 0;
	for (;;) {
		unsigned numAdds;
		UInt32 value;
		if (srcPos == srcLen) {
			*destLen = destPos;
			return True;
		}
		value = src[srcPos++];
		if (value < 0x80) {
			if (dest)
				dest[destPos] = (char)value;
			destPos++;
			continue;
		}
		if (value >= 0xD800 && value < 0xE000) {
			UInt32 c2;
			if (value >= 0xDC00 || srcPos == srcLen)
				break;
			c2 = src[srcPos++];
			if (c2 < 0xDC00 || c2 >= 0xE000)
				break;
			value = (((value - 0xD800) << 10) | (c2 - 0xDC00)) + 0x10000;
		}
		for (numAdds = 1; numAdds < 5; numAdds++)
			if (value < (((UInt32)1) << (numAdds * 5 + 6)))
				break;
		if (dest)
			dest[destPos] = (char)(kUtf8Limits[numAdds - 1] + (value >> (6 * numAdds)));
		destPos++;
		do {
			numAdds--;
			if (dest)
				dest[destPos] = (char)(0x80 + ((value >> (6 * numAdds)) & 0x3F));
			destPos++;
		} while (numAdds != 0);
	}
	*destLen = destPos;
	return False;
}

CSevenZipArchiveFactory::CSevenZipArchiveFactory()
	: IArchiveFactory("sd7")
{
}

int CSevenZipArchive::GetFileName(const CSzArEx* db, int i)
{
	size_t len = SzArEx_GetFileNameUtf16(db, i, NULL);

	if (len > tempBufSize) {
		SzFree(NULL, tempBuf);
		tempBufSize = len;
		tempBuf = (UInt16 *)SzAlloc(NULL, tempBufSize * sizeof(tempBuf[0]));
		if (tempBuf == 0) {
			return SZ_ERROR_MEM;
		}
	}
	tempBuf[len-1] = 0;
	return SzArEx_GetFileNameUtf16(db, i, tempBuf);
}

IArchive* CSevenZipArchiveFactory::DoCreateArchive(const std::string& filePath) const
{
	return new CSevenZipArchive(filePath);
}

const char* CSevenZipArchive::GetErrorStr(int err)
{
	switch(err) {
	case SZ_OK:
		return "OK";
	case SZ_ERROR_FAIL:
		return "Extracting failed";
	case SZ_ERROR_CRC:
		return "CRC error (archive corrupted?)";
	case SZ_ERROR_INPUT_EOF:
		return "Unexpected end of file (truncated?)";
	case SZ_ERROR_MEM:
		return "Out of memory";
	case SZ_ERROR_UNSUPPORTED:
		return "Unsupported archive";
	case SZ_ERROR_NO_ARCHIVE:
		return "Archive not found";
	}
	return "Unknown error";
}


CSevenZipArchive::CSevenZipArchive(const std::string& name):
	CBufferedArchive(name, false),
	blockIndex(0xFFFFFFFF),
	outBuffer(NULL),
	outBufferSize(0),
	tempBuf(NULL),
	tempBufSize(0),
	isOpen(false)
{
	allocImp.Alloc = SzAlloc;
	allocImp.Free = SzFree;
	allocTempImp.Alloc = SzAllocTemp;
	allocTempImp.Free = SzFreeTemp;

	SzArEx_Init(&db);

	WRes wres = InFile_Open(&archiveStream.file, name.c_str());
	if (wres) {
		boost::system::error_code e(wres, boost::system::get_system_category());
		LOG_L(L_ERROR, "Error opening \"%s\": %s (%i)",
				name.c_str(), e.message().c_str(), e.value());
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
		LOG_L(L_ERROR, "Error opening \"%s\": %s", name.c_str(), GetErrorStr(res));
		return;
	}

	// In 7zip talk, folders are pack-units (solid blocks),
	// not related to file-system folders.
	UInt64* folderUnpackSizes = new UInt64[db.db.NumFolders];
	for (unsigned int fi = 0; fi < db.db.NumFolders; fi++) {
		folderUnpackSizes[fi] = SzFolder_GetUnpackSize(db.db.Folders + fi);
	}

	// Get contents of archive and store name->int mapping
	for (unsigned int i = 0; i < db.db.NumFiles; ++i) {
		CSzFileItem* f = db.db.Files + i;
		if (!f->IsDir) {
			int written = GetFileName(&db, i);
			if (written<=0) {
				LOG_L(L_ERROR, "Error getting filename in Archive: %s %d, file skipped in %s", GetErrorStr(res), res, name.c_str());
				continue;
			}

			char buf[1024];
			size_t dstlen = sizeof(buf);

			Utf16_To_Utf8(buf, &dstlen, tempBuf, written);

			FileData fd;

			fd.origName=buf;
			fd.fp = i;
			fd.size = f->Size;
			fd.crc = (f->Size > 0) ? f->Crc: 0;

			const UInt32 folderIndex = db.FileIndexToFolderIndexMap[i];
			if (folderIndex == ((UInt32)-1)) {
				// file has no folder assigned
				fd.unpackedSize = f->Size;
				fd.packedSize   = f->Size;
			} else {
				fd.unpackedSize = folderUnpackSizes[folderIndex];
				fd.packedSize   = db.db.PackSizes[folderIndex];
			}
			std::string fileName = fd.origName;
			StringToLowerInPlace(fileName);
			fileData.push_back(fd);
			lcNameIndex[fileName] = fileData.size()-1;
		}
	}

	delete [] folderUnpackSizes;
}


CSevenZipArchive::~CSevenZipArchive()
{
	if (outBuffer) {
		IAlloc_Free(&allocImp, outBuffer);
	}
	if (isOpen) {
		File_Close(&archiveStream.file);
	}
	SzArEx_Free(&db, &allocImp);
	SzFree(NULL, tempBuf);
	tempBuf = NULL;
	tempBufSize = 0;
}

bool CSevenZipArchive::IsOpen()
{
	return isOpen;
}

unsigned int CSevenZipArchive::NumFiles() const
{
	return fileData.size();
}

bool CSevenZipArchive::GetFileImpl(unsigned int fid, std::vector<boost::uint8_t>& buffer)
{
	assert(IsFileId(fid));
	
	// Get 7zip to decompress it
	size_t offset;
	size_t outSizeProcessed;
	SRes res;

	res = SzArEx_Extract(&db, &lookStream.s, fileData[fid].fp, &blockIndex, &outBuffer, &outBufferSize, &offset, &outSizeProcessed, &allocImp, &allocTempImp);
	if (res == SZ_OK) {
		buffer.resize(outSizeProcessed);
		memcpy(&buffer[0], (char*)outBuffer+offset, outSizeProcessed);
		return true;
	} else {
		return false;
	}
}

void CSevenZipArchive::FileInfo(unsigned int fid, std::string& name, int& size) const
{
	assert(IsFileId(fid));
	name = fileData[fid].origName;
	size = fileData[fid].size;
}


const size_t CSevenZipArchive::COST_LIMIT_UNPACK_OVERSIZE = 32 * 1024;
const size_t CSevenZipArchive::COST_LIMIT_DISC_READ       = 32 * 1024;

bool CSevenZipArchive::HasLowReadingCost(unsigned int fid) const
{
	assert(IsFileId(fid));
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

unsigned int CSevenZipArchive::GetCrc32(unsigned int fid)
{
	assert(IsFileId(fid));
	return fileData[fid].crc;
}
