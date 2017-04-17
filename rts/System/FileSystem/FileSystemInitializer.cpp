/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FileSystemInitializer.h"
#include "DataDirLocater.h"
#include "ArchiveScanner.h"
#include "VFSHandler.h"
#include "System/LogOutput.h"
#include "System/Util.h"
#include "System/Config/ConfigHandler.h"
#include "System/Platform/Misc.h"



bool FileSystemInitializer::initialized = false;

void FileSystemInitializer::PreInitializeConfigHandler(const std::string& configSource, const bool safemode)
{
	dataDirLocater.LocateDataDirs();
	dataDirLocater.ChangeCwdToWriteDir();
	ConfigHandler::Instantiate(configSource, safemode);
}


void FileSystemInitializer::InitializeLogOutput(const std::string& filename)
{
	if (!filename.empty() && !logOutput.IsInitialized())
		logOutput.SetFileName(filename);

	logOutput.Initialize();
}


void FileSystemInitializer::Initialize()
{
	if (initialized)
		return;

	try {
		Platform::SetOrigCWD();

		dataDirLocater.LocateDataDirs();
		dataDirLocater.Check();

		archiveScanner = new CArchiveScanner();
		vfsHandler = new CVFSHandler();

		initialized = true;
	} catch (const std::exception& ex) {
		// even if we end up here, do not clean up configHandler yet
		// since it can already have early observers registered that
		// do not remove themselves until exit
		Cleanup(false);
		throw;
	} catch (...) {
		Cleanup(false);
		throw;
	}
}

void FileSystemInitializer::Cleanup(bool deallocConfigHandler)
{
	if (initialized) {
		spring::SafeDelete(archiveScanner);
		spring::SafeDelete(vfsHandler);
		initialized = false;
	}

	if (deallocConfigHandler) {
		ConfigHandler::Deallocate();
	}
}
