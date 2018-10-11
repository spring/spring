/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>

#include "ArchiveLoader.h"

#include "Archives/IArchiveFactory.h"
#include "Archives/IArchive.h"
#include "Archives/PoolArchive.h"
#include "Archives/DirArchive.h"
#include "Archives/ZipArchive.h"
#include "Archives/SevenZipArchive.h"
#include "Archives/VirtualArchive.h"

#include "FileSystem.h"
#include "DataDirsAccess.h"
#include "System/Log/ILog.h"

static CPoolArchiveFactory sdpArchiveFactory;
static CDirArchiveFactory sddArchiveFactory;
static CZipArchiveFactory sdzArchiveFactory;
static CSevenZipArchiveFactory sd7ArchiveFactory;
static CVirtualArchiveFactory sdvArchiveFactory;

CArchiveLoader::CArchiveLoader()
{
	const auto AddFactory = [&](unsigned archiveType, IArchiveFactory& factory) {
		archiveFactories[archiveType] = {factory.GetDefaultExtension(), &factory};
	};
	AddFactory(ARCHIVE_TYPE_SDP, sdpArchiveFactory);
	AddFactory(ARCHIVE_TYPE_SDD, sddArchiveFactory);
	AddFactory(ARCHIVE_TYPE_SDZ, sdzArchiveFactory);
	AddFactory(ARCHIVE_TYPE_SD7, sd7ArchiveFactory);
	AddFactory(ARCHIVE_TYPE_SDV, sdvArchiveFactory);

	using P = decltype(archiveFactories)::value_type;
	std::sort(archiveFactories.begin(), archiveFactories.end(), [](const P& a, const P& b) { return (a.first < b.first); });
}


const CArchiveLoader& CArchiveLoader::GetInstance()
{
	static const CArchiveLoader singleton;
	return singleton;
}


bool CArchiveLoader::IsArchiveFile(const std::string& fileName) const
{
	const std::string fileExt = FileSystem::GetExtension(fileName);

	using P = decltype(archiveFactories)::value_type;

	const auto pred = [](const P& a, const P& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(archiveFactories.begin(), archiveFactories.end(), P{fileExt, nullptr}, pred);

	return (iter != archiveFactories.end() && iter->first == fileExt);
}


IArchive* CArchiveLoader::OpenArchive(const std::string& fileName, const std::string& type) const
{
	IArchive* ret = nullptr;

	const std::string fileExt = type.empty() ? FileSystem::GetExtension(fileName) : type;
	const std::string filePath = dataDirsAccess.LocateFile(fileName);

	using P = decltype(archiveFactories)::value_type;

	const auto pred = [](const P& a, const P& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(archiveFactories.begin(), archiveFactories.end(), P{fileExt, nullptr}, pred);

	if (iter != archiveFactories.end() && iter->first == fileExt)
		ret = iter->second->CreateArchive(filePath);

	if (ret != nullptr && ret->IsOpen())
		return ret;

	LOG_L(L_INFO, "[ArchiveLoader::%s(name=\"%s\" type=\"%s\")] could not load or open archive %p", __func__, fileName.c_str(), type.c_str(), ret);

	delete ret;
	return nullptr;
}

