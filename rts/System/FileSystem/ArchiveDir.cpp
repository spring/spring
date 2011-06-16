/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include "ArchiveDir.h"

#include <assert.h>
#include <fstream>

#include "FileSystem/FileSystem.h"
#include "Util.h"
#include "mmgr.h"

CArchiveDir::CArchiveDir(const std::string& archiveName)
	: CArchiveBase(archiveName)
	, dirName(archiveName + '/')
{
	const std::vector<std::string>& found = filesystem.FindFiles(dirName, "*", FileSystem::RECURSE);

	// because spring expects the contents of archives to be case independent,
	// we convert filenames to lowercase in every function, and keep a std::map
	// lcNameToOrigName to convert back from lowercase to original case.
	std::vector<std::string>::const_iterator fi;
	for (fi = found.begin(); fi != found.end(); ++fi) {
		// strip our own name off.. & convert to forward slashes
		std::string origName(*fi, dirName.length());
		filesystem.ForwardSlashes(origName);
		// convert to lowercase and store
		searchFiles.push_back(origName);
		lcNameIndex[StringToLower(origName)] = searchFiles.size() - 1;
	}
}

CArchiveDir::~CArchiveDir()
{
}

bool CArchiveDir::IsOpen()
{
	return true;
}

unsigned int CArchiveDir::NumFiles() const
{
	return searchFiles.size();
}

bool CArchiveDir::GetFile(unsigned int fid, std::vector<boost::uint8_t>& buffer)
{
	assert(fid >= 0 && fid < NumFiles());

	const std::string rawpath = filesystem.LocateFile(dirName + searchFiles[fid]);
	std::ifstream ifs(rawpath.c_str(), std::ios::in | std::ios::binary);
	if (!ifs.bad() && ifs.is_open()) {
		ifs.seekg(0, std::ios_base::end);
		buffer.resize(ifs.tellg());
		ifs.seekg(0, std::ios_base::beg);
		ifs.clear();
		ifs.read((char*)&buffer[0], buffer.size());
		return true;
	} else {
		return false;
	}
}

void CArchiveDir::FileInfo(unsigned int fid, std::string& name, int& size) const
{
	assert(fid >= 0 && fid < NumFiles());

	name = searchFiles[fid];
	const std::string rawPath = filesystem.LocateFile(dirName + name);
	std::ifstream ifs(rawPath.c_str(), std::ios::in | std::ios::binary);
	if (!ifs.bad() && ifs.is_open()) {
		ifs.seekg(0, std::ios_base::end);
		size = ifs.tellg();
	} else {
		size = 0;
	}
}
