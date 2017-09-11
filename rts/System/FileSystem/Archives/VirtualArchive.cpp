/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "VirtualArchive.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/Log/ILog.h"

#include "minizip/zip.h"
#include <cassert>

CVirtualArchiveFactory* virtualArchiveFactory;

///////////////////////////////
// CVirtualArchiveFactory start
///////////////////////////////
CVirtualArchiveFactory::CVirtualArchiveFactory() : IArchiveFactory("sva")
{
	virtualArchiveFactory = this;
}

void CVirtualArchiveFactory::Clear()
{
	for (auto& kv : archives) {
		delete kv.second;
	}
	archives.clear();
}

CVirtualArchiveFactory::~CVirtualArchiveFactory()
{
	Clear();
	virtualArchiveFactory = nullptr;
}

CVirtualArchive* CVirtualArchiveFactory::AddArchive(const std::string& fileName)
{
	if (archives.find(fileName) != archives.end()) {
		LOG("AddArchive: Archive with name %s already exists.", fileName.c_str());
		return nullptr;
	}
	CVirtualArchive* archive = new CVirtualArchive(fileName);
	archives[fileName] = archive;
	return archive;
}

CVirtualArchive* CVirtualArchiveFactory::GetArchive(const std::string& fileName) const
{
	auto it = archives.find(fileName);
	if (it != archives.end()) {
		return it->second;
	}

	return nullptr;
}

bool CVirtualArchiveFactory::RemoveArchive(const std::string& fileName)
{
	auto it = archives.find(fileName);
	if (it == archives.end()) {
		LOG("RemoveArchive: No such archive %s.", fileName.c_str());
		return false;
	}

	CVirtualArchive* archive = it->second;
	delete archive;
	archives.erase(it);

	return true;
}

const std::map<std::string, CVirtualArchive*>& CVirtualArchiveFactory::GetAllArchives() const
{
	return archives;
}

IArchive* CVirtualArchiveFactory::DoCreateArchive(const std::string& fileName) const
{
	const std::string baseName = FileSystem::GetBasename(fileName);

	CVirtualArchive* archive = GetArchive(baseName);

	if (archive != nullptr) {
		return archive->Open();
	}

	return nullptr;
}
///////////////////////////////
// CVirtualArchiveFactory end
///////////////////////////////

////////////////////////////
// CVirtualArchiveOpen start
////////////////////////////
CVirtualArchiveOpen::CVirtualArchiveOpen(CVirtualArchive* archive, const std::string& fileName) : IArchive(fileName), archive(archive)
{
	//Set subclass name index to archive's index (doesn't update while archive is open)
	lcNameIndex = archive->GetNameIndex();
}

CVirtualArchiveOpen::~CVirtualArchiveOpen()
{
}

bool CVirtualArchiveOpen::IsOpen()
{
	//Virtual archives are stored in memory and as such always open
	return true;
}

unsigned int CVirtualArchiveOpen::NumFiles() const
{
	return archive->NumFiles();
}

bool CVirtualArchiveOpen::GetFile(unsigned int fid, std::vector<std::uint8_t>& buffer)
{
	return archive->GetFile(fid, buffer);
}

void CVirtualArchiveOpen::FileInfo(unsigned int fid, std::string& name, int& size) const
{
	return archive->FileInfo(fid, name, size);
}
////////////////////////////
// CVirtualArchiveOpen end
////////////////////////////

////////////////////////
// CVirtualArchive start
////////////////////////
CVirtualArchive::CVirtualArchive(const std::string& fileName) : fileName(fileName)
{
}

CVirtualArchive::~CVirtualArchive()
{
	for (auto& kv : files) {
		delete kv;
	}
	files.clear();
}

CVirtualArchiveOpen* CVirtualArchive::Open()
{
	return new CVirtualArchiveOpen(this, fileName);
}

unsigned int CVirtualArchive::NumFiles() const
{
	return files.size();
}

bool CVirtualArchive::GetFile(unsigned int fid, std::vector<std::uint8_t>& buffer)
{
	if (fid >= files.size()) {
		return false;
	}

	CVirtualFile* file = files[fid];
	buffer = file->buffer;

	return true;
}

void CVirtualArchive::FileInfo(unsigned int fid, std::string& name, int& size) const
{
	assert(fid < files.size());

	CVirtualFile* file = files[fid];
	name = file->GetName();
	size = file->buffer.size();
}

CVirtualFile* CVirtualArchive::AddFile(const std::string& name)
{
	uint fid = files.size();

	CVirtualFile* file = new CVirtualFile(fid, name);
	files.push_back(file);

	lcNameIndex[name] = fid;

	return file;
}

CVirtualFile* CVirtualArchive::GetFile(const std::string& name)
{
	auto it = lcNameIndex.find(name);
	if (it != lcNameIndex.end()) {
		return files[it->second];
	}

	return nullptr;
}

void CVirtualArchive::WriteToFile()
{
	std::string zipFilePath = dataDirsAccess.LocateFile(fileName, FileQueryFlags::WRITE) + ".sdz";
	LOG("Writing zip file for virtual archive %s to %s", fileName.c_str(), zipFilePath.c_str());

	zipFile zip = zipOpen(zipFilePath.c_str(), APPEND_STATUS_CREATE);
	if (!zip) {
		LOG("Could not open zip file %s for writing", zipFilePath.c_str());
		return;
	}

	for (const CVirtualFile* file : files) {
		zipOpenNewFileInZip(zip, file->GetName().c_str(), NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION);
		zipWriteInFileInZip(zip, file->buffer.empty() ? NULL : &file->buffer[0], file->buffer.size());
		zipCloseFileInZip(zip);
	}

	zipClose(zip, NULL);
}
////////////////////////
// CVirtualArchive end
////////////////////////

/////////////////////////
// CVirtualArchive start
/////////////////////////
CVirtualFile::CVirtualFile(int fid, const std::string& name) : name(name), fid(fid)
{

}

CVirtualFile::~CVirtualFile()
{

}
/////////////////////////
// CVirtualArchive end
/////////////////////////
