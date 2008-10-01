/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>
	
	This program is free software {} you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation {} either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY {} without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "AIInterfaceLibraryInfo.h"

#include "Platform/errorhandler.h"
#include "IAIInterfaceLibrary.h"
#include "Interface/aidefines.h"
#include "Interface/SInfo.h"
#include "StdAfx.h"
#include "LogOutput.h"

CAIInterfaceLibraryInfo::CAIInterfaceLibraryInfo(const IAIInterfaceLibrary& interface) {
	infos = interface.GetInfos();
	levelOfSupport = interface.GetLevelOfSupportFor(std::string(ENGINE_VERSION_STRING),
			ENGINE_VERSION_NUMBER);
}

CAIInterfaceLibraryInfo::CAIInterfaceLibraryInfo(const CAIInterfaceLibraryInfo& interfaceInfo) {
	infos = std::map<std::string, InfoItem>(
			interfaceInfo.infos.begin(),
			interfaceInfo.infos.end());
	levelOfSupport = interfaceInfo.levelOfSupport;
}

CAIInterfaceLibraryInfo::CAIInterfaceLibraryInfo(
		const std::string& interfaceInfoFile,
		const std::string& fileModes,
		const std::string& accessModes) {
	
	//std::vector<InfoItem> tmpInfos = ParseInfos(interfaceInfoFile, fileModes, accessModes);
	InfoItem tmpInfos[MAX_INFOS];
	unsigned int num = ParseInfos(interfaceInfoFile.c_str(), fileModes.c_str(), accessModes.c_str(), tmpInfos, MAX_INFOS);
/*
	std::vector<InfoItem>::iterator i;
    for (i = tmpInfos.begin(); i != tmpInfos.end(); ++i) {
		infos[std::string(i->key)] = *i;
    }
*/
    for (unsigned int i=0; i < num; ++i) {
		logOutput.Print("info %i: %s / %s / %s", i, tmpInfos[i].key, tmpInfos[i].value, tmpInfos[i].desc);
		infos[std::string(tmpInfos[i].key)] = copyInfoItem(&(tmpInfos[i]));
    }
	
	levelOfSupport = LOS_Unknown;
}

LevelOfSupport CAIInterfaceLibraryInfo::GetLevelOfSupportForCurrentEngine() const {
	return levelOfSupport;
}

std::string CAIInterfaceLibraryInfo::GetFileName() const {
	return GetInfo(AI_INTERFACE_PROPERTY_FILE_NAME);
}
std::string CAIInterfaceLibraryInfo::GetShortName() const {
	return GetInfo(AI_INTERFACE_PROPERTY_SHORT_NAME);
}
std::string CAIInterfaceLibraryInfo::GetVersion() const {
	return GetInfo(AI_INTERFACE_PROPERTY_VERSION);
}
std::string CAIInterfaceLibraryInfo::GetName() const {
	return GetInfo(AI_INTERFACE_PROPERTY_NAME);
}
std::string CAIInterfaceLibraryInfo::GetDescription() const {
	return GetInfo(AI_INTERFACE_PROPERTY_DESCRIPTION);
}
std::string CAIInterfaceLibraryInfo::GetURL() const {
	return GetInfo(AI_INTERFACE_PROPERTY_URL);
}
std::string CAIInterfaceLibraryInfo::GetInfo(const std::string& key) const {
	
	if (infos.find(key) == infos.end()) {
		std::string tmp = std::string("numInfos: ");
		tmp += IntToString(infos.size());
		tmp += "\n";
		std::map<std::string, InfoItem>::const_iterator it = infos.begin();
		for (unsigned int i=0; it != infos.end(); ++i, ++it) {
            tmp += "info ";
			tmp += IntToString(i);
			tmp += ": ";
			tmp += it->first;
			tmp += " / ";
			tmp += it->second.value;
			tmp += "\n";
        }
		tmp += "shortName: ";
		tmp += infos.at(std::string(AI_INTERFACE_PROPERTY_SHORT_NAME)).value;
		tmp += "\n";

		handleerror(NULL, std::string("AI interface property '").append(key).append("' can not be found").append("\n").append(tmp).c_str(), "IntefaceInfo Error", MBF_OK | MBF_EXCL);
	}
	return infos.at(key).value;
}
const std::map<std::string, InfoItem>* CAIInterfaceLibraryInfo::GetInfos() const {
	return &infos;
}


void CAIInterfaceLibraryInfo::SetFileName(const std::string& fileName) {
	SetInfo(AI_INTERFACE_PROPERTY_FILE_NAME, fileName);
}
void CAIInterfaceLibraryInfo::SetShortName(const std::string& shortName) {
	SetInfo(AI_INTERFACE_PROPERTY_SHORT_NAME, shortName);
}
void CAIInterfaceLibraryInfo::SetVersion(const std::string& version) {
	SetInfo(AI_INTERFACE_PROPERTY_VERSION, version);
}
void CAIInterfaceLibraryInfo::SetName(const std::string& name) {
	SetInfo(AI_INTERFACE_PROPERTY_NAME, name);
}
void CAIInterfaceLibraryInfo::SetDescription(const std::string& description) {
	SetInfo(AI_INTERFACE_PROPERTY_DESCRIPTION, description);
}
void CAIInterfaceLibraryInfo::SetURL(const std::string& url) {
	SetInfo(AI_INTERFACE_PROPERTY_URL, url);
}
bool CAIInterfaceLibraryInfo::SetInfo(const std::string& key, const std::string& value) {
	
	if (key == AI_INTERFACE_PROPERTY_SHORT_NAME ||
			key == AI_INTERFACE_PROPERTY_VERSION) {
		if (value.find_first_of("\t _#") != std::string::npos) {
			handleerror(NULL, "AI interface property (shortName or version) contains illegal characters ('_', '#' or white spaces)", "IntefaceInfo Error", MBF_OK | MBF_EXCL);
			return false;
		}
	}
	
	InfoItem ii = {key.c_str(), value.c_str(), NULL};
	infos[key] = ii;
	return true;
}
