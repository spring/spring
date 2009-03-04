/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Interface/SAIInterfaceCallback.h"

#include "Game/GameVersion.h"
#include "Game/GameSetup.h"
#include "FileSystem/FileSystem.h"
//#include "FileSystem/DataDirectories.h"
#include "Sim/Misc/GlobalConstants.h" // for MAX_TEAMS
#include "Sim/Misc/TeamHandler.h" // ActiveTeams()
#include "IAILibraryManager.h"
#include "AIInterfaceLibraryInfo.h"
#include "LogOutput.h"
#include "Interface/ELevelOfSupport.h"     // for ABI version
#include "Interface/SAIFloat3.h"            // for ABI version
#include "Interface/AISEvents.h"           // for ABI version
#include "Interface/AISCommands.h"         // for ABI version
#include "Interface/SSAILibrary.h"         // for ABI version
#include "Interface/SAIInterfaceLibrary.h" // for ABI version and AI_INTERFACE_PROPERTY_*

#include <vector>
#include <stdlib.h> // malloc(), calloc(), free()


static std::vector<const CAIInterfaceLibraryInfo*> infos;

EXPORT(void) _Log_exception(int interfaceId, const char* const msg, int severety, bool die);
#define CHECK_INTERFACE_ID(interfaceId) \
	if (interfaceId < 0 || (size_t)interfaceId >= infos.size()) { \
		const static size_t interfaceIdError_maxSize = 512; \
		char interfaceIdError[interfaceIdError_maxSize]; \
		SNPRINTF(interfaceIdError, interfaceIdError_maxSize, \
				"Bad AI Interface ID supplied by an interface.\n" \
				"Is %i, but should be between min %i and  max %u.", \
				interfaceId, 0, infos.size()); \
		/* log exception to the engine and die */ \
		_Log_exception(interfaceId, interfaceIdError, 1, true); \
	}


EXPORT(int) _Engine_AIInterface_ABIVersion_getFailPart(int interfaceId) {
	return AIINTERFACE_ABI_VERSION_FAIL;
}
EXPORT(int) _Engine_AIInterface_ABIVersion_getWarningPart(int interfaceId) {
	return AIINTERFACE_ABI_VERSION_WARNING;
}

EXPORT(const char* const) _Engine_Version_getMajor(int interfaceId) {
	return SpringVersion::Major;
}
EXPORT(const char* const) _Engine_Version_getMinor(int interfaceId) {
	return SpringVersion::Minor;
}
EXPORT(const char* const) _Engine_Version_getPatchset(int interfaceId) {
	return SpringVersion::Patchset;
}
EXPORT(const char* const) _Engine_Version_getAdditional(int interfaceId) {
	return SpringVersion::Additional;
}
EXPORT(const char* const) _Engine_Version_getBuildTime(int interfaceId) {
	return SpringVersion::BuildTime;
}
EXPORT(const char* const) _Engine_Version_getNormal(int interfaceId) {
	return SpringVersion::Get().c_str();
}
EXPORT(const char* const) _Engine_Version_getFull(int interfaceId) {
	return SpringVersion::GetFull().c_str();
}




EXPORT(int              ) _AIInterface_Info_getSize(int interfaceId) {

	CHECK_INTERFACE_ID(interfaceId);

	const CAIInterfaceLibraryInfo* info = infos[interfaceId];
	return (int)info->size();
}
EXPORT(const char* const) _AIInterface_Info_getKey(int interfaceId, int infoIndex) {

	CHECK_INTERFACE_ID(interfaceId);

	const CAIInterfaceLibraryInfo* info = infos[interfaceId];
	return info->GetKeyAt(infoIndex).c_str();
}
EXPORT(const char* const) _AIInterface_Info_getValue(int interfaceId, int infoIndex) {

	CHECK_INTERFACE_ID(interfaceId);

	const CAIInterfaceLibraryInfo* info = infos[interfaceId];
	return info->GetValueAt(infoIndex).c_str();
}
EXPORT(const char* const) _AIInterface_Info_getDescription(int interfaceId, int infoIndex) {

	CHECK_INTERFACE_ID(interfaceId);

	const CAIInterfaceLibraryInfo* info = infos[interfaceId];
	return info->GetDescriptionAt(infoIndex).c_str();
}
EXPORT(const char* const) _AIInterface_Info_getValueByKey(int interfaceId, const char* const key) {

	CHECK_INTERFACE_ID(interfaceId);

	const CAIInterfaceLibraryInfo* info = infos[interfaceId];
	return info->GetInfo(key).c_str();
}

EXPORT(int) _Teams_getSize(int interfaceId) {
	return teamHandler->ActiveTeams();
}

EXPORT(int) _SkirmishAIs_getSize(int interfaceId) {
	return gameSetup->GetSkirmishAIs();
}
EXPORT(int) _SkirmishAIs_getMax(int interfaceId) {
	return MAX_TEAMS;
}
EXPORT(const char* const) _SkirmishAIs_Info_getValueByKey(int interfaceId, const char* const shortName, const char* const version, const char* const key) {

	const char* value = NULL;

	SkirmishAIKey aiKey(shortName, version);
	const IAILibraryManager::T_skirmishAIInfos& skirmishInfos = IAILibraryManager::GetInstance()->GetSkirmishAIInfos();
	IAILibraryManager::T_skirmishAIInfos::const_iterator inf = skirmishInfos.find(aiKey);
	if (inf != skirmishInfos.end()) {
		const std::string& valueStr = inf->second->GetInfo(key);
		if (valueStr != "") {
			value = valueStr.c_str();
		}
	}

	return value;
}

EXPORT(void) _Log_log(int interfaceId, const char* const msg) {

	CHECK_INTERFACE_ID(interfaceId);

	const CAIInterfaceLibraryInfo* info = infos[interfaceId];
	logOutput.Print("AI Interface <%s-%s>: %s", info->GetName().c_str(), info->GetVersion().c_str(), msg);
}
EXPORT(void) _Log_exception(int interfaceId, const char* const msg, int severety, bool die) {

	CHECK_INTERFACE_ID(interfaceId);

	const CAIInterfaceLibraryInfo* info = infos[interfaceId];
	logOutput.Print("AI Interface <%s-%s>: error, severety %i: [%s] %s",
			info->GetName().c_str(), info->GetVersion().c_str(), severety,
			(die ? "AI shut down" : "AI still running"), msg);
	if (die) {
		// TODO: FIXME: unload all skirmish AIs of this interface plus the interface itsself
		//IAILibraryManager::GetInstance()->ReleaseSkirmishAILibrary(...);
	}
}

EXPORT(char) _DataDirs_getPathSeparator(int interfaceId) {
#ifdef _WIN32
	return '\\';
#else
	return '/';
#endif
}
EXPORT(int) _DataDirs_Roots_getSize(int interfaceId) {

	const std::vector<std::string> dds =
			FileSystemHandler::GetInstance().GetDataDirectories();
	return dds.size();
}
EXPORT(bool) _DataDirs_Roots_getDir(int interfaceId, char* path, int path_sizeMax, int dirIndex) {

	const std::vector<std::string> dds =
			FileSystemHandler::GetInstance().GetDataDirectories();
	size_t numDataDirs = dds.size();
	if (dirIndex >= 0 && (size_t)dirIndex < numDataDirs) {
		STRNCPY(path, dds[dirIndex].c_str(), path_sizeMax);
		return true;
	} else {
		return false;
	}
}
EXPORT(bool) _DataDirs_Roots_locatePath(int interfaceId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir) {

	bool exists = false;

	int locateFlags = 0;
	if (writeable) {
		locateFlags = locateFlags | FileSystem::WRITE;
		if (create) {
			locateFlags = locateFlags | FileSystem::CREATE_DIRS;
		}
	}
	std::string locatedPath = "";
	char tmpRelPath[strlen(relPath) + 1];
	STRCPY(tmpRelPath, relPath);
	std::string tmpRelPathStr = tmpRelPath;
	if (dir) {
		locatedPath = filesystem.LocateDir(tmpRelPathStr, locateFlags);
	} else {
		locatedPath = filesystem.LocateFile(tmpRelPathStr, locateFlags);
	}
	exists = (locatedPath != relPath);
	STRNCPY(path, locatedPath.c_str(), path_sizeMax);

	return exists;
}
EXPORT(char*) _DataDirs_Roots_allocatePath(int interfaceId, const char* const relPath, bool writeable, bool create, bool dir) {

	static const unsigned int path_sizeMax = 2048;

	char* path = (char*) calloc(path_sizeMax, sizeof(char*));
	bool fetchOk = _DataDirs_Roots_locatePath(interfaceId, path, path_sizeMax, relPath, writeable, create, dir);

	if (!fetchOk) {
		free(path);
		path = NULL;
	}

	return path;
}
EXPORT(const char* const) _DataDirs_getConfigDir(int interfaceId) {

	CHECK_INTERFACE_ID(interfaceId);

	const CAIInterfaceLibraryInfo* info = infos[interfaceId];
	return info->GetDataDir().c_str();
}
EXPORT(bool) _DataDirs_locatePath(int interfaceId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir) {

	bool exists = false;

	char ps = _DataDirs_getPathSeparator(interfaceId);
	std::string interfaceShortName = _AIInterface_Info_getValueByKey(interfaceId, AI_INTERFACE_PROPERTY_SHORT_NAME);
	std::string interfaceVersion = _AIInterface_Info_getValueByKey(interfaceId, AI_INTERFACE_PROPERTY_VERSION);
	std::string interfaceRelPath(AI_INTERFACES_DATA_DIR);
	interfaceRelPath += ps + interfaceShortName + ps + interfaceVersion + ps + relPath;

	exists = _DataDirs_Roots_locatePath(interfaceId, path, path_sizeMax, interfaceRelPath.c_str(), writeable, create, dir);

	return exists;
}
EXPORT(char*) _DataDirs_allocatePath(int interfaceId, const char* const relPath, bool writeable, bool create, bool dir) {

	static const unsigned int path_sizeMax = 2048;

	char* path = (char*) calloc(path_sizeMax, sizeof(char*));
	bool fetchOk = _DataDirs_locatePath(interfaceId, path, path_sizeMax, relPath, writeable, create, dir);

	if (!fetchOk) {
		free(path);
		path = NULL;
	}

	return path;
}
static std::vector<std::string> writeableDataDirs;
EXPORT(const char* const) _DataDirs_getWriteableDir(int interfaceId) {

	CHECK_INTERFACE_ID(interfaceId);

	// fill up writeableDataDirs until interfaceId index is in there
	// if it is not yet
	size_t wdd;
	for (wdd=writeableDataDirs.size(); wdd <= (size_t)interfaceId; ++wdd) {
		writeableDataDirs.push_back("");
	}
	if (writeableDataDirs[interfaceId].empty()) {
		static const unsigned int sizeMax = 1024;
		char tmpRes[sizeMax];
		static const char* const rootPath = "";
		bool exists = _DataDirs_locatePath(interfaceId,
				tmpRes, sizeMax, rootPath, true, true, true);
		writeableDataDirs[interfaceId] = tmpRes;
		if (!exists) {
			char errorMsg[sizeMax];
			SNPRINTF(errorMsg, sizeMax,
					"Unable to create writable data-dir for interface %i: %s",
					interfaceId, tmpRes);
			_Log_exception(interfaceId, errorMsg, 1, true);
			return NULL;
		}
	}

	return writeableDataDirs[interfaceId].c_str();
}


void SAIInterfaceCallback_init(struct SAIInterfaceCallback* callback) {

	callback->Engine_AIInterface_ABIVersion_getFailPart = &_Engine_AIInterface_ABIVersion_getFailPart;
	callback->Engine_AIInterface_ABIVersion_getWarningPart = &_Engine_AIInterface_ABIVersion_getWarningPart;
	callback->Engine_Version_getMajor = &_Engine_Version_getMajor;
	callback->Engine_Version_getMinor = &_Engine_Version_getMinor;
	callback->Engine_Version_getPatchset = &_Engine_Version_getPatchset;
	callback->Engine_Version_getAdditional = &_Engine_Version_getAdditional;
	callback->Engine_Version_getBuildTime = &_Engine_Version_getBuildTime;
	callback->Engine_Version_getNormal = &_Engine_Version_getNormal;
	callback->Engine_Version_getFull = &_Engine_Version_getFull;
	callback->AIInterface_Info_getSize = &_AIInterface_Info_getSize;
	callback->AIInterface_Info_getKey = &_AIInterface_Info_getKey;
	callback->AIInterface_Info_getValue = &_AIInterface_Info_getValue;
	callback->AIInterface_Info_getValueByKey = &_AIInterface_Info_getValueByKey;
	callback->Teams_getSize = &_Teams_getSize;
	callback->SkirmishAIs_getSize = &_SkirmishAIs_getSize;
	callback->SkirmishAIs_getMax = &_SkirmishAIs_getMax;
	callback->SkirmishAIs_Info_getValueByKey = &_SkirmishAIs_Info_getValueByKey;
	callback->Log_log = &_Log_log;
	callback->Log_exception = &_Log_exception;
	callback->DataDirs_getPathSeparator = &_DataDirs_getPathSeparator;
	callback->DataDirs_getConfigDir = &_DataDirs_getConfigDir;
	callback->DataDirs_getWriteableDir = &_DataDirs_getWriteableDir;
	callback->DataDirs_locatePath = &_DataDirs_locatePath;
	callback->DataDirs_allocatePath = &_DataDirs_allocatePath;
	callback->DataDirs_Roots_getSize = &_DataDirs_Roots_getSize;
	callback->DataDirs_Roots_getDir = &_DataDirs_Roots_getDir;
	callback->DataDirs_Roots_locatePath = &_DataDirs_Roots_locatePath;
	callback->DataDirs_Roots_allocatePath = &_DataDirs_Roots_allocatePath;
}

int SAIInterfaceCallback_getInstanceFor(const CAIInterfaceLibraryInfo* info, struct SAIInterfaceCallback* callback) {

	int interfaceId =-1;

	SAIInterfaceCallback_init(callback);

	size_t i;
	for (i = 0; i <  infos.size(); ++i) {
		if (infos[i] == info) {
			interfaceId = i;
			break;
		}
	}

	// if it was not yet inserted, do this now
	if (interfaceId == -1) {
		infos.push_back(info);
		interfaceId = infos.size()-1;
	}

	return interfaceId;
}
void SAIInterfaceCallback_release(int interfaceId) {

	CHECK_INTERFACE_ID(interfaceId);

	infos.erase(infos.begin() + interfaceId);
}
