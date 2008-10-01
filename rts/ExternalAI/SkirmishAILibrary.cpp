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
#include <string>

CSkirmishAILibrary::CSkirmishAILibrary(const SSAILibrary& ai,
		const SSAISpecifyer& specifyer) : sSAI(ai), specifyer(specifyer) {
	
/*
	std::map<std::string, InfoItem> infos = GetInfos();
	specifyer.shortName = infos.at(SKIRMISH_AI_PROPERTY_SHORT_NAME).value;
	specifyer.version = infos.at(SKIRMISH_AI_PROPERTY_VERSION).value;
*/
}

CSkirmishAILibrary::~CSkirmishAILibrary() {}
	
SSAISpecifyer CSkirmishAILibrary::GetSpecifyer() const {
	return specifyer;
}

LevelOfSupport CSkirmishAILibrary::GetLevelOfSupportFor(
			const std::string& engineVersionString, int engineVersionNumber,
		const SAIInterfaceSpecifyer& interfaceSpecifyer) const {
	
	if (sSAI.getLevelOfSupportFor != NULL) {
		return sSAI.getLevelOfSupportFor(engineVersionString.c_str(), engineVersionNumber,
			interfaceSpecifyer.shortName, interfaceSpecifyer.version);
	} else {
		return LOS_Unknown;
	}
}
	
std::map<std::string, InfoItem> CSkirmishAILibrary::GetInfos() const {
	
	std::map<std::string, InfoItem> infos;
	
	if (sSAI.getInfos != NULL) {
		InfoItem infs[MAX_INFOS];
		int num = sSAI.getInfos(infs, MAX_INFOS);

		int i;
		for (i=0; i < num; ++i) {
			InfoItem newII = copyInfoItem(&infs[i]);
			infos[std::string(newII.key)] = newII;
		}
	}

	return infos;
}
std::vector<Option> CSkirmishAILibrary::GetOptions() const {
	
	std::vector<Option> ops;
	
	if (sSAI.getOptions != NULL) {
		Option options[MAX_OPTIONS];
		int num = sSAI.getOptions(options, MAX_OPTIONS);

		int i;
		for (i=0; i < num; ++i) {
			ops.push_back(options[i]);
		}
	}

	return ops;
}


void CSkirmishAILibrary::Init(int teamId) const {
	sSAI.init(teamId);
}

void CSkirmishAILibrary::Release(int teamId) const {
	sSAI.release(teamId);
}

int CSkirmishAILibrary::HandleEvent(int teamId, int topic, const void* data) const {
	return sSAI.handleEvent(teamId, topic, data);
}
