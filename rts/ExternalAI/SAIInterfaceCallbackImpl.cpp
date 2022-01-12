/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdio>

#include "SAIInterfaceCallbackImpl.h"

#include "Game/GameVersion.h"
#include "Sim/Misc/GlobalConstants.h" // for MAX_TEAMS
#include "Sim/Misc/TeamHandler.h" // ActiveTeams()
#include "ExternalAI/AILibraryManager.h"
#include "ExternalAI/AIInterfaceLibraryInfo.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "ExternalAI/Interface/ELevelOfSupport.h"     // for ABI version
#include "ExternalAI/Interface/AISEvents.h"           // for ABI version
#include "ExternalAI/Interface/AISCommands.h"         // for ABI version
#include "ExternalAI/Interface/SSkirmishAILibrary.h"  // for ABI version
#include "ExternalAI/Interface/SAIInterfaceLibrary.h" // for ABI version and AI_INTERFACE_PROPERTY_*
#include "System/SafeCStrings.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/FileSystem/DataDirLocater.h"
#include "System/Log/ILog.h"

#include <vector>
#include <cstdlib> // malloc(), calloc(), free()
#include <sstream> // ostringstream
#include <cstring>


static const char* AI_INTERFACES_VERSION_COMMON = "common";

static std::vector<const CAIInterfaceLibraryInfo*> infos;

void CHECK_INTERFACE_ID(const int interfaceId)
{
	if (interfaceId < 0 || (size_t)interfaceId >= infos.size()) {
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
	return SpringVersion::GetMajor().c_str();
}
EXPORT(const char*) aiInterfaceCallback_Engine_Version_getMinor(int UNUSED_interfaceId) {
	return SpringVersion::GetMinor().c_str();
}
EXPORT(const char*) aiInterfaceCallback_Engine_Version_getPatchset(int UNUSED_interfaceId) {
	return SpringVersion::GetPatchSet().c_str();
}
EXPORT(const char*) aiInterfaceCallback_Engine_Version_getCommits(int UNUSED_interfaceId) {
	return SpringVersion::GetCommits().c_str();
}
EXPORT(const char*) aiInterfaceCallback_Engine_Version_getHash(int UNUSED_interfaceId) {
	return SpringVersion::GetHash().c_str();
}
EXPORT(const char*) aiInterfaceCallback_Engine_Version_getBranch(int UNUSED_interfaceId) {
	return SpringVersion::GetBranch().c_str();
}
EXPORT(const char*) aiInterfaceCallback_Engine_Version_getAdditional(int UNUSED_interfaceId) {
	return SpringVersion::GetAdditional().c_str();
}
EXPORT(const char*) aiInterfaceCallback_Engine_Version_getBuildTime(int UNUSED_interfaceId) {
	return "";
}
EXPORT(bool) aiInterfaceCallback_Engine_Version_isRelease(int UNUSED_interfaceId) {
	return SpringVersion::IsRelease();
}
EXPORT(const char*) aiInterfaceCallback_Engine_Version_getNormal(int UNUSED_interfaceId) {
	return SpringVersion::Get().c_str();
}
EXPORT(const char*) aiInterfaceCallback_Engine_Version_getSync(int UNUSED_interfaceId) {
	return SpringVersion::GetSync().c_str();
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
	return teamHandler.ActiveTeams();
}

EXPORT(int) aiInterfaceCallback_SkirmishAIs_getSize(int UNUSED_interfaceId) {
	return skirmishAIHandler.GetNumSkirmishAIs();
}

EXPORT(int) aiInterfaceCallback_SkirmishAIs_getMax(int UNUSED_interfaceId) {
	// TODO: should rather be something like (maxPlayers - numPlayers)
	return MAX_TEAMS;
}

EXPORT(const char*) aiInterfaceCallback_SkirmishAIs_Info_getValueByKey(
	int UNUSED_interfaceId,
	const char* const shortName,
	const char* const version,
	const char* const key
) {
	const char* value = "";

	const SkirmishAIKey aiKey(shortName, version);

	const AILibraryManager::T_skirmishAIInfos& skirmishInfos = AILibraryManager::GetInstance()->GetSkirmishAIInfos();
	const AILibraryManager::T_skirmishAIInfos::const_iterator inf = skirmishInfos.find(aiKey);

	if (inf == skirmishInfos.end())
		return value;

	const std::string& valueStr = (inf->second).GetInfo(key);

	if (valueStr.empty())
		return value;

	return (valueStr.c_str());
}

EXPORT(void) aiInterfaceCallback_Log_log(int interfaceId, const char* const msg) {
	CHECK_INTERFACE_ID(interfaceId);

	const CAIInterfaceLibraryInfo* info = infos[interfaceId];

	LOG("AI Interface <%s-%s>: %s", info->GetName().c_str(), info->GetVersion().c_str(), msg);
}

EXPORT(void) aiInterfaceCallback_Log_logsl(int interfaceId, const char* section, int loglevel, const char* const msg) {
	CHECK_INTERFACE_ID(interfaceId);

	log_frontend_record(loglevel, section, "%s", msg);
}


EXPORT(void) aiInterfaceCallback_Log_exception(int interfaceId, const char* const msg, int severety, bool die) {
	CHECK_INTERFACE_ID(interfaceId);

	const CAIInterfaceLibraryInfo* info = infos[interfaceId];

	LOG_L(L_ERROR, "AI Interface <%s-%s>: severety %i: [%s] %s",
		info->GetName().c_str(), info->GetVersion().c_str(), severety,
		(die ? "AI Interface shutting down" : "AI Interface still running"), msg);

	if (die) {
		// TODO: FIXME: unload all skirmish AIs of this interface plus the interface itsself
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
	return (dataDirLocater.GetDataDirPaths()).size();
}

EXPORT(bool) aiInterfaceCallback_DataDirs_Roots_getDir(int UNUSED_interfaceId, char* path, int pathMaxSize, int dirIndex) {
	const std::vector<std::string>& dds = dataDirLocater.GetDataDirPaths();
	size_t numDataDirs = dds.size();

	if (dirIndex >= 0 && (size_t)dirIndex < numDataDirs) {
		STRCPY_T(path, pathMaxSize, dds[dirIndex].c_str());
		return true;
	}

	return false;
}

EXPORT(bool) aiInterfaceCallback_DataDirs_Roots_locatePath(
	int UNUSED_interfaceId,
	char* path,
	int pathMaxSize,
	const char* const relPath,
	bool writeable,
	bool create,
	bool dir
) {
	int locateFlags = 0;

	if (writeable) {
		locateFlags = locateFlags | FileQueryFlags::WRITE;
		if (create) {
			locateFlags = locateFlags | FileQueryFlags::CREATE_DIRS;
		}
	}

	std::string locatedPath;
	std::vector<char> tmpRelPath(strlen(relPath) + 1);

	STRCPY_T(&tmpRelPath[0], tmpRelPath.size(), relPath);

	if (dir) {
		locatedPath = dataDirsAccess.LocateDir(&tmpRelPath[0], locateFlags);
	} else {
		locatedPath = dataDirsAccess.LocateFile(&tmpRelPath[0], locateFlags);
	}

	STRCPY_T(path, pathMaxSize, locatedPath.c_str());

	return (locatedPath != relPath);
}

EXPORT(char*) aiInterfaceCallback_DataDirs_Roots_allocatePath(int UNUSED_interfaceId, const char* const relPath, bool writeable, bool create, bool dir) {
	static const unsigned int pathMaxSize = 2048;

	// FIXME LEAK
	char* path = (char*) calloc(pathMaxSize, sizeof(char*));

	if (!aiInterfaceCallback_DataDirs_Roots_locatePath(-1, path, pathMaxSize, relPath, writeable, create, dir))
		FREE(path);

	return path;
}

EXPORT(const char*) aiInterfaceCallback_DataDirs_getConfigDir(int interfaceId) {
	CHECK_INTERFACE_ID(interfaceId);
	return infos[interfaceId]->GetDataDir().c_str();
}

EXPORT(bool) aiInterfaceCallback_DataDirs_locatePath(int interfaceId, char* path, int pathMaxSize, const char* const relPath, bool writeable, bool create, bool dir, bool common) {
	const char ps = aiInterfaceCallback_DataDirs_getPathSeparator(interfaceId);

	std::string interfaceShortName = aiInterfaceCallback_AIInterface_Info_getValueByKey(interfaceId, AI_INTERFACE_PROPERTY_SHORT_NAME);
	std::string interfaceVersion;

	if (common) {
		interfaceVersion = AI_INTERFACES_VERSION_COMMON;
	} else {
		interfaceVersion = aiInterfaceCallback_AIInterface_Info_getValueByKey(interfaceId, AI_INTERFACE_PROPERTY_VERSION);
	}

	std::string interfaceRelPath(AI_INTERFACES_DATA_DIR);

	interfaceRelPath += (ps + interfaceShortName);
	interfaceRelPath += (ps + interfaceVersion);
	interfaceRelPath += (ps + std::string(relPath));

	return aiInterfaceCallback_DataDirs_Roots_locatePath(interfaceId, path, pathMaxSize, interfaceRelPath.c_str(), writeable, create, dir);
}

EXPORT(char*) aiInterfaceCallback_DataDirs_allocatePath(int interfaceId, const char* const relPath, bool writeable, bool create, bool dir, bool common) {
	static const unsigned int pathMaxSize = 2048;

	// FIXME LEAK
	char* path = (char*) calloc(pathMaxSize, sizeof(char*));

	if (!aiInterfaceCallback_DataDirs_locatePath(interfaceId, path, pathMaxSize, relPath, writeable, create, dir, common))
		FREE(path);

	return path;
}


static std::vector<std::string> writeableDataDirs;

EXPORT(const char*) aiInterfaceCallback_DataDirs_getWriteableDir(int interfaceId) {
	CHECK_INTERFACE_ID(interfaceId);

	// fill up writeableDataDirs until interfaceId index is in there
	// if it is not yet
	for (size_t wdd = writeableDataDirs.size(); wdd <= (size_t)interfaceId; ++wdd)
		writeableDataDirs.emplace_back("");

	if (writeableDataDirs[interfaceId].empty()) {
		char tmpRes[1024];

		const bool exists = aiInterfaceCallback_DataDirs_locatePath(interfaceId,
				tmpRes, sizeof(tmpRes), "", true, true, true, false);

		writeableDataDirs[interfaceId] = tmpRes;

		if (!exists) {
			char errorMsg[1086];

			SNPRINTF(errorMsg, sizeof(errorMsg),
				"Unable to create writable data-dir for interface %i: %s",
				interfaceId, tmpRes);

			aiInterfaceCallback_Log_exception(interfaceId, errorMsg, 1, true);
			return nullptr;
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
	callback->Engine_Version_getCommits = &aiInterfaceCallback_Engine_Version_getCommits;
	callback->Engine_Version_getHash = &aiInterfaceCallback_Engine_Version_getHash;
	callback->Engine_Version_getBranch = &aiInterfaceCallback_Engine_Version_getBranch;
	callback->Engine_Version_getAdditional = &aiInterfaceCallback_Engine_Version_getAdditional;
	callback->Engine_Version_getBuildTime = &aiInterfaceCallback_Engine_Version_getBuildTime;
	callback->Engine_Version_isRelease = &aiInterfaceCallback_Engine_Version_isRelease;
	callback->Engine_Version_getNormal = &aiInterfaceCallback_Engine_Version_getNormal;
	callback->Engine_Version_getSync = &aiInterfaceCallback_Engine_Version_getSync;
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
	callback->Log_logsl = &aiInterfaceCallback_Log_logsl;
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
