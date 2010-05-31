/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>
#include <sstream>

#include "StdAfx.h"
#include "LuaLoadSaveHandler.h"

#include "lib/minizip/zip.h"

#include "ExternalAI/EngineOutHandler.h"
#include "Game/GameSetup.h"
#include "Lua/LuaZip.h"
#include "Map/MapDamage.h"
#include "Map/ReadMap.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/ArchiveZip.h"
#include "System/Platform/byteorder.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/LogOutput.h"

#include "mmgr.h"


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
: savefile(NULL), loadfile(NULL)
{
}


CLuaLoadSaveHandler::~CLuaLoadSaveHandler()
{
	delete loadfile;
}


void CLuaLoadSaveHandler::SaveGame(const std::string& file)
{
	const std::string realname = filesystem.LocateFile(file, FileSystem::WRITE).c_str();

	filename = file;
	savefile = NULL;

	try {
		// Remove any existing file
		filesystem.Remove(realname);

		// Open the zip
		if (realname.empty() ||
				(savefile = zipOpen(realname.c_str(), APPEND_STATUS_CREATE)) == NULL) {
			throw content_error("Unable to open save file \"" + filename + "\"");
		}

		SaveEventClients();
		SaveGameStartInfo();
		SaveAIData();
		SaveHeightmap();

		// Close zip file.
		if (Z_OK != zipClose(savefile, "Spring save file, visit http://springrts.com/ for details.")) {
			logOutput.Print("Unable to close save file \"" + filename + "\"");
		}
		return; // Success
	}
	catch (content_error &e) {
		logOutput.Print("Save failed(content error): %s", e.what());
	}
	catch (std::exception &e) {
		logOutput.Print("Save failed: %s", e.what());
	}
	catch (char* &e) {
		logOutput.Print("Save failed: %s", e);
	}
	catch (...) {
		logOutput.Print("Save failed(unknown error)");
	}

	// Failure => cleanup
	if (savefile != NULL) {
		zipClose(savefile, NULL);
		savefile = NULL;
		filesystem.Remove(realname);
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
	const std::string scriptText = gameSetup->gameSetupText;
	SaveEntireFile(FILE_STARTSCRIPT, "game setup", scriptText.data(), scriptText.size());
}


void CLuaLoadSaveHandler::SaveAIData()
{
	// Save to a stringstream first, to be able to use current interface.
	// FIXME: maybe expose richer stream to AI interface?
	//        (e.g. one file in the zip per AI?)
	std::stringstream aidata;
	eoh->Save(&aidata);
	SaveEntireFile(FILE_AIDATA, "AI data", aidata.str().data(), aidata.tellp());
}


void CLuaLoadSaveHandler::SaveHeightmap()
{
	// This implements a trivial compression algorithm (relying on zip):
	// For every heightmap pixel the bits are XOR'ed with the orig bits,
	// so that unmodified terrain comes out as 0.
	// Big chunks of 0s are then very well compressed by zip.
	const int* currHeightmap = (const int*) (const char*) readmap->GetHeightmap();
	const int* origHeightmap = (const int*) (const char*) readmap->orgheightmap;
	const int size = (gs->mapx + 1) * (gs->mapy + 1);
	int* temp = new int[size];
	for (int i = 0; i < size; ++i) {
		temp[i] = swabdword(currHeightmap[i] ^ origHeightmap[i]);
	}
	SaveEntireFile(FILE_HEIGHTMAP, "heightmap", temp, size * sizeof(int));
	delete[] temp;
}


void CLuaLoadSaveHandler::SaveEntireFile(const char* file, const char* what, const void* data, int size, bool throwOnError)
{
	std::string err;

	if (Z_OK != zipOpenNewFileInZip(savefile, file, NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION)) {
		err = "open";
	}
	else if (Z_OK != zipWriteInFileInZip(savefile, data, size)) {
		err = "write";
	}
	else if (Z_OK != zipCloseFileInZip(savefile)) {
		err = "close";
	}

	if (!err.empty()) {
		err = "Unable to " + err + " " + what + " file in save file \"" + filename + "\"";
		if (throwOnError) {
			throw content_error(err);
		}
		else {
			logOutput.Print(err);
		}
	}
}


void CLuaLoadSaveHandler::LoadGameStartInfo(const std::string& file)
{
	const std::string realfile = filesystem.LocateFile(FindSaveFile(file)).c_str();

	filename = file;
	loadfile = new CArchiveZip(realfile);

	if (!loadfile->IsOpen()) {
		logOutput.Print("Unable to open save file \"" + filename + "\"");
		return;
	}

	scriptText = LoadEntireFile(FILE_STARTSCRIPT);
}


void CLuaLoadSaveHandler::LoadGame()
{
	LoadEventClients();
	LoadAIData();
	LoadHeightmap();
}


void CLuaLoadSaveHandler::LoadEventClients()
{
	// FIXME: need some way to 'chroot' them into a single directory?
	eventHandler.Load(loadfile);
}


void CLuaLoadSaveHandler::LoadAIData()
{
	std::stringstream aidata(LoadEntireFile(FILE_AIDATA));
	eoh->Load(&aidata);
}


void CLuaLoadSaveHandler::LoadHeightmap()
{
	std::vector<boost::uint8_t> buf;

	if (loadfile->GetFile(FILE_HEIGHTMAP, buf)) {
		const int size = (gs->mapx + 1) * (gs->mapy + 1);
		const int* temp = (const int*) (const char*) &*buf.begin();
		const int* origHeightmap = (const int*) (const char*) readmap->orgheightmap;

		for (int i = 0; i < size; ++i) {
			const int newHeightBits = swabdword(temp[i]) ^ origHeightmap[i];
			const float newHeight = *(const float*) (const char*) &newHeightBits;
			readmap->SetHeight(i, newHeight);
		}
		mapDamage->RecalcArea(0, gs->mapx, 0, gs->mapy);
	}
	else {
		logOutput.Print("Unable to load heightmap from save file \"" + filename + "\"");
	}
}


std::string CLuaLoadSaveHandler::LoadEntireFile(const std::string& file)
{
	std::vector<boost::uint8_t> buf;
	if (loadfile->GetFile(file, buf)) {
		return std::string((char*) &*buf.begin(), buf.size());
	}
	return "";
}
