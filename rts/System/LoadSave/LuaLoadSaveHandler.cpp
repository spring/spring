/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>
#include <sstream>

#include "LuaLoadSaveHandler.h"

#include "minizip/zip.h"

#include "ExternalAI/SkirmishAIHandler.h"
#include "ExternalAI/EngineOutHandler.h"
#include "Game/GameSetup.h"
#include "Lua/LuaZip.h"
#include "Map/MapDamage.h"
#include "Map/ReadMap.h"
#include "System/FileSystem/Archives/IArchive.h"
#include "System/FileSystem/ArchiveLoader.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/Platform/byteorder.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "System/SafeUtil.h"



// Prefix for all files in the save file.
// May be used to prevent clashes with other code. (AI, Lua)
#define PREFIX "Spring/"

// Names of files in save file for various components of engine.
// They all have a version number as suffix. When breaking compatibility
// in the respective file format, please increment its version number.
static const char* FILE_STARTSCRIPT  = PREFIX"startscript.0";
static const char* FILE_AIDATA       = PREFIX"aidata.0";
static const char* FILE_HEIGHTMAP    = PREFIX"heightmap.0";

#undef PREFIX


CLuaLoadSaveHandler::CLuaLoadSaveHandler()
	: savefile(nullptr)
	, loadfile(nullptr)
{
}


CLuaLoadSaveHandler::~CLuaLoadSaveHandler()
{
	delete loadfile;
}


void CLuaLoadSaveHandler::SaveGame(const std::string& file)
{
	const std::string realname = dataDirsAccess.LocateFile(file, FileQueryFlags::WRITE);

	filename = file;
	savefile = nullptr;

	try {
		// Remove any existing file
		FileSystem::Remove(realname);

		// Open the zip
		if (realname.empty() ||
				(savefile = zipOpen(realname.c_str(), APPEND_STATUS_CREATE)) == nullptr) {
			throw content_error("Unable to open save file \"" + filename + "\"");
		}

		SaveEventClients();
		SaveGameStartInfo();
		SaveAIData();
		SaveHeightmap();

		// Close zip file.
		if (Z_OK != zipClose(savefile, "Spring save file, visit https://springrts.com/ for details.")) {
			LOG_L(L_ERROR, "Unable to close save file \"%s\"", filename.c_str());
		}
		return; // Success
	}
	catch (const content_error& ex) {
		LOG_L(L_ERROR, "Save failed(content error): %s", ex.what());
	}
	catch (const std::exception& ex) {
		LOG_L(L_ERROR, "Save failed: %s", ex.what());
	}
	catch (const char*& exStr) {
		LOG_L(L_ERROR, "Save failed: %s", exStr);
	}
	catch (...) {
		LOG_L(L_ERROR, "Save failed(unknown error)");
	}

	// Failure => cleanup
	if (savefile != nullptr) {
		zipClose(savefile, nullptr);
		savefile = nullptr;
		FileSystem::Remove(realname);
	}
}


void CLuaLoadSaveHandler::SaveEventClients()
{
	// FIXME: need some way to 'chroot' them into a single directory?
	//        (maybe abstract zipFile void* after all...)
	eventHandler.Save(savefile);
}


void CLuaLoadSaveHandler::SaveGameStartInfo()
{
	const std::string scriptText = gameSetup->setupText;
	SaveEntireFile(FILE_STARTSCRIPT, "game setup", scriptText.data(), scriptText.size());
}


void CLuaLoadSaveHandler::SaveAIData()
{
	for (const auto& ai: skirmishAIHandler.GetAllSkirmishAIs()) {
		std::stringstream aiData;
		eoh->Save(&aiData, ai.first);

		const std::string aiSection = FILE_AIDATA + IntToString(ai.first, ".%i");
		const std::string aiDataStr = aiData.str();

		SaveEntireFile(aiSection.c_str(), "AI data", aiDataStr.data(), aiData.tellp());
	}
}


void CLuaLoadSaveHandler::SaveHeightmap()
{
	// This implements a trivial compression algorithm (relying on zip):
	// For every heightmap pixel the bits are XOR'ed with the orig bits,
	// so that unmodified terrain comes out as 0.
	// Big chunks of 0s are then very well compressed by zip.
	const int* currHeightmap = (const int*) (const char*) readMap->GetCornerHeightMapSynced();
	const int* origHeightmap = (const int*) (const char*) readMap->GetOriginalHeightMapSynced();
	const int size = mapDims.mapxp1 * mapDims.mapyp1;
	int* temp = new int[size];
	for (int i = 0; i < size; ++i) {
		temp[i] = swabDWord(currHeightmap[i] ^ origHeightmap[i]);
	}
	SaveEntireFile(FILE_HEIGHTMAP, "heightmap", temp, size * sizeof(int));
	delete[] temp;
}


void CLuaLoadSaveHandler::SaveEntireFile(const char* file, const char* what, const void* data, int size, bool throwOnError)
{
	std::string failedOperation;

	if (Z_OK != zipOpenNewFileInZip(savefile, file, nullptr, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_BEST_COMPRESSION)) {
		failedOperation = "open";
	}
	else if (Z_OK != zipWriteInFileInZip(savefile, data, size)) {
		failedOperation = "write";
	}
	else if (Z_OK != zipCloseFileInZip(savefile)) {
		failedOperation = "close";
	}

	if (!failedOperation.empty()) {
		const std::string error = "Unable to " + failedOperation + " " + what
				+ " file in save file \"" + filename + "\"";
		if (throwOnError) {
			throw content_error(error);
		} else {
			LOG_L(L_ERROR, "%s", error.c_str());
		}
	}
}


void CLuaLoadSaveHandler::LoadGameStartInfo(const std::string& file)
{
	const std::string realfile = dataDirsAccess.LocateFile(FindSaveFile(filename = file));

	if ((loadfile = archiveLoader.OpenArchive(realfile, "sdz")) == nullptr || !loadfile->IsOpen())
		throw content_error("Unable to open savegame \"" + filename + "\"");

	scriptText = LoadEntireFile(FILE_STARTSCRIPT);
}


void CLuaLoadSaveHandler::LoadGame()
{
	ENTER_SYNCED_CODE();

	LoadEventClients();
	LoadAIData();
	LoadHeightmap();

	LEAVE_SYNCED_CODE();
}


void CLuaLoadSaveHandler::LoadEventClients()
{
	// FIXME: need some way to 'chroot' them into a single directory? absolute-ify & check path-prefix?
	eventHandler.Load(loadfile);
}


void CLuaLoadSaveHandler::LoadAIData()
{
	for (const auto& ai: skirmishAIHandler.GetAllSkirmishAIs()) {
		const std::string aiSection = FILE_AIDATA + IntToString(ai.first, ".%i");
		const std::string aiDataFile = LoadEntireFile(aiSection);

		std::stringstream aiData(aiDataFile);

		eoh->Load(&aiData, ai.first);
	}
}


void CLuaLoadSaveHandler::LoadHeightmap()
{
	std::vector<std::uint8_t> buf;

	if (loadfile->GetFile(FILE_HEIGHTMAP, buf)) {
		const int size = mapDims.mapxp1 * mapDims.mapyp1;
		const int* temp = (const int*) (const char*) &*buf.begin();
		const int* origHeightmap = (const int*) (const char*) readMap->GetOriginalHeightMapSynced();

		for (int i = 0; i < size; ++i) {
			const int newHeightBits = swabDWord(temp[i]) ^ origHeightmap[i];
			const float newHeight = *(const float*) (const char*) &newHeightBits;
			readMap->SetHeight(i, newHeight);
		}
		mapDamage->RecalcArea(0, mapDims.mapx, 0, mapDims.mapy);
	} else {
		LOG_L(L_ERROR, "Unable to load heightmap from save file \"%s\"", filename.c_str());
	}
}


std::string CLuaLoadSaveHandler::LoadEntireFile(const std::string& file)
{
	std::vector<std::uint8_t> buf;
	if (loadfile->GetFile(file, buf)) {
		return std::string((char*) &*buf.begin(), buf.size());
	}
	return "";
}
