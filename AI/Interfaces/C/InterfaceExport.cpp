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

#include "InterfaceExport.h"

#include "Interface.h"
#include "CUtils/Util.h"

#include "ExternalAI/Interface/SAIInterfaceLibrary.h"
#include "ExternalAI/Interface/SStaticGlobalData.h"

static CInterface* myInterface = NULL;

static void local_copyToInfoMap(std::map<std::string, std::string>& map,
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues) {

	for (unsigned int i=0; i < infoSize; ++i) {
		map[infoKeys[i]] = infoValues[i];
	}
}

EXPORT(int) initStatic(
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues,
		const SStaticGlobalData* staticGlobalData) {

	util_setMyInfo(infoSize, infoKeys, infoValues);

	std::map<std::string, std::string> infoMap;
	local_copyToInfoMap(infoMap, infoSize, infoKeys, infoValues);

	myInterface = new CInterface(infoMap, staticGlobalData);
	return 0;
}

EXPORT(int) releaseStatic() {

	delete myInterface;
	myInterface = NULL;
	return 0;
}

EXPORT(enum LevelOfSupport) getLevelOfSupportFor(
		const char* engineVersion, int engineAIInterfaceGeneratedVersion) {
	return myInterface->GetLevelOfSupportFor(engineVersion, engineAIInterfaceGeneratedVersion);
}

EXPORT(const struct SSAILibrary*) loadSkirmishAILibrary(
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues) {

	std::map<std::string, std::string> infoMap;
	local_copyToInfoMap(infoMap, infoSize, infoKeys, infoValues);

	return myInterface->LoadSkirmishAILibrary(infoMap);
}
EXPORT(int) unloadSkirmishAILibrary(
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues) {

	std::map<std::string, std::string> infoMap;
	local_copyToInfoMap(infoMap, infoSize, infoKeys, infoValues);

	return myInterface->UnloadSkirmishAILibrary(infoMap);
}
EXPORT(int) unloadAllSkirmishAILibraries() {
	return myInterface->UnloadAllSkirmishAILibraries();
}
