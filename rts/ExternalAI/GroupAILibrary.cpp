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
#include <string>

CGroupAILibrary::CGroupAILibrary(const SGAILibrary& ai) {
	
	sGAI = ai;
	
	std::map<std::string, InfoItem> infos = GetInfos();
	specifier.shortName = infos.at(GROUP_AI_PROPERTY_SHORT_NAME).value;
	specifier.version = infos.at(GROUP_AI_PROPERTY_VERSION).value;
}

CGroupAILibrary::~CGroupAILibrary() {}
	
SGAISpecifier CGroupAILibrary::GetSpecifier() const {
	return specifier;
}

LevelOfSupport CGroupAILibrary::GetLevelOfSupportFor(
			const std::string& engineVersionString, int engineVersionNumber,
		const SAIInterfaceSpecifier& interfaceSpecifier) const {
	
	if (sGAI.getLevelOfSupportFor != NULL) {
		return sGAI.getLevelOfSupportFor(engineVersionString.c_str(), engineVersionNumber,
			interfaceSpecifier.shortName, interfaceSpecifier.version);
	} else {
		return LOS_Unknown;
	}
}
	
std::map<std::string, InfoItem> CGroupAILibrary::GetInfos() const {
	
	std::map<std::string, InfoItem> infos;
	
	if (sGAI.getInfos != NULL) {
		InfoItem infs[MAX_INFOS];
		int num = sGAI.getInfos(infs, MAX_INFOS);

		int i;
		for (i=0; i < num; ++i) {
			InfoItem newII = copyInfoItem(&infs[i]);
			infos[std::string(newII.key)] = newII;
		}
	}

	return infos;
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


void CGroupAILibrary::Init(int teamId, int groupId) const {
	sGAI.init(teamId, groupId);
}

void CGroupAILibrary::Release(int teamId, int groupId) const {
	sGAI.release(teamId, groupId);
}

int CGroupAILibrary::HandleEvent(int teamId, int groupId, int topic, const void* data) const {
	return sGAI.handleEvent(teamId, groupId, topic, data);
}
