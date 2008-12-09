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
#include "Interface/SAIInterfaceLibrary.h"
#include "Util.h"

#include "Platform/errorhandler.h"
#include "FileSystem/VFSModes.h"

static const char* BAD_CHARS = "\t _#";

static const std::string& info_getValue(
		const std::map<std::string, InfoItem>& info,
		const std::string& key, const std::string& defValue) {

	std::map<std::string, InfoItem>::const_iterator inf = info.find(StringToLower(key));
	if (inf == info.end()) {
		return defValue;
	} else {
		return inf->second.value;
	}
}


CAIInterfaceLibraryInfo::CAIInterfaceLibraryInfo(
		const CAIInterfaceLibraryInfo& interfaceInfo)
		: info(interfaceInfo.info), infoKeys_c(NULL), infoValues_c(NULL) {}

CAIInterfaceLibraryInfo::CAIInterfaceLibraryInfo(
		const std::string& interfaceInfoFile)
		: infoKeys_c(NULL), infoValues_c(NULL) {

	std::vector<InfoItem> tmpInfo;
	parseInfo(tmpInfo, interfaceInfoFile);
	std::vector<InfoItem>::const_iterator ii;
	for (ii = tmpInfo.begin(); ii != tmpInfo.end(); ++ii) {
		info[StringToLower(ii->key)] = *ii;
	}
}

CAIInterfaceLibraryInfo::~CAIInterfaceLibraryInfo() {

	FreeCReferences();
}

AIInterfaceKey CAIInterfaceLibraryInfo::GetKey() const {

	static const std::string defVal = "";
	static const std::string snKey = StringToLower(AI_INTERFACE_PROPERTY_SHORT_NAME);
	static const std::string vKey = StringToLower(AI_INTERFACE_PROPERTY_VERSION);

	const std::string& sn = info_getValue(info, snKey, defVal);
	const std::string& v = info_getValue(info, vKey, defVal);
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

	static const std::string defVal = "";
	const std::string& val = info_getValue(info, key, defVal);
	if (val == defVal) {
		std::string errorMsg = std::string("AI interface property '") + key
				+ "' could not be found.\n";
		handleerror(NULL, errorMsg.c_str(), "AI Interface Info Error",
				MBF_OK | MBF_EXCL);
	}

	return val;
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
		infoKeys_c[i] = ii->second.key.c_str();
		infoValues_c[i] = ii->second.value.c_str();
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

	static const std::string snKey = StringToLower(AI_INTERFACE_PROPERTY_SHORT_NAME);
	static const std::string vKey = StringToLower(AI_INTERFACE_PROPERTY_VERSION);

	std::string lowerKey = StringToLower(key);
	if (lowerKey == snKey || key == vKey) {
		if (value.find_first_of(BAD_CHARS) != std::string::npos) {
			std::string msg = "AI interface property (shortName or version)\n";
			msg += "contains illegal characters (";
			msg += BAD_CHARS;
			msg += ")";
			handleerror(NULL, msg.c_str(), "AI Interface Info Error",
					MBF_OK | MBF_EXCL);
			return false;
		}
	}

	InfoItem ii = {key, value, ""};
	info[lowerKey] = ii;

	return true;
}
