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

#include "ExternalAI/Interface/SAIInterfaceLibrary.h"
#include "ExternalAI/Interface/SStaticGlobalData.h"

CInterface* myInterface = NULL;

Export(int) initStatic(const SStaticGlobalData* staticGlobalData) {
	
	myInterface = new CInterface(staticGlobalData);
	return 0;
}

Export(int) releaseStatic() {
	
	delete myInterface;
	myInterface = NULL;
	return 0;
}

Export(enum LevelOfSupport) getLevelOfSupportFor(
		const char* engineVersion, int engineAIInterfaceGeneratedVersion) {
	return myInterface->GetLevelOfSupportFor(engineVersion, engineAIInterfaceGeneratedVersion);
}

Export(unsigned int) getInfo(struct InfoItem info[], unsigned int maxInfoItems) {
	return myInterface->GetInfo(info, maxInfoItems);
}


// skirmish AI methods
/*
Export(int) getSkirmishAISpecifiers(struct SSAISpecifier* sAISpecifiers, int max) {
	return myInterface->GetSkirmishAISpecifiers(sAISpecifiers, max);
}
Export(const struct SSAILibrary*) loadSkirmishAILibrary(const struct SSAISpecifier* const sAISpecifier) {
	return myInterface->LoadSkirmishAILibrary(sAISpecifier);
}
*/
Export(const struct SSAILibrary*) loadSkirmishAILibrary(
const struct InfoItem info[], unsigned int numInfoItems) {
	return myInterface->LoadSkirmishAILibrary(info, numInfoItems);
}
Export(int) unloadSkirmishAILibrary(const struct SSAISpecifier* const sAISpecifier) {
	return myInterface->UnloadSkirmishAILibrary(sAISpecifier);
}
Export(int) unloadAllSkirmishAILibraries() {
	return myInterface->UnloadAllSkirmishAILibraries();
}



// group AI methods
/*
Export(int) getGroupAISpecifiers(struct SGAISpecifier* gAISpecifiers, int max) {
	return myInterface->GetGroupAISpecifiers(gAISpecifiers, max);
}
Export(const struct SGAILibrary*) loadGroupAILibrary(const struct SGAISpecifier* const gAISpecifier) {
	return myInterface->LoadGroupAILibrary(gAISpecifier);
}
*/
Export(const struct SGAILibrary*) loadGroupAILibrary(
const struct InfoItem info[], unsigned int numInfoItems) {
	return myInterface->LoadGroupAILibrary(info, numInfoItems);
}
Export(int) unloadGroupAILibrary(const struct SGAISpecifier* const gAISpecifier) {
	return myInterface->UnloadGroupAILibrary(gAISpecifier);
}
Export(int) unloadAllGroupAILibraries() {
	return myInterface->UnloadAllGroupAILibraries();
}

