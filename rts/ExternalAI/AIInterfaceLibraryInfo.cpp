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

#include "IAIInterfaceLibrary.h"
#include "AIInterfaceKey.h"
#include "Interface/aidefines.h"
#include "Interface/SInfo.h"
#include "Interface/SAIInterfaceLibrary.h"

#include "Platform/errorhandler.h"
#include "FileSystem/VFSModes.h"

//CAIInterfaceLibraryInfo::CAIInterfaceLibraryInfo(
//		const IAIInterfaceLibrary& interface) {
//
//	info = interface.GetInfo();
//
//	std::map<std::string, InfoItem>::iterator iip;
//    for (iip = info.begin(); iip != info.begin(); ++iip) {
//		iip->second = copyInfoItem(&(iip->second));
//    }
//}

CAIInterfaceLibraryInfo::CAIInterfaceLibraryInfo(
		const CAIInterfaceLibraryInfo& interfaceInfo)
		: infoKeys_c(NULL), infoValues_c(NULL) {

	info = std::map<std::string, InfoItem>(
			interfaceInfo.info.begin(),
			interfaceInfo.info.end());

	std::map<std::string, InfoItem>::iterator iip;
    for (iip = info.begin(); iip != info.begin(); ++iip) {
		iip->second = copyInfoItem(&(iip->second));
    }
}

CAIInterfaceLibraryInfo::CAIInterfaceLibraryInfo(
		const std::string& interfaceInfoFile)
		: infoKeys_c(NULL), infoValues_c(NULL) {

	InfoItem tmpInfo[MAX_INFOS];
	unsigned int num = ParseInfo(interfaceInfoFile.c_str(), SPRING_VFS_RAW,
			SPRING_VFS_RAW, tmpInfo, MAX_INFOS);
    for (unsigned int i=0; i < num; ++i) {
/*
		logOutput.Print("info %i: %s / %s / %s", i, tmpInfo[i].key,
				tmpInfo[i].value, tmpInfo[i].desc);
*/
		info[std::string(tmpInfo[i].key)] = copyInfoItem(&(tmpInfo[i]));
    }

	//levelOfSupport = LOS_Unknown;
}

CAIInterfaceLibraryInfo::~CAIInterfaceLibraryInfo() {

	FreeCReferences();
	std::map<std::string, InfoItem>::const_iterator iip;
    for (iip = info.begin(); iip != info.begin(); ++iip) {
		deleteInfoItem(&(iip->second));
    }
}

/*
LevelOfSupport CAIInterfaceLibraryInfo::GetLevelOfSupportForCurrentEngine() const {
	return levelOfSupport;
}
*/

AIInterfaceKey CAIInterfaceLibraryInfo::GetKey() const {

	const char* sn = info.at(AI_INTERFACE_PROPERTY_SHORT_NAME).value;
	const char* v = info.at(AI_INTERFACE_PROPERTY_VERSION).value;
	AIInterfaceKey key = AIInterfaceKey(sn, v);
	return key;
}

std::string CAIInterfaceLibraryInfo::GetDataDir() const {
	return GetInfo(AI_INTERFACE_PROPERTY_DATA_DIR);
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

	if (info.find(key) == info.end()) {
		std::string errorMsg = std::string("AI interface property '") + key
				+ "' could not be found.\n";
		handleerror(NULL, errorMsg.c_str(), "AI Interface Info Error",
				MBF_OK | MBF_EXCL);
	}
	return info.at(key).value;
}
const std::map<std::string, InfoItem>& CAIInterfaceLibraryInfo::GetInfo() const {
	return info;
}
void CAIInterfaceLibraryInfo::CreateCReferences() {

	FreeCReferences();

	infoKeys_c = (const char**) calloc(info.size(), sizeof(char*));
	infoValues_c = (const char**) calloc(info.size(), sizeof(char*));
	unsigned int i=0;
	std::map<std::string, InfoItem>::const_iterator ii;
	for (ii=info.begin(); ii != info.end(); ++ii) {
		infoKeys_c[i] = ii->second.key;
		infoValues_c[i] = ii->second.value;
		i++;
	}
}
void CAIInterfaceLibraryInfo::FreeCReferences() {

	free(infoKeys_c);
	infoKeys_c = NULL;
	free(infoValues_c);
	infoValues_c = NULL;
}

const char** CAIInterfaceLibraryInfo::GetCInfoKeys() const {
	return infoKeys_c;
}
const char** CAIInterfaceLibraryInfo::GetCInfoValues() const {
	return infoValues_c;
}


void CAIInterfaceLibraryInfo::SetDataDir(const std::string& dataDir) {
	SetInfo(AI_INTERFACE_PROPERTY_DATA_DIR, dataDir);
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
bool CAIInterfaceLibraryInfo::SetInfo(const std::string& key,
		const std::string& value) {

	if (key == AI_INTERFACE_PROPERTY_SHORT_NAME ||
			key == AI_INTERFACE_PROPERTY_VERSION) {
		if (value.find_first_of("\t _#") != std::string::npos) {
			handleerror(NULL, "AI interface property (shortName or version)\n"
					"contains illegal characters ('_', '#' or white spaces)",
					"AI Interface Info Error", MBF_OK | MBF_EXCL);
			return false;
		}
	}

	InfoItem ii = {key.c_str(), value.c_str(), NULL};
	ii = copyInfoItem(&ii);

	info[key] = ii;
	return true;
}
