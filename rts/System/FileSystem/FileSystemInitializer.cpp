/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FileSystemInitializer.h"
#include "DataDirLocater.h"
#include "ArchiveScanner.h"
#include "VFSHandler.h"
#include "System/LogOutput.h"
#include "System/SafeUtil.h"
#include "System/Config/ConfigHandler.h"
#include "System/Platform/Misc.h"



volatile bool FileSystemInitializer::initSuccess = false;
volatile bool FileSystemInitializer::initFailure = false;

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


bool FileSystemInitializer::Initialize()
{
	if (initSuccess)
		return true;

	try {
		Platform::SetOrigCWD();

		dataDirLocater.LocateDataDirs();
		dataDirLocater.Check();

		archiveScanner = new CArchiveScanner();
		vfsHandler = new CVFSHandler();

		initSuccess = true;
	} catch (const std::exception& ex) {
		// even if we end up here, do not clean up configHandler yet
		// since it can already have early observers registered that
		// do not remove themselves until exit
		logOutput.LogExceptionInfo("FileSystemInit", ex.what());
		Cleanup(false);
		initFailure = true;
	} catch (...) {
		Cleanup(false);
		initFailure = true;
	}

	return (initSuccess && !initFailure);
}

void FileSystemInitializer::Cleanup(bool deallocConfigHandler)
{
	if (initSuccess) {
		spring::SafeDelete(archiveScanner);
		spring::SafeDelete(vfsHandler);

		initSuccess = false;
		initFailure = false;
	}

	if (deallocConfigHandler) {
		ConfigHandler::Deallocate();
	}
}
