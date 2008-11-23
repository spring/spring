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

#include "SkirmishAILibrary.h"

#include "Interface/SAIInterfaceLibrary.h"
#include "LogOutput.h"
#include <string>

CSkirmishAILibrary::CSkirmishAILibrary(const SSAILibrary& ai,
		const SSAISpecifier& specifier,
		const struct InfoItem p_info[], unsigned int numInfoItems)
		: sSAI(ai), specifier(specifier), numInfoItems(numInfoItems) {

	info = (struct InfoItem*) calloc(numInfoItems, sizeof(struct InfoItem));
	unsigned int i;
    for (i=0; i < numInfoItems; ++i) {
        info[i] = copyInfoItem(p_info[i]);
    }
}

CSkirmishAILibrary::~CSkirmishAILibrary() {

	unsigned int i;
    for (i=0; i < numInfoItems; ++i) {
        deleteInfoItem(info[i]);
    }
	free(info);
}
	
SSAISpecifier CSkirmishAILibrary::GetSpecifier() const {
	return specifier;
}

LevelOfSupport CSkirmishAILibrary::GetLevelOfSupportFor(int teamId,
			const std::string& engineVersionString, int engineVersionNumber,
		const SAIInterfaceSpecifier& interfaceSpecifier) const {
	
	if (sSAI.getLevelOfSupportFor != NULL) {
		return sSAI.getLevelOfSupportFor(teamId, engineVersionString.c_str(),
				engineVersionNumber, interfaceSpecifier.shortName,
				interfaceSpecifier.version);
	} else {
		return LOS_Unknown;
	}
}
	
/*
std::map<std::string, InfoItem> CSkirmishAILibrary::GetInfo(unsigned int teamId) const {
	
	std::map<std::string, InfoItem> info;
	
	if (sSAI.getInfo != NULL) {
		InfoItem infs[MAX_INFOS];
		int num = sSAI.getInfo(teamId, infs, MAX_INFOS);

		int i;
		for (i=0; i < num; ++i) {
			InfoItem newII = copyInfoItem(&infs[i]);
			info[std::string(newII.key)] = newII;
		}
	}

	return info;
}
std::vector<Option> CSkirmishAILibrary::GetOptions(unsigned int teamId) const {
	
	std::vector<Option> ops;
	
	if (sSAI.getOptions != NULL) {
		Option options[MAX_OPTIONS];
		int num = sSAI.getOptions(teamId, options, MAX_OPTIONS);

		int i;
		for (i=0; i < num; ++i) {
			ops.push_back(options[i]);
		}
	}

	return ops;
}
*/


void CSkirmishAILibrary::Init(int teamId) const {
	
	if (sSAI.init != NULL) {
		int error = sSAI.init(teamId, info, numInfoItems);
		if (error != 0) {
			// init failed
			logOutput.Print("Failed to initialize an AI for team %d, error: %d", teamId, error);
		}
	}
}

void CSkirmishAILibrary::Release(int teamId) const {
	
	if (sSAI.release != NULL) {
		int error = sSAI.release(teamId);
		if (error != 0) {
			// release failed
			logOutput.Print("Failed to release the AI for team %d, error: %d", teamId, error);
		}
	}
}

int CSkirmishAILibrary::HandleEvent(int teamId, int topic, const void* data) const {
	return sSAI.handleEvent(teamId, topic, data);
}
