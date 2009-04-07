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


static CInterface* myInterface = NULL;

EXPORT(int) initStatic(int interfaceId,
		const struct SAIInterfaceCallback* callback) {

	if (myInterface == NULL) {
		myInterface = new CInterface(interfaceId, callback);
	}

	return 0; // signal: OK
}

EXPORT(int) releaseStatic() {

	delete myInterface;
	myInterface = NULL;

	return 0;
}

//EXPORT(enum LevelOfSupport) getLevelOfSupportFor(
//		const char* engineVersion, int engineAIInterfaceGeneratedVersion) {
//	return myInterface->GetLevelOfSupportFor(engineVersion, engineAIInterfaceGeneratedVersion);
//}

EXPORT(const struct SSkirmishAILibrary*) loadSkirmishAILibrary(
		const char* const shortName,
		const char* const version) {
	return myInterface->LoadSkirmishAILibrary(shortName, version);
}
EXPORT(int) unloadSkirmishAILibrary(
		const char* const shortName,
		const char* const version) {
	return myInterface->UnloadSkirmishAILibrary(shortName, version);
}
EXPORT(int) unloadAllSkirmishAILibraries() {
	return myInterface->UnloadAllSkirmishAILibraries();
}
