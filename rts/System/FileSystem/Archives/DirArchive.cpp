/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "DirArchive.h"

#include <assert.h>
#include <fstream>

#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/StringUtil.h"


CDirArchiveFactory::CDirArchiveFactory()
	: IArchiveFactory("sdd")
{
}

IArchive* CDirArchiveFactory::DoCreateArchive(const std::string& filePath) const
{
	return new CDirArchive(filePath);
}


CDirArchive::CDirArchive(const std::string& archiveName)
	: IArchive(archiveName)
	, dirName(archiveName + '/')
{
	const std::vector<std::string>& found = dataDirsAccess.FindFiles(dirName, "*", FileQueryFlags::RECURSE);

	for (const std::string& f: found) {
		// strip our own name off.. & convert to forward slashes
		std::string origName(f, dirName.length());

		FileSystem::ForwardSlashes(origName);
		// convert to lowercase and store
		searchFiles.push_back(origName);

		lcNameIndex[StringToLower(origName)] = searchFiles.size() - 1;
	}
}


bool CDirArchive::GetFile(unsigned int fid, std::vector<std::uint8_t>& buffer)
{
	assert(IsFileId(fid));

	const std::string rawpath = dataDirsAccess.LocateFile(dirName + searchFiles[fid]);
	std::ifstream ifs(rawpath.c_str(), std::ios::in | std::ios::binary);

	if (ifs.bad() || !ifs.is_open())
		return false;

	ifs.seekg(0, std::ios_base::end);
	buffer.resize(ifs.tellg());
	ifs.seekg(0, std::ios_base::beg);
	ifs.clear();

	if (!buffer.empty())
		ifs.read((char*)&buffer[0], buffer.size());

	return true;
}

void CDirArchive::FileInfo(unsigned int fid, std::string& name, int& size) const
{
	assert(IsFileId(fid));

	name = searchFiles[fid];
	const std::string rawPath = dataDirsAccess.LocateFile(dirName + name);
	std::ifstream ifs(rawPath.c_str(), std::ios::in | std::ios::binary);

	if (!ifs.bad() && ifs.is_open()) {
		ifs.seekg(0, std::ios_base::end);
		size = ifs.tellg();
	} else {
		size = 0;
	}
}
