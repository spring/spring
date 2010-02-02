/* Author: Tobi Vollebregt */

#include "LuaLoadSaveHandler.h"

#include <sstream>
#include "lib/minizip/zip.h"

#include "ExternalAI/EngineOutHandler.h"
#include "Game/GameSetup.h"
#include "Lua/LuaZip.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/ArchiveZip.h"
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

#undef PREFIX


CLuaLoadSaveHandler::CLuaLoadSaveHandler()
: loadfile(NULL)
{
}


CLuaLoadSaveHandler::~CLuaLoadSaveHandler()
{
	delete loadfile;
}


void CLuaLoadSaveHandler::SaveGame(const std::string& file)
{
	const int z_method = Z_DEFLATED;
	const int z_level = Z_BEST_COMPRESSION;

	const std::string realname = filesystem.LocateFile(file, FileSystem::WRITE).c_str();
	zipFile zip = NULL;

	try {
		// Remove any existing file
		filesystem.Remove(realname);

		// Open the zip
		if (realname.empty() ||
				(zip = zipOpen(realname.c_str(), APPEND_STATUS_CREATE)) == NULL) {
			throw content_error("Unable to open save file \"" + file + "\"");
		}

		// Save game setup
		const std::string scriptText = gameSetup->gameSetupText;
		if (Z_OK != zipOpenNewFileInZip(zip, FILE_STARTSCRIPT, NULL, NULL, 0, NULL, 0, NULL, z_method, z_level)) {
			throw content_error("Unable to open game setup file in save file \"" + file + "\"");
		}
		else if (Z_OK != zipWriteInFileInZip(zip, scriptText.data(), scriptText.size())) {
			throw content_error("Unable to write game setup file in save file \"" + file + "\"");
		}

		// Allow all event clients to save
		// FIXME: need some way to 'chroot' them into a single directory?
		//        (maybe abstract zipFile void* after all...)
		eventHandler.Save(zip);

		// Save AI data (first to a stringstream, to be able to use current interface)
		// FIXME: maybe expose richer stream to AI interface?
		//        (e.g. one file in the zip per AI?)
		std::stringstream aidata;
		eoh->Save(&aidata);

		// Save AI data (stringstream => file, errors are not fatal)
		if (Z_OK != zipOpenNewFileInZip(zip, FILE_AIDATA, NULL, NULL, 0, NULL, 0, NULL, z_method, z_level)) {
			logOutput.Print("Unable to open AI data file in save file \"" + file + "\"");
		}
		else if (Z_OK != zipWriteInFileInZip(zip, aidata.str().data(), aidata.tellp())) {
			logOutput.Print("Unable to write AI data to save file \"" + file + "\"");
		}

		// Close zip file.
		if (Z_OK != zipClose(zip, "Spring save file, visit http://springrts.com/ for details.")) {
			logOutput.Print("Unable to close save file \"" + file + "\"");
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
	if (zip != NULL) {
		zipClose(zip, NULL);
		filesystem.Remove(realname);
	}
}


void CLuaLoadSaveHandler::LoadGameStartInfo(const std::string& file)
{
	const std::string realfile = filesystem.LocateFile(FindSaveFile(file)).c_str();

	loadfile = new CArchiveZip(realfile);

	if (!loadfile->IsOpen()) {
		logOutput.Print("Unable to open save file \"" + file + "\"");
		return;
	}

	scriptText = LoadEntireFile(FILE_STARTSCRIPT);
}


void CLuaLoadSaveHandler::LoadGame()
{
	// Allow event clients to load
	eventHandler.Load(loadfile);

	// Load AI data
	std::stringstream aidata(LoadEntireFile(FILE_AIDATA));
	eoh->Load(&aidata);
}


std::string CLuaLoadSaveHandler::LoadEntireFile(const std::string& file)
{
	const int handle = loadfile->OpenFile(file);
	if (handle == 0) {
		return "";
	}

	const int size = loadfile->FileSize(handle);
	char* data = new char[size];
	loadfile->ReadFile(handle, data, size);
	loadfile->CloseFile(handle);

	return std::string(data, size);
}
