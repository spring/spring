/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SevenZipArchive.h"

#include <algorithm>
#include <stdexcept>
#include <string.h> //memcpy

extern "C" {
#include "lib/7z/Types.h"
#include "lib/7z/7zAlloc.h"
#include "lib/7z/7zCrc.h"
}

#include "System/CRC.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"

static Byte kUtf8Limits[5] = { 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
static Bool Utf16_To_Utf8(char* dest, size_t* destLen, const UInt16* src, size_t srcLen)
{
	size_t destPos = 0;
	size_t srcPos = 0;

	for (;;) {
		unsigned numAdds;
		UInt32 value;

		if (srcPos == srcLen) {
			*destLen = destPos;
			return True;
		}

		if ((value = src[srcPos++]) < 0x80) {
			dest[destPos++] = (char)value;
			continue;
		}

		if (value >= 0xD800 && value < 0xE000) {
			if (value >= 0xDC00 || srcPos == srcLen)
				break;

			const UInt32 c2 = src[srcPos++];

			if (c2 < 0xDC00 || c2 >= 0xE000)
				break;

			value = (((value - 0xD800) << 10) | (c2 - 0xDC00)) + 0x10000;
		}
		for (numAdds = 1; numAdds < 5; numAdds++)
			if (value < (((UInt32)1) << (numAdds * 5 + 6)))
				break;

		dest[destPos++] = (char)(kUtf8Limits[numAdds - 1] + (value >> (6 * numAdds)));

		do {
			numAdds--;
			dest[destPos++] = (char)(0x80 + ((value >> (6 * numAdds)) & 0x3F));
		} while (numAdds != 0);
	}

	*destLen = destPos;
	return False;
}



int CSevenZipArchive::GetFileName(const CSzArEx* db, int i)
{
	// caller has archiveLock
	const size_t len = SzArEx_GetFileNameUtf16(db, i, nullptr);

	if (len >= sizeof(tempBuffer))
		return -1;

	tempBuffer[len - 1] = 0;
	return SzArEx_GetFileNameUtf16(db, i, tempBuffer);
}

IArchive* CSevenZipArchiveFactory::DoCreateArchive(const std::string& filePath) const
{
	return new CSevenZipArchive(filePath);
}


static inline const char* GetErrorStr(int err)
{
	switch (err) {
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

static inline const char* GetSystemErrorStr(WRes wres)
{
	static char buf[16384];

	memset(buf, 0, sizeof(buf));
	strncpy(buf, strerror(wres), sizeof(buf) - 1);

#ifdef USE_WINDOWS_FILE
	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
								 nullptr, wres, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &messageBuffer, 0, nullptr);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, messageBuffer, std::min(sizeof(buf), strlen(static_cast<char*>(messageBuffer))));

	LocalFree(messageBuffer);
#endif

	return buf;
}




CSevenZipArchive::CSevenZipArchive(const std::string& name): CBufferedArchive(name, false)
{
	std::lock_guard<spring::mutex> lck(archiveLock);

	allocImp.Alloc = SzAlloc;
	allocImp.Free = SzFree;
	allocTempImp.Alloc = SzAllocTemp;
	allocTempImp.Free = SzFreeTemp;

	SzArEx_Init(&db);

	const WRes wres = InFile_Open(&archiveStream.file, name.c_str());
	if (wres) {
		LOG_L(L_ERROR, "[%s] error opening \"%s\": %s (%i)", __func__, name.c_str(), GetSystemErrorStr(wres), (int) wres);
		return;
	}

	FileInStream_CreateVTable(&archiveStream);
	LookToRead_CreateVTable(&lookStream, False);

	lookStream.realStream = &archiveStream.s;
	LookToRead_Init(&lookStream);

	CRC::InitTable();

	const SRes res = SzArEx_Open(&db, &lookStream.s, &allocImp, &allocTempImp);
	if (!(isOpen = (res == SZ_OK))) {
		LOG_L(L_ERROR, "[%s] error opening \"%s\": %s", __func__, name.c_str(), GetErrorStr(res));
		return;
	}

	// in 7zip, "folders" are pack-units (solid blocks)
	// which have no relation to file-system folders
	std::array<UInt64, 16384> folderUnpackSizesArr;
	std::vector<UInt64> folderUnpackSizesVec;

	UInt64* folderUnpackSizesPtr = nullptr;

	if (db.db.NumFolders > folderUnpackSizesArr.size()) {
		folderUnpackSizesVec.resize(db.db.NumFolders);
		folderUnpackSizesPtr = folderUnpackSizesVec.data();
	} else {
		folderUnpackSizesPtr = folderUnpackSizesArr.data();
	}

	for (unsigned int fi = 0; fi < db.db.NumFolders; fi++) {
		folderUnpackSizesPtr[fi] = SzFolder_GetUnpackSize(db.db.Folders + fi);
	}


	fileEntries.reserve(db.db.NumFiles);

	// Get contents of archive and store name->int mapping
	for (unsigned int i = 0; i < db.db.NumFiles; ++i) {
		const CSzFileItem* f = db.db.Files + i;
		if (f->IsDir)
			continue;

		const int written = GetFileName(&db, i);
		if (written <= 0) {
			LOG_L(L_ERROR, "[%s] error getting filename in Archive: %s (%d), file skipped in %s", __func__, GetErrorStr(res), res, name.c_str());
			continue;
		}

		char buf[1024];
		size_t dstlen = sizeof(buf);

		Utf16_To_Utf8(buf, &dstlen, tempBuffer, written);

		FileEntry fd;

		fd.origName = buf;
		fd.fp = i;
		fd.size = f->Size;
		fd.crc = (f->Size > 0) ? f->Crc: 0;

		const UInt32 folderIndex = db.FileIndexToFolderIndexMap[i];

		if (folderIndex == ((UInt32)-1)) {
			// file has no folder assigned
			fd.unpackedSize = f->Size;
			fd.packedSize   = f->Size;
		} else {
			fd.unpackedSize = folderUnpackSizesPtr[folderIndex];
			fd.packedSize   = db.db.PackSizes[folderIndex];
		}

		lcNameIndex.emplace(StringToLower(fd.origName), fileEntries.size());
		fileEntries.emplace_back(std::move(fd));
	}
}


CSevenZipArchive::~CSevenZipArchive()
{
	std::lock_guard<spring::mutex> lck(archiveLock);

	if (outBuffer != nullptr)
		IAlloc_Free(&allocImp, outBuffer);

	if (isOpen)
		File_Close(&archiveStream.file);

	SzArEx_Free(&db, &allocImp);
}


int CSevenZipArchive::GetFileImpl(unsigned int fid, std::vector<std::uint8_t>& buffer)
{
	// assert(archiveLock.locked());
	assert(IsFileId(fid));

	size_t offset = 0;
	size_t outSizeProcessed = 0;

	if (SzArEx_Extract(&db, &lookStream.s, fileEntries[fid].fp, &blockIndex, &outBuffer, &outBufferSize, &offset, &outSizeProcessed, &allocImp, &allocTempImp) != SZ_OK)
		return 0;

	buffer.resize(outSizeProcessed);
	memcpy(buffer.data(), reinterpret_cast<char*>(outBuffer) + offset, outSizeProcessed);
	return 1;
}

void CSevenZipArchive::FileInfo(unsigned int fid, std::string& name, int& size) const
{
	assert(IsFileId(fid));
	name = fileEntries[fid].origName;
	size = fileEntries[fid].size;
}


bool CSevenZipArchive::HasLowReadingCost(unsigned int fid) const
{
	assert(IsFileId(fid));
	const FileEntry& fd = fileEntries[fid];
	// consider the cost of reading a file "high" if
	//   1) the to-be-unpacked data is >= 32KB larger than it
	//   2) the to-be-read-from-disk data is larger than 32KB
	// this should work well for
	//   1) small meta-files in small solid blocks
	//   2) big files in separate solid blocks
	//   3) non-solid archives
	return (((fd.unpackedSize - fd.size) <= COST_LIMIT_UNPACK_OVERSIZE) || (fd.packedSize <= COST_LIMIT_DISK_READ));
}

