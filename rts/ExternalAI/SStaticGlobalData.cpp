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

#include "Interface/SStaticGlobalData.h"

#include <stdlib.h>	// malloc, calloc, free
#include <string.h>	// strcpy

#if defined	__cplusplus && !defined BUILDING_AI

#include "Sim/Misc/GlobalConstants.h"        // for MAX_TEAMS
#include "Game/GameVersion.h"                // for VERSION_STRING
#include "FileSystem/FileSystem.h"           // for data directories
#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/SkirmishAIKey.h"
#include "IAILibraryManager.h"               // for AI info

#include <string>

/**
 * Appends '/' to inPath if it is not empty and does not yet have a '/'
 * or a '\\'at the end.
 */
static std::string ensureNoPathSeparatorAtTail(const std::string& inPath) {

	std::string outPath = inPath;

	if (!outPath.empty()) {
		char lastChar = outPath.at(outPath.size()-1);
		if (lastChar == '/' || lastChar == '\\') {
			outPath.erase(outPath.size()-1, 1);
		}
	}

	return outPath;
}

struct SStaticGlobalData* createStaticGlobalData() {

	const std::vector<std::string> dds =
			FileSystemHandler::GetInstance().GetDataDirectories();
	unsigned int numDataDirs = dds.size();

	// converting the data-dirs vector to a C strings array (char*[])
	char** dataDirs = (char**) calloc(numDataDirs, sizeof(char*));
	unsigned int i;
	for (i=0; i < numDataDirs; ++i) {
		std::string cleanPath = ensureNoPathSeparatorAtTail(dds.at(i));
		dataDirs[i] = (char*) calloc(cleanPath.size()+1, sizeof(char));
		strcpy(dataDirs[i], cleanPath.c_str());
	}

	IAILibraryManager* aiLibMan = IAILibraryManager::GetInstance();

	// fetch infos for the Skirmish AI libraries that will be used
	// in the current game
	const IAILibraryManager::T_skirmishAIInfos& sAIInfos =
			aiLibMan->GetUsedSkirmishAIInfos();
	IAILibraryManager::T_skirmishAIInfos::const_iterator sInfs;
	unsigned int numSkirmishAIs = sAIInfos.size();
	unsigned int* skirmishAIInfosSizes =
			(unsigned int*) calloc(numSkirmishAIs, sizeof(unsigned int));
	const char*** skirmishAIInfosKeys =
			(const char***) calloc(numSkirmishAIs, sizeof(char**));
	const char*** skirmishAIInfosValues =
			(const char***) calloc(numSkirmishAIs, sizeof(char**));
	for (i=0, sInfs=sAIInfos.begin(); sInfs != sAIInfos.end(); ++i, ++sInfs) {
		skirmishAIInfosSizes[i] = sInfs->second->GetInfo().size();
		skirmishAIInfosKeys[i] = sInfs->second->GetCInfoKeys();
		skirmishAIInfosValues[i] = sInfs->second->GetCInfoValues();
	}

	const SStaticGlobalData sgd = {
		MAX_TEAMS,
		SpringVersion::GetFull().c_str(),	// spring version string
		numDataDirs,
		(const char**) dataDirs,
		numSkirmishAIs,
		skirmishAIInfosSizes,
		skirmishAIInfosKeys,
		skirmishAIInfosValues,
	};

	SStaticGlobalData* staticGlobalData = (struct SStaticGlobalData*)
			malloc(sizeof(struct SStaticGlobalData));
	*staticGlobalData = sgd;

	return staticGlobalData;
}

void freeStaticGlobalData(struct SStaticGlobalData* staticGlobalData) {

	// free the data dirs
	for(unsigned int i=0; i < staticGlobalData->numDataDirs; ++i) {
		free(const_cast<char*>(staticGlobalData->dataDirs[i]));
		staticGlobalData->dataDirs[i] = NULL;
	}
	free(const_cast<char**>(staticGlobalData->dataDirs));
	staticGlobalData->dataDirs = NULL;

	// free the skirmish AI infos
	free(const_cast<unsigned int*>(staticGlobalData->skirmishAIInfosSizes));
	staticGlobalData->skirmishAIInfosSizes = NULL;
	free(const_cast<char***>(staticGlobalData->skirmishAIInfosKeys));
	staticGlobalData->skirmishAIInfosKeys = NULL;
	free(const_cast<char***>(staticGlobalData->skirmishAIInfosValues));
	staticGlobalData->skirmishAIInfosValues = NULL;

	free(staticGlobalData);
	staticGlobalData = NULL;
}

#endif // defined __cplusplus && !defined BUILDING_AI
