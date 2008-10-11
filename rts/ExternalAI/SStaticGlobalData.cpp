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

#include <string>


#include "Interface/SStaticGlobalData.h"

#if defined	__cplusplus && !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE
#include "System/GlobalStuff.h" // for MAX_TEAMS
#include "System/Platform/FileSystem.h" // for data directories
#include "Game/GameVersion.h" // for VERSION_STRING

#ifdef WIN32
//#include <windows.h>	// filesystem operations
#include <direct.h>	// filesystem operations
#else
#include <unistd.h>	// filesystem operations
#endif
#include <stdlib.h>	// malloc, calloc, free
#include <string.h>	// strcpy

bool getCWD(char* cwdPath, unsigned int maxLength) {

	bool success = false;
	//#ifdef WIN32
	//	unsigned int cwdLength = GetCurrentDirectory(maxLength, cwdPath);
	//	if (cwdLength > 0 && cwdLength <= maxLength) {
	//		success = true;
	//	}
	//#else	/* WIN32 */
		const char* ret = getcwd(cwdPath, maxLength);
		if (ret != NULL) {
			success = true;
		}
	//#endif	/* WIN32 */

	if (!success) {
		cwdPath[0] = '\0'; // make the path an empty string
		//cwdPath = "./";
	}

	return success;
}

/**
 * Appends '/' to inPath if it is not empty and does not yet have a '/'
 * or a '\\'at the end.
 */
std::string ensurePathHasNoSlashAtTail(const std::string& inPath) {
	
	std::string outPath = inPath;
	
	if (!outPath.empty()) {
		char lastChar = outPath.at(outPath.size()-1);
		if (lastChar == '/' || lastChar == '\\') {
			outPath.erase(outPath.size()-1, 1);
		}
	}
	
	return outPath;
}

SStaticGlobalData* createStaticGlobalData() {
	
	char tmpPath[1024];
	getCWD(tmpPath, sizeof(tmpPath));
	std::string cleanCWD = ensurePathHasNoSlashAtTail(tmpPath);
	char* cwdPath = (char*) calloc(cleanCWD.size()+1, sizeof(char));
	strcpy(cwdPath, cleanCWD.c_str());

	const std::vector<std::string> dds = FileSystemHandler::GetInstance().GetDataDirectories();
	unsigned int numDataDirs = dds.size();
	char** dataDirs = (char**) calloc(numDataDirs, sizeof(char*));
	for (unsigned int i=0; i < numDataDirs; ++i) {
		std::string cleanPath = ensurePathHasNoSlashAtTail(dds.at(i));
		dataDirs[i] = (char*) calloc(cleanPath.size()+1, sizeof(char));
		strcpy(dataDirs[i], cleanPath.c_str());
	}

	const SStaticGlobalData sgd = {
		MAX_TEAMS,
		VERSION_STRING,	// spring version string
		cwdPath,		// libDir
		numDataDirs,
		(const char**) dataDirs};
	SStaticGlobalData* staticGlobalData = (struct SStaticGlobalData*) malloc(sizeof(struct SStaticGlobalData));
	*staticGlobalData = sgd;
	
	return staticGlobalData;
}

void freeStaticGlobalData(SStaticGlobalData* staticGlobalData) {
	
	free(const_cast<char*>(staticGlobalData->springVersion));
	free(const_cast<char*>(staticGlobalData->libDir));
	for(unsigned int i=0; i < staticGlobalData->numDataDirs; ++i)
		free(const_cast<char*>(staticGlobalData->dataDirs[i]));
	free(const_cast<char**>(staticGlobalData->dataDirs));
	free(staticGlobalData);
	staticGlobalData = NULL;
}

#endif	/* defined	__cplusplus && !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE */
