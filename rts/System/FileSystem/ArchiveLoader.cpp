/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ArchiveLoader.h"

#include "Archives/ArchiveFactory.h"
#include "Archives/IArchive.h"
#include "Archives/PoolArchive.h"
#include "Archives/DirArchive.h"
#include "Archives/ZipArchive.h"
#include "Archives/SevenZipArchive.h"
#include "Archives/VirtualArchive.h"

#include "FileSystem.h"
#include "DataDirsAccess.h"

#include "System/SafeUtil.h"


CArchiveLoader::CArchiveLoader()
{
	// TODO maybe move ArchiveFactory registration to some external place
	AddFactory(new CPoolArchiveFactory());
	AddFactory(new CDirArchiveFactory());
	AddFactory(new CZipArchiveFactory());
	AddFactory(new CSevenZipArchiveFactory());
	AddFactory(new CVirtualArchiveFactory());
}

CArchiveLoader::~CArchiveLoader()
{
	std::map<std::string, IArchiveFactory*>::iterator afi;
	for (afi = archiveFactories.begin(); afi != archiveFactories.end(); ++afi) {
		spring::SafeDelete(afi->second);
	}
}


CArchiveLoader& CArchiveLoader::GetInstance()
{
	static CArchiveLoader singleton;
	return singleton;
}


bool CArchiveLoader::IsArchiveFile(const std::string& fileName) const
{
	const std::string ext = FileSystem::GetExtension(fileName);

	return (archiveFactories.find(ext) != archiveFactories.end());
}


IArchive* CArchiveLoader::OpenArchive(const std::string& fileName, const std::string& type) const
{
	IArchive* ret = NULL;

	const std::string ext = type.empty() ? FileSystem::GetExtension(fileName) : type;
	const std::string filePath = dataDirsAccess.LocateFile(fileName);

	const std::map<std::string, IArchiveFactory*>::const_iterator afi
			= archiveFactories.find(ext);

	if (afi != archiveFactories.end()) {
		ret = afi->second->CreateArchive(filePath);
	}

	if (ret && ret->IsOpen()) {
		return ret;
	}

	delete ret;
	return NULL;
}


void CArchiveLoader::AddFactory(IArchiveFactory* archiveFactory)
{
	assert(archiveFactory != NULL);
	// ensure unique extensions
	assert(archiveFactories.find(archiveFactory->GetDefaultExtension()) == archiveFactories.end());

	archiveFactories[archiveFactory->GetDefaultExtension()] = archiveFactory;
}
