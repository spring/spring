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
#include "Game/GlobalConstants.h"			// for MAX_TEAMS
#define MAX_GROUPS	10	// 0..9
#include "Game/GameVersion.h"			// for VERSION_STRING
#include "System/Platform/FileSystem.h"	// for data directories
#include "ExternalAI/IAILibraryManager.h"
#include "IAILibraryManager.h"	// for AI info

#include <string>

/**
 * Appends '/' to inPath if it is not empty and does not yet have a '/'
 * or a '\\'at the end.
 */
std::string ensureNoPathSeparatorAtTail(const std::string& inPath) {
	
	std::string outPath = inPath;
	
	if (!outPath.empty()) {
		char lastChar = outPath.at(outPath.size()-1);
		if (lastChar == '/' || lastChar == '\\') {
			outPath.erase(outPath.size()-1, 1);
		}
	}
	
	return outPath;
}

#include <stdio.h>	// for file IO
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
	const IAILibraryManager::T_skirmishAIInfos* sAIInfos = aiLibMan->GetUsedSkirmishAIInfos();
	unsigned int numSkirmishAIs = sAIInfos->size();
	unsigned int* numsSkirmishAIInfo = (unsigned int*) calloc(numSkirmishAIs, sizeof(unsigned int));
	InfoItem** skirmishAIInfos = (InfoItem**) calloc(numSkirmishAIs, sizeof(InfoItem*));
	IAILibraryManager::T_skirmishAIInfos::const_iterator sInfs;
	for (i=0, sInfs=sAIInfos->begin(); sInfs != sAIInfos->end(); ++i, ++sInfs) {
		numsSkirmishAIInfo[i] = sInfs->second->GetInfo()->size();
		skirmishAIInfos[i] = (InfoItem*) calloc(numsSkirmishAIInfo[i], sizeof(InfoItem));
		numsSkirmishAIInfo[i] = sInfs->second->GetInfoCReference(skirmishAIInfos[i], numsSkirmishAIInfo[i]);
	}
	
	// fetch infos for the Group AI libraries that will be used
	// in the current game
	const IAILibraryManager::T_groupAIInfos* gAIInfos = aiLibMan->GetGroupAIInfos();
	unsigned int numGroupAIs = gAIInfos->size();
	unsigned int* numsGroupAIInfo = (unsigned int*) calloc(numGroupAIs, sizeof(unsigned int));
	InfoItem** groupAIInfos = (InfoItem**) calloc(numGroupAIs, sizeof(InfoItem*));
	IAILibraryManager::T_groupAIInfos::const_iterator gInfs;
	for (i=0, gInfs=gAIInfos->begin(); gInfs != gAIInfos->end(); ++i, ++gInfs) {
		numsGroupAIInfo[i] = gInfs->second->GetInfo()->size();
		groupAIInfos[i] = (InfoItem*) calloc(numsGroupAIInfo[i], sizeof(InfoItem));
		numsGroupAIInfo[i] = gInfs->second->GetInfoCReference(groupAIInfos[i], numsGroupAIInfo[i]);
	}
	
	const SStaticGlobalData sgd = {
		MAX_TEAMS,
		MAX_GROUPS,
		VERSION_STRING,	// spring version string
		numDataDirs,
		(const char**) dataDirs,
		numSkirmishAIs,
		numsSkirmishAIInfo,
		(const InfoItem**) skirmishAIInfos,
		numGroupAIs,
		numsGroupAIInfo,
		(const InfoItem**) groupAIInfos
	};
	
	SStaticGlobalData* staticGlobalData = (struct SStaticGlobalData*)
			malloc(sizeof(struct SStaticGlobalData));
	*staticGlobalData = sgd;
	
	return staticGlobalData;
}

void freeStaticGlobalData(struct SStaticGlobalData* staticGlobalData) {
	
	free(const_cast<char*>(staticGlobalData->springVersion));
	for(unsigned int i=0; i < staticGlobalData->numDataDirs; ++i)
		free(const_cast<char*>(staticGlobalData->dataDirs[i]));
	free(const_cast<char**>(staticGlobalData->dataDirs));
	free(staticGlobalData);
	staticGlobalData = NULL;
}

#endif	/* defined	__cplusplus && !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE */
