/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FileSystemInitializer.h"
#include "DataDirLocater.h"
#include "ArchiveScanner.h"
#include "VFSHandler.h"
#include "System/Util.h"
#include "System/Platform/Misc.h"


bool FileSystemInitializer::initialized = false;

void FileSystemInitializer::Initialize()
{
	if (!initialized) {
		try {
			Platform::SetOrigCWD();
			dataDirLocater.LocateDataDirs();
			dataDirLocater.Check();
			archiveScanner = new CArchiveScanner();
			vfsHandler = new CVFSHandler();
			initialized = true;
		} catch (const std::exception& ex) {
			Cleanup();
			throw;
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
