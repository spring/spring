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

#ifndef _SKIRMISHAILIBRARY_H
#define	_SKIRMISHAILIBRARY_H

#include "ISkirmishAILibrary.h"

#include "Interface/SSAILibrary.h"

class CSkirmishAILibrary : public ISkirmishAILibrary {
public:
	CSkirmishAILibrary(const SSAILibrary& ai, const SSAISpecifier& specifier);
	virtual ~CSkirmishAILibrary();
	
	virtual SSAISpecifier GetSpecifier() const;
	/**
	 * Level of Support for a specific engine version and ai interface.
	 * @return see enum LevelOfSupport (higher values could be used optionally)
	 */
	virtual LevelOfSupport GetLevelOfSupportFor(
			const std::string& engineVersionString, int engineVersionNumber,
			const SAIInterfaceSpecifier& interfaceSpecifier) const;
	
    virtual std::map<std::string, InfoItem> GetInfo() const;
	virtual std::vector<Option> GetOptions() const;
	
	
    virtual void Init(int teamId) const;
    virtual void Release(int teamId) const;
    virtual int HandleEvent(int teamId, int topic, const void* data) const;
	
private:
	SSAILibrary sSAI;
	SSAISpecifier specifier;
	
private:
//	void reportInterfaceFunctionError(const std::string* libFileName, const std::string* functionName);
	
	static const int MAX_INFOS = 128;
	static const int MAX_OPTIONS = 128;
};

#endif	/* _SKIRMISHAILIBRARY_H */
