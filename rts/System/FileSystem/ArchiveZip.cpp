/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include "ArchiveZip.h"

#include <algorithm>
#include <stdexcept>

#include "Util.h"
#include "mmgr.h"
#include "LogOutput.h"


CArchiveZip::CArchiveZip(const std::string& name) : CArchiveBuffered(name)
{
#ifdef USEWIN32IOAPI
	zlib_filefunc_def ffunc;
	fill_win32_filefunc(&ffunc);
	zip = unzOpen2(name.c_str(),&ffunc);
#else
	zip = unzOpen(name.c_str());
#endif
	if (!zip)
	{
		LogObject() << "Error opening " << name;
		return;
	}

	// We need to map file positions to speed up opening later
	for (int ret = unzGoToFirstFile(zip); ret == UNZ_OK; ret = unzGoToNextFile(zip))
	{
		unz_file_info info;
		char fname[512];

		unzGetCurrentFileInfo(zip, &info, fname, 512, NULL, 0, NULL, 0);

		const std::string name = StringToLower(fname);
		if (name.empty()) {
			continue;
		}
		const char last = name[name.length() - 1];
		if ((last == '/') || (last == '\\'))
		{
			continue; // exclude directory names
		}

		FileData fd;
		unzGetFilePos(zip, &fd.fp);
		fd.size = info.uncompressed_size;
		fd.origName = fname;
		fd.crc = info.crc;
		fileData.push_back(fd);
		lcNameIndex[name] = fileData.size()-1;
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

unsigned CArchiveZip::NumFiles() const
{
	return fileData.size();
}

void CArchiveZip::FileInfo(unsigned fid, std::string& name, int& size) const
{
	assert(fid >= 0 && fid < NumFiles());

	name = fileData[fid].origName;
	size = fileData[fid].size;
}

unsigned CArchiveZip::GetCrc32(unsigned fid)
{
	assert(fid >= 0 && fid < NumFiles());

	return fileData[fid].crc;
}

// To simplify things, files are always read completely into memory from the zipfile, since zlib does not
// provide any way of reading more than one file at a time
bool CArchiveZip::GetFileImpl(unsigned fid, std::vector<boost::uint8_t>& buffer)
{
	// Don't allow opening files on missing/invalid archives
	if (!zip)
		return false;
	assert(fid >= 0 && fid < NumFiles());

	unzGoToFilePos(zip, &fileData[fid].fp);

	unz_file_info fi;
	unzGetCurrentFileInfo(zip, &fi, NULL, 0, NULL, 0, NULL, 0);

	if (unzOpenCurrentFile(zip) != UNZ_OK)
		return false;

	buffer.resize(fi.uncompressed_size);
	if (unzReadCurrentFile(zip, &buffer[0], buffer.size()) < 0 || unzCloseCurrentFile(zip) == UNZ_CRCERROR)
	{
		buffer.clear();
		return false;
	}
	return true;
}
