/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "VirtualArchive.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/Log/ILog.h"

#include "minizip/zip.h"
#include <cassert>

CVirtualArchiveFactory* virtualArchiveFactory;

CVirtualArchiveFactory::CVirtualArchiveFactory() : IArchiveFactory("sva")
{
	virtualArchiveFactory = this;
}

CVirtualArchiveFactory::~CVirtualArchiveFactory()
{
	virtualArchiveFactory = 0;
}

CVirtualArchive* CVirtualArchiveFactory::AddArchive(const std::string& fileName)
{
	CVirtualArchive* archive = new CVirtualArchive(fileName);
	archives.push_back(archive);
	return archive;
}

IArchive* CVirtualArchiveFactory::DoCreateArchive(const std::string& fileName) const
{
	const std::string baseName = FileSystem::GetBasename(fileName);

	for(std::vector<CVirtualArchive*>::const_iterator it = archives.begin(); it != archives.end(); ++it)
	{
		CVirtualArchive* archive = *it;
		if(archive->GetFileName() == baseName)
		{
			return archive->Open();
		}
	}

	return 0;
}

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

bool CVirtualArchiveOpen::GetFile( unsigned int fid, std::vector<std::uint8_t>& buffer )
{
	return archive->GetFile(fid, buffer);
}

void CVirtualArchiveOpen::FileInfo( unsigned int fid, std::string& name, int& size ) const
{
	return archive->FileInfo(fid, name, size);
}

CVirtualArchive::CVirtualArchive(const std::string& fileName) : fileName(fileName)
{
}

CVirtualArchive::~CVirtualArchive()
{
	for(std::vector<CVirtualFile*>::iterator it = files.begin(); it != files.end(); ++it)
	{
		delete *it;
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

bool CVirtualArchive::GetFile( unsigned int fid, std::vector<std::uint8_t>& buffer )
{
	if(fid >= files.size()) return false;

	CVirtualFile* file = files[fid];
	buffer = file->buffer;

	return true;
}

void CVirtualArchive::FileInfo( unsigned int fid, std::string& name, int& size ) const
{
	assert(fid < files.size());

	CVirtualFile* file = files[fid];
	name = file->GetName();
	size = file->buffer.size();
}

CVirtualFile* CVirtualArchive::AddFile(const std::string& name)
{
	unsigned int fidcounter = files.size();
	CVirtualFile* file = new CVirtualFile(fidcounter, name);
	files.push_back(file);

	lcNameIndex[name] = fidcounter;

	return file;
}

void CVirtualArchive::WriteToFile()
{
	std::string zipFilePath = dataDirsAccess.LocateFile(fileName, FileQueryFlags::WRITE) + ".sdz";
	LOG("Writing zip file for virtual archive %s to %s", fileName.c_str(), zipFilePath.c_str());

	zipFile zip = zipOpen(zipFilePath.c_str(), APPEND_STATUS_CREATE);
	if(!zip)
	{
		LOG("Could not open zip file %s for writing", zipFilePath.c_str());
		return;
	}

	for(std::vector<CVirtualFile*>::iterator it = files.begin(); it != files.end(); ++it)
	{
		CVirtualFile* file = *it;

		zipOpenNewFileInZip(zip, file->GetName().c_str(), NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION);
		zipWriteInFileInZip(zip, file->buffer.empty() ? NULL : &file->buffer[0], file->buffer.size());
		zipCloseFileInZip(zip);
	}

	zipClose(zip, NULL);
}

CVirtualFile::CVirtualFile(int fid, const std::string& name) : name(name), fid(fid)
{

}

CVirtualFile::~CVirtualFile()
{

}
