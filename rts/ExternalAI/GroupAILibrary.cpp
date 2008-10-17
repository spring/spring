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

#include "GroupAILibrary.h"

#include "Interface/SAIInterfaceLibrary.h"
#include "LogOutput.h"
#include <string>

CGroupAILibrary::CGroupAILibrary(const SGAILibrary& ai,
		const SGAISpecifier& specifier) : sGAI(ai), specifier(specifier) {}

CGroupAILibrary::~CGroupAILibrary() {}
	
SGAISpecifier CGroupAILibrary::GetSpecifier() const {
	return specifier;
}

LevelOfSupport CGroupAILibrary::GetLevelOfSupportFor(int teamId, int groupId,
			const std::string& engineVersionString, int engineVersionNumber,
		const SAIInterfaceSpecifier& interfaceSpecifier) const {
	
	if (sGAI.getLevelOfSupportFor != NULL) {
		return sGAI.getLevelOfSupportFor(teamId, groupId,
				engineVersionString.c_str(), engineVersionNumber,
				interfaceSpecifier.shortName, interfaceSpecifier.version);
	} else {
		return LOS_Unknown;
	}
}
	
/*
std::map<std::string, InfoItem> CGroupAILibrary::GetInfo() const {
	
	std::map<std::string, InfoItem> info;
	
	if (sGAI.getInfo != NULL) {
		InfoItem infs[MAX_INFOS];
		int num = sGAI.getInfo(infs, MAX_INFOS);

		int i;
		for (i=0; i < num; ++i) {
			InfoItem newII = copyInfoItem(&infs[i]);
			info[std::string(newII.key)] = newII;
		}
	}

	return info;
}
std::vector<Option> CGroupAILibrary::GetOptions() const {
	
	std::vector<Option> ops;
	
	if (sGAI.getOptions != NULL) {
		Option options[MAX_OPTIONS];
		int num = sGAI.getOptions(options, MAX_OPTIONS);

		int i;
		for (i=0; i < num; ++i) {
			ops.push_back(options[i]);
		}
	}

	return ops;
}
*/


void CGroupAILibrary::Init(int teamId, int groupId, const InfoItem info[],
		unsigned int numInfoItems) const {
	
	if (sGAI.init != NULL) {
		int error = sGAI.init(teamId, groupId, info, numInfoItems);
		if (error != 0) {
			// init failed
			logOutput.Print("Failed to initialize an AI for team %d and group %d, error: %d", teamId, groupId, error);
		}
	}
}

void CGroupAILibrary::Release(int teamId, int groupId) const {
	
	if (sGAI.release != NULL) {
		int error = sGAI.release(teamId, groupId);
		if (error != 0) {
			// release failed
			logOutput.Print("Failed to release the AI for team %d and group %d, error: %d", teamId, groupId, error);
		}
	}
}

int CGroupAILibrary::HandleEvent(int teamId, int groupId, int topic, const void* data) const {
	return sGAI.handleEvent(teamId, groupId, topic, data);
}
