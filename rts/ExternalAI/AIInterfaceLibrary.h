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

#ifndef _AIINTERFACELIBRARY_H
#define	_AIINTERFACELIBRARY_H

#include "IAIInterfaceLibrary.h"

#include "Platform/SharedLib.h"
#include "Interface/SAIInterfaceLibrary.h"
#include "SkirmishAIKey.h"

#include <string>
#include <map>

class CAIInterfaceLibraryInfo;
class CSkirmishAILibraryInfo;

/**
 * Default implementation of IAIInterfaceLibrary.
 */
class CAIInterfaceLibrary : public IAIInterfaceLibrary {
public:
	CAIInterfaceLibrary(const CAIInterfaceLibraryInfo& info);
	virtual ~CAIInterfaceLibrary();

	virtual AIInterfaceKey GetKey() const;

	virtual LevelOfSupport GetLevelOfSupportFor(
			const std::string& engineVersionString, int engineVersionNumber) const;

	virtual int GetLoadCount() const;

	// Skirmish AI methods
	virtual const ISkirmishAILibrary* FetchSkirmishAILibrary(const CSkirmishAILibraryInfo& aiInfo);
	virtual int ReleaseSkirmishAILibrary(const SkirmishAIKey& sAISpecifier);
	virtual int GetSkirmishAILibraryLoadCount(const SkirmishAIKey& sAISpecifier) const;
	virtual int ReleaseAllSkirmishAILibraries();

private:
	SStaticGlobalData* staticGlobalData;
	void InitStatic();
	void ReleaseStatic();

private:
	std::string libFilePath;
	SharedLib* sharedLib;
	SAIInterfaceLibrary sAIInterfaceLibrary;
	const CAIInterfaceLibraryInfo& info;
	std::map<const SkirmishAIKey, ISkirmishAILibrary*> loadedSkirmishAILibraries;
	std::map<const SkirmishAIKey, int> skirmishAILoadCount;

private:
	static const int MAX_INFOS = 128;

	static void reportInterfaceFunctionError(const std::string* libFileName,
			const std::string* functionName);
	int InitializeFromLib(const std::string& libFilePath);

	std::string FindLibFile();
};

#endif // _AIINTERFACELIBRARY_H
