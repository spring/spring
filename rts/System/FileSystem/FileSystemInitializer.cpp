/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FileSystemInitializer.h"
#include "DataDirLocater.h"
#include "ArchiveScanner.h"
#include "VFSHandler.h"
#include "System/Util.h"


bool FileSystemInitializer::initialized = false;

void FileSystemInitializer::Initialize()
{
	if (!initialized) {
		try {
			dataDirLocater.LocateDataDirs();
			archiveScanner = new CArchiveScanner();
			vfsHandler = new CVFSHandler();
			initialized = true;
		} catch (const std::exception& ex) {
			Cleanup();
			throw ex;
		} catch (...) {
			Cleanup();
			throw;
		}
	}
}

void FileSystemInitializer::Cleanup()
{
	if (initialized) {
		SafeDelete(archiveScanner);
		SafeDelete(vfsHandler);
		initialized = false;
	}
}
