/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FileSystemInitializer.h"
#include "DataDirLocater.h"
#include "ArchiveScanner.h"
#include "VFSHandler.h"
#include "System/LogOutput.h"
#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "System/Config/ConfigHandler.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/Misc.h"
#include "System/Platform/Watchdog.h"

#ifdef UNITSYNC
void ErrorMessageBox(const char*, const char*, unsigned int) { throw; } // pass to US
#endif

#if (!defined(UNITSYNC) && !defined(DEDICATED))
static void SetupThreadReg() {
	Threading::SetFileSysThread();
	Watchdog::RegisterThread(WDT_VFSI);
}
static void ClearThreadReg() {
	Watchdog::DeregisterThread(WDT_VFSI);
}
#else
static void SetupThreadReg() {}
static void ClearThreadReg() {}
#endif


std::atomic<bool> FileSystemInitializer::initSuccess = {false};
std::atomic<bool> FileSystemInitializer::initFailure = {false};

void FileSystemInitializer::PreInitializeConfigHandler(const std::string& configSource, const std::string& configName, const bool safemode)
{
	dataDirLocater.LocateDataDirs();
	dataDirLocater.ChangeCwdToWriteDir();

	ConfigHandler::Instantiate(configSource, safemode);

	if (configName.empty())
		return;

	configHandler->SetString("name", StringReplace(configName, " ", "_"));
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

	SetupThreadReg();

	try {
		Platform::SetOrigCWD();

		dataDirLocater.LocateDataDirs();
		dataDirLocater.Check();

		archiveScanner = new CArchiveScanner();
		CVFSHandler::SetGlobalInstance(new CVFSHandler());

		initSuccess = true;
	} catch (const std::exception& ex) {
		// abort VFS-init thread
		initFailure = true;

		// even if we end up here, do not clean up configHandler yet
		// since it can already have early observers registered that
		// do not remove themselves until exit
		ErrorMessageBox(ex.what(), "Spring: caught std::exception", MBF_OK | MBF_EXCL);
	} catch (...) {
		initFailure = true;

		ErrorMessageBox("", "Spring: caught generic exception", MBF_OK | MBF_EXCL);
	}

	// in case of an exception, ErrorMessageBox takes care of this
	ClearThreadReg();

	return (initSuccess && !initFailure);
}

void FileSystemInitializer::Cleanup(bool deallocConfigHandler)
{
	if (initSuccess) {
		spring::SafeDelete(archiveScanner);
		CVFSHandler::FreeGlobalInstance();

		initSuccess = false;
		initFailure = false;
	}

	if (deallocConfigHandler) {
		ConfigHandler::Deallocate();
	}
}

void FileSystemInitializer::Reload()
{
	// repopulated by PreGame, etc
	vfsHandler->DeleteArchives();
	archiveScanner->Reload();
}

