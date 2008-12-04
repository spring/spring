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

#if defined	__cplusplus && !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE
#include "Sim/Misc/GlobalConstants.h"			// for MAX_TEAMS
#define MAX_GROUPS	10	// 0..9
#include "Game/GameVersion.h"			// for VERSION_STRING
#include "FileSystem/FileSystem.h"	// for data directories
#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/SkirmishAIKey.h"
#include "IAILibraryManager.h"	// for AI info

#include "LogOutput.h" // TODO: remove again

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

//#include <stdio.h>	// for file IO
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
	const IAILibraryManager::T_skirmishAIInfos& sAIInfos = aiLibMan->GetUsedSkirmishAIInfos();
	IAILibraryManager::T_skirmishAIInfos::const_iterator sInfs;
	unsigned int numSkirmishAIs = sAIInfos.size();
//	unsigned int* numsSkirmishAIInfo = (unsigned int*) calloc(numSkirmishAIs, sizeof(unsigned int));
//	const InfoItem** skirmishAIInfos = (const InfoItem**) calloc(numSkirmishAIs, sizeof(InfoItem*));
//	for (i=0, sInfs=sAIInfos.begin(); sInfs != sAIInfos.end(); ++i, ++sInfs) {
//		numsSkirmishAIInfo[i] = sInfs->second->GetInfo().size();
////		skirmishAIInfos[i] = (InfoItem*) calloc(numsSkirmishAIInfo[i], sizeof(InfoItem));
////		numsSkirmishAIInfo[i] = sInfs->second->GetInfoCReference(skirmishAIInfos[i], numsSkirmishAIInfo[i]);
//		skirmishAIInfos[i] = sInfs->second->GetInfoCReference();
////unsigned int x;
////for (x=0; x < numsSkirmishAIInfo[i]; ++x) {
////logOutput.Print("SStaticGlobalData.cpp skirmishAI[%u].info[%u] key / value: %s / %s", i, x, skirmishAIInfos[i][x].key, skirmishAIInfos[i][x].value);
////}
//	}
	unsigned int* skirmishAIInfosSizes = (unsigned int*) calloc(numSkirmishAIs, sizeof(unsigned int));
	const char*** skirmishAIInfosKeys = (const char***) calloc(numSkirmishAIs, sizeof(char**));
	const char*** skirmishAIInfosValues = (const char***) calloc(numSkirmishAIs, sizeof(char**));
	for (i=0, sInfs=sAIInfos.begin(); sInfs != sAIInfos.end(); ++i, ++sInfs) {
		skirmishAIInfosSizes[i] = sInfs->second->GetInfo().size();
		skirmishAIInfosKeys[i] = sInfs->second->GetCInfoKeys();
		skirmishAIInfosValues[i] = sInfs->second->GetCInfoValues();
	}

//	// fetch infos for the Group AI libraries that will be used
//	// in the current game
//	const IAILibraryManager::T_groupAIInfos& gAIInfos = aiLibMan->GetGroupAIInfos();
//	unsigned int numGroupAIs = gAIInfos.size();
//	unsigned int* numsGroupAIInfo = (unsigned int*) calloc(numGroupAIs, sizeof(unsigned int));
//	InfoItem** groupAIInfos = (InfoItem**) calloc(numGroupAIs, sizeof(InfoItem*));
//	IAILibraryManager::T_groupAIInfos::const_iterator gInfs;
//	for (i=0, gInfs=gAIInfos.begin(); gInfs != gAIInfos.end(); ++i, ++gInfs) {
//		numsGroupAIInfo[i] = gInfs->second->GetInfo()->size();
//		groupAIInfos[i] = (InfoItem*) calloc(numsGroupAIInfo[i], sizeof(InfoItem));
//		numsGroupAIInfo[i] = gInfs->second->GetInfoCReference(groupAIInfos[i], numsGroupAIInfo[i]);
//	}

	const SStaticGlobalData sgd = {
		MAX_TEAMS,
		MAX_GROUPS,
		SpringVersion::GetFull().c_str(),	// spring version string
		numDataDirs,
		(const char**) dataDirs,
		numSkirmishAIs,
//		numsSkirmishAIInfo,
//		(const InfoItem**) skirmishAIInfos,
		skirmishAIInfosSizes,
		skirmishAIInfosKeys,
		skirmishAIInfosValues,
//		numGroupAIs,
//		numsGroupAIInfo,
//		(const InfoItem**) groupAIInfos
	};

	SStaticGlobalData* staticGlobalData = (struct SStaticGlobalData*)
			malloc(sizeof(struct SStaticGlobalData));
	*staticGlobalData = sgd;

	return staticGlobalData;
}

void freeStaticGlobalData(struct SStaticGlobalData* staticGlobalData) {

	//free(const_cast<char*>(staticGlobalData->springVersion));

	// free the data dirs
	for(unsigned int i=0; i < staticGlobalData->numDataDirs; ++i) {
		free(const_cast<char*>(staticGlobalData->dataDirs[i]));
	}
	free(const_cast<char**>(staticGlobalData->dataDirs));

	// free the skirmish AI infos
//	// we only hold refs to the Infos in CSkirmishAILibraryInfo,
//	// therefore, we do not call freeInfoItem()
//	for(unsigned int i=0; i < staticGlobalData->numSkirmishAIs; ++i) {
//		free(const_cast<struct InfoItem*>(staticGlobalData->skirmishAIInfos[i]));
//	}
//	free(const_cast<unsigned int*>(staticGlobalData->numsSkirmishAIInfo));
//	free(const_cast<struct InfoItem**>(staticGlobalData->skirmishAIInfos));
	free(const_cast<unsigned int*>(staticGlobalData->skirmishAIInfosSizes));
	free(const_cast<char***>(staticGlobalData->skirmishAIInfosKeys));
	free(const_cast<char***>(staticGlobalData->skirmishAIInfosValues));

//	// free the group AI infos
//	// we only hold refs to the Infos in CGroupAILibraryInfo,
//	// therefore, we do not call freeInfoItem()
//	for(unsigned int i=0; i < staticGlobalData->numGroupAIs; ++i) {
//		free(const_cast<struct InfoItem*>(staticGlobalData->groupAIInfos[i]));
//	}
//	free(const_cast<unsigned int*>(staticGlobalData->numsGroupAIInfo));
//	free(const_cast<struct InfoItem**>(staticGlobalData->groupAIInfos));

	free(staticGlobalData);
	staticGlobalData = NULL;
}

#endif // defined __cplusplus && !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE */
