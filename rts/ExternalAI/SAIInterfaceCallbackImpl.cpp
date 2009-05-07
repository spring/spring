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

#include <cstdio>

using std::sprintf;

#include "SAIInterfaceCallbackImpl.h"

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
#include "Interface/SAIFloat3.h"           // for ABI version
#include "Interface/AISEvents.h"           // for ABI version
#include "Interface/AISCommands.h"         // for ABI version
#include "Interface/SSkirmishAILibrary.h"  // for ABI version
#include "Interface/SAIInterfaceLibrary.h" // for ABI version and AI_INTERFACE_PROPERTY_*

#include <vector>
#include <stdlib.h> // malloc(), calloc(), free()


static const char* AI_INTERFACES_VERSION_COMMON = "common";

static std::vector<const CAIInterfaceLibraryInfo*> infos;

void CHECK_INTERFACE_ID(const int interfaceId)
{
	if (interfaceId < 0 || (size_t)interfaceId >= infos.size())
	{
		std::ostringstream buf;
		buf << "Bad AI Interface ID supplied by an interface.\n";
		buf << "Is " << interfaceId << ", but should be between min 0 and max " << infos.size() << ".";
		aiInterfaceCallback_Log_exception(interfaceId, buf.str().c_str(), 1, true);
	}
}


EXPORT(int) aiInterfaceCallback_Engine_AIInterface_ABIVersion_getFailPart(int UNUSED_interfaceId) {
	return AIINTERFACE_ABI_VERSION_FAIL;
}
EXPORT(int) aiInterfaceCallback_Engine_AIInterface_ABIVersion_getWarningPart(int UNUSED_interfaceId) {
	return AIINTERFACE_ABI_VERSION_WARNING;
}

EXPORT(const char*) aiInterfaceCallback_Engine_Version_getMajor(int UNUSED_interfaceId) {
	return SpringVersion::Major;
}
EXPORT(const char*) aiInterfaceCallback_Engine_Version_getMinor(int UNUSED_interfaceId) {
	return SpringVersion::Minor;
}
EXPORT(const char*) aiInterfaceCallback_Engine_Version_getPatchset(int UNUSED_interfaceId) {
	return SpringVersion::Patchset;
}
EXPORT(const char*) aiInterfaceCallback_Engine_Version_getAdditional(int UNUSED_interfaceId) {
	return SpringVersion::Additional;
}
EXPORT(const char*) aiInterfaceCallback_Engine_Version_getBuildTime(int UNUSED_interfaceId) {
	return SpringVersion::BuildTime;
}
EXPORT(const char*) aiInterfaceCallback_Engine_Version_getNormal(int UNUSED_interfaceId) {
	return SpringVersion::Get().c_str();
}
EXPORT(const char*) aiInterfaceCallback_Engine_Version_getFull(int UNUSED_interfaceId) {
	return SpringVersion::GetFull().c_str();
}




EXPORT(int) aiInterfaceCallback_AIInterface_Info_getSize(int interfaceId) {

	CHECK_INTERFACE_ID(interfaceId);

	const CAIInterfaceLibraryInfo* info = infos[interfaceId];
	return (int)info->size();
}
EXPORT(const char*) aiInterfaceCallback_AIInterface_Info_getKey(int interfaceId, int infoIndex) {

	CHECK_INTERFACE_ID(interfaceId);

	const CAIInterfaceLibraryInfo* info = infos[interfaceId];
	return info->GetKeyAt(infoIndex).c_str();
}
EXPORT(const char*) aiInterfaceCallback_AIInterface_Info_getValue(int interfaceId, int infoIndex) {

	CHECK_INTERFACE_ID(interfaceId);

	const CAIInterfaceLibraryInfo* info = infos[interfaceId];
	return info->GetValueAt(infoIndex).c_str();
}
EXPORT(const char*) aiInterfaceCallback_AIInterface_Info_getDescription(int interfaceId, int infoIndex) {

	CHECK_INTERFACE_ID(interfaceId);

	const CAIInterfaceLibraryInfo* info = infos[interfaceId];
	return info->GetDescriptionAt(infoIndex).c_str();
}
EXPORT(const char*) aiInterfaceCallback_AIInterface_Info_getValueByKey(int interfaceId, const char* const key) {

	CHECK_INTERFACE_ID(interfaceId);

	const CAIInterfaceLibraryInfo* info = infos[interfaceId];
	return info->GetInfo(key).c_str();
}

EXPORT(int) aiInterfaceCallback_Teams_getSize(int UNUSED_interfaceId) {
	return teamHandler->ActiveTeams();
}

EXPORT(int) aiInterfaceCallback_SkirmishAIs_getSize(int UNUSED_interfaceId) {
	return gameSetup->GetSkirmishAIs();
}
EXPORT(int) aiInterfaceCallback_SkirmishAIs_getMax(int UNUSED_interfaceId) {
	return MAX_TEAMS;
}
EXPORT(const char*) aiInterfaceCallback_SkirmishAIs_Info_getValueByKey(int UNUSED_interfaceId, const char* const shortName, const char* const version, const char* const key) {

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

EXPORT(void) aiInterfaceCallback_Log_log(int interfaceId, const char* const msg) {

	CHECK_INTERFACE_ID(interfaceId);

	const CAIInterfaceLibraryInfo* info = infos[interfaceId];
	logOutput.Print("AI Interface <%s-%s>: %s", info->GetName().c_str(), info->GetVersion().c_str(), msg);
}
EXPORT(void) aiInterfaceCallback_Log_exception(int interfaceId, const char* const msg, int severety, bool die) {

	CHECK_INTERFACE_ID(interfaceId);

	const CAIInterfaceLibraryInfo* info = infos[interfaceId];
	logOutput.Print("AI Interface <%s-%s>: error, severety %i: [%s] %s",
			info->GetName().c_str(), info->GetVersion().c_str(), severety,
			(die ? "AI Interface shutting down" : "AI Interface still running"), msg);
	if (die) {
		// TODO: FIXME: unload all skirmish AIs of this interface plus the interface itsself
// 		std::vector<int> teamIds = IAILibraryManager::GetInstance()->GetAllTeamIdsAccociatedWithInterface(info->GetKey());
// 		std::vector<int>::const_iterator teamId;
// 		for (teamId = teamIds.begin(); teamId != teamIds.end(); ++teamId) {
// 			eoh->DestroySkirmishAI(*teamId);
// 		}
	}
}
EXPORT(char) aiInterfaceCallback_DataDirs_getPathSeparator(int UNUSED_interfaceId) {
#ifdef _WIN32
	return '\\';
#else
	return '/';
#endif
}
EXPORT(int) aiInterfaceCallback_DataDirs_Roots_getSize(int UNUSED_interfaceId) {

	const std::vector<std::string> dds =
			FileSystemHandler::GetInstance().GetDataDirectories();
	return dds.size();
}
EXPORT(bool) aiInterfaceCallback_DataDirs_Roots_getDir(int UNUSED_interfaceId, char* path, int path_sizeMax, int dirIndex) {

	const std::vector<std::string> dds =
			FileSystemHandler::GetInstance().GetDataDirectories();
	size_t numDataDirs = dds.size();
	if (dirIndex >= 0 && (size_t)dirIndex < numDataDirs) {
		STRCPYS(path, path_sizeMax, dds[dirIndex].c_str());
		return true;
	} else {
		return false;
	}
}
EXPORT(bool) aiInterfaceCallback_DataDirs_Roots_locatePath(int UNUSED_interfaceId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir) {

	bool exists = false;

	int locateFlags = 0;
	if (writeable) {
		locateFlags = locateFlags | FileSystem::WRITE;
		if (create) {
			locateFlags = locateFlags | FileSystem::CREATE_DIRS;
		}
	}
	std::string locatedPath = "";
	const size_t tmpRelPath_size = strlen(relPath) + 1;
	char* tmpRelPath = new char[tmpRelPath_size];
	STRCPYS(tmpRelPath, tmpRelPath_size, relPath);
	std::string tmpRelPathStr = tmpRelPath;
	if (dir) {
		locatedPath = filesystem.LocateDir(tmpRelPathStr, locateFlags);
	} else {
		locatedPath = filesystem.LocateFile(tmpRelPathStr, locateFlags);
	}
	exists = (locatedPath != relPath);
	STRCPYS(path, path_sizeMax, locatedPath.c_str());

	delete [] tmpRelPath;
	return exists;
}
EXPORT(char*) aiInterfaceCallback_DataDirs_Roots_allocatePath(int UNUSED_interfaceId, const char* const relPath, bool writeable, bool create, bool dir) {

	static const unsigned int path_sizeMax = 2048;

	char* path = (char*) calloc(path_sizeMax, sizeof(char*));
	bool fetchOk = aiInterfaceCallback_DataDirs_Roots_locatePath(-1, path, path_sizeMax, relPath, writeable, create, dir);

	if (!fetchOk) {
		FREE(path);
	}

	return path;
}
EXPORT(const char*) aiInterfaceCallback_DataDirs_getConfigDir(int interfaceId) {

	CHECK_INTERFACE_ID(interfaceId);

	const CAIInterfaceLibraryInfo* info = infos[interfaceId];
	return info->GetDataDir().c_str();
}
EXPORT(bool) aiInterfaceCallback_DataDirs_locatePath(int interfaceId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir, bool common) {

	bool exists = false;

	char ps = aiInterfaceCallback_DataDirs_getPathSeparator(interfaceId);
	std::string interfaceShortName = aiInterfaceCallback_AIInterface_Info_getValueByKey(interfaceId, AI_INTERFACE_PROPERTY_SHORT_NAME);
	std::string interfaceVersion;
	if (common) {
		interfaceVersion = AI_INTERFACES_VERSION_COMMON;
	} else {
		interfaceVersion = aiInterfaceCallback_AIInterface_Info_getValueByKey(interfaceId, AI_INTERFACE_PROPERTY_VERSION);
	}
	std::string interfaceRelPath(AI_INTERFACES_DATA_DIR);
	interfaceRelPath += ps + interfaceShortName + ps + interfaceVersion + ps + relPath;

	exists = aiInterfaceCallback_DataDirs_Roots_locatePath(interfaceId, path, path_sizeMax, interfaceRelPath.c_str(), writeable, create, dir);

	return exists;
}
EXPORT(char*) aiInterfaceCallback_DataDirs_allocatePath(int interfaceId, const char* const relPath, bool writeable, bool create, bool dir, bool common) {

	static const unsigned int path_sizeMax = 2048;

	char* path = (char*) calloc(path_sizeMax, sizeof(char*));
	bool fetchOk = aiInterfaceCallback_DataDirs_locatePath(interfaceId, path, path_sizeMax, relPath, writeable, create, dir, common);

	if (!fetchOk) {
		FREE(path);
	}

	return path;
}
static std::vector<std::string> writeableDataDirs;
EXPORT(const char*) aiInterfaceCallback_DataDirs_getWriteableDir(int interfaceId) {

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
		bool exists = aiInterfaceCallback_DataDirs_locatePath(interfaceId,
				tmpRes, sizeMax, rootPath, true, true, true, false);
		writeableDataDirs[interfaceId] = tmpRes;
		if (!exists) {
			char errorMsg[sizeMax];
			SNPRINTF(errorMsg, sizeMax,
					"Unable to create writable data-dir for interface %i: %s",
					interfaceId, tmpRes);
			aiInterfaceCallback_Log_exception(interfaceId, errorMsg, 1, true);
			return NULL;
		}
	}

	return writeableDataDirs[interfaceId].c_str();
}


static void aiInterfaceCallback_init(struct SAIInterfaceCallback* callback) {

	callback->Engine_AIInterface_ABIVersion_getFailPart = &aiInterfaceCallback_Engine_AIInterface_ABIVersion_getFailPart;
	callback->Engine_AIInterface_ABIVersion_getWarningPart = &aiInterfaceCallback_Engine_AIInterface_ABIVersion_getWarningPart;
	callback->Engine_Version_getMajor = &aiInterfaceCallback_Engine_Version_getMajor;
	callback->Engine_Version_getMinor = &aiInterfaceCallback_Engine_Version_getMinor;
	callback->Engine_Version_getPatchset = &aiInterfaceCallback_Engine_Version_getPatchset;
	callback->Engine_Version_getAdditional = &aiInterfaceCallback_Engine_Version_getAdditional;
	callback->Engine_Version_getBuildTime = &aiInterfaceCallback_Engine_Version_getBuildTime;
	callback->Engine_Version_getNormal = &aiInterfaceCallback_Engine_Version_getNormal;
	callback->Engine_Version_getFull = &aiInterfaceCallback_Engine_Version_getFull;
	callback->AIInterface_Info_getSize = &aiInterfaceCallback_AIInterface_Info_getSize;
	callback->AIInterface_Info_getKey = &aiInterfaceCallback_AIInterface_Info_getKey;
	callback->AIInterface_Info_getValue = &aiInterfaceCallback_AIInterface_Info_getValue;
	callback->AIInterface_Info_getDescription = &aiInterfaceCallback_AIInterface_Info_getDescription;
	callback->AIInterface_Info_getValueByKey = &aiInterfaceCallback_AIInterface_Info_getValueByKey;
	callback->Teams_getSize = &aiInterfaceCallback_Teams_getSize;
	callback->SkirmishAIs_getSize = &aiInterfaceCallback_SkirmishAIs_getSize;
	callback->SkirmishAIs_getMax = &aiInterfaceCallback_SkirmishAIs_getMax;
	callback->SkirmishAIs_Info_getValueByKey = &aiInterfaceCallback_SkirmishAIs_Info_getValueByKey;
	callback->Log_log = &aiInterfaceCallback_Log_log;
	callback->Log_exception = &aiInterfaceCallback_Log_exception;
	callback->DataDirs_getPathSeparator = &aiInterfaceCallback_DataDirs_getPathSeparator;
	callback->DataDirs_getConfigDir = &aiInterfaceCallback_DataDirs_getConfigDir;
	callback->DataDirs_getWriteableDir = &aiInterfaceCallback_DataDirs_getWriteableDir;
	callback->DataDirs_locatePath = &aiInterfaceCallback_DataDirs_locatePath;
	callback->DataDirs_allocatePath = &aiInterfaceCallback_DataDirs_allocatePath;
	callback->DataDirs_Roots_getSize = &aiInterfaceCallback_DataDirs_Roots_getSize;
	callback->DataDirs_Roots_getDir = &aiInterfaceCallback_DataDirs_Roots_getDir;
	callback->DataDirs_Roots_locatePath = &aiInterfaceCallback_DataDirs_Roots_locatePath;
	callback->DataDirs_Roots_allocatePath = &aiInterfaceCallback_DataDirs_Roots_allocatePath;
}

int aiInterfaceCallback_getInstanceFor(const CAIInterfaceLibraryInfo* info, struct SAIInterfaceCallback* callback) {

	int interfaceId = -1;

	aiInterfaceCallback_init(callback);

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
void aiInterfaceCallback_release(int interfaceId) {

	CHECK_INTERFACE_ID(interfaceId);

	infos.erase(infos.begin() + interfaceId);
}
