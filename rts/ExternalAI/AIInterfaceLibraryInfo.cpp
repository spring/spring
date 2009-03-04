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
#include "Info.h"

#include "Platform/errorhandler.h"
#include "FileSystem/VFSModes.h"


static const char* BAD_CHARS = "\t _#";
static const std::string DEFAULT_VALUE = "";


CAIInterfaceLibraryInfo::CAIInterfaceLibraryInfo(
		const CAIInterfaceLibraryInfo& interfaceInfo) :
		keyLower_key(interfaceInfo.keyLower_key),
		key_value(interfaceInfo.key_value),
		key_description(interfaceInfo.key_description)
		{}

CAIInterfaceLibraryInfo::CAIInterfaceLibraryInfo(
		const std::string& interfaceInfoFile) {

	std::vector<InfoItem> tmpInfo;
	parseInfo(tmpInfo, interfaceInfoFile);
	std::vector<InfoItem>::const_iterator ii;
	for (ii = tmpInfo.begin(); ii != tmpInfo.end(); ++ii) {
		SetInfo(ii->key, ii->value, ii->desc);
	}
}

CAIInterfaceLibraryInfo::~CAIInterfaceLibraryInfo() {}

size_t CAIInterfaceLibraryInfo::size() const {
	return keys.size();
}
const std::string& CAIInterfaceLibraryInfo::GetKeyAt(size_t index) const {

	if (index < keys.size()) {
		return *(keys.begin() + index);
	} else {
		return DEFAULT_VALUE;
	}
}
const std::string& CAIInterfaceLibraryInfo::GetValueAt(size_t index) const {

	if (index < keys.size()) {
		return key_value.find(GetKeyAt(index))->second;
	} else {
		return DEFAULT_VALUE;
	}
}
const std::string& CAIInterfaceLibraryInfo::GetDescriptionAt(size_t index) const {

	if (index < keys.size()) {
		return key_description.find(GetKeyAt(index))->second;
	} else {
		return DEFAULT_VALUE;
	}
}

AIInterfaceKey CAIInterfaceLibraryInfo::GetKey() const {

	const std::string& sn = GetInfo(AI_INTERFACE_PROPERTY_SHORT_NAME);
	const std::string& v = GetInfo(AI_INTERFACE_PROPERTY_VERSION);
	AIInterfaceKey key = AIInterfaceKey(sn, v);

	return key;
}

const std::string& CAIInterfaceLibraryInfo::GetDataDir() const {
	return GetInfo(AI_INTERFACE_PROPERTY_DATA_DIR);
}
const std::string& CAIInterfaceLibraryInfo::GetShortName() const {
	return GetInfo(AI_INTERFACE_PROPERTY_SHORT_NAME);
}
const std::string& CAIInterfaceLibraryInfo::GetVersion() const {
	return GetInfo(AI_INTERFACE_PROPERTY_VERSION);
}
const std::string& CAIInterfaceLibraryInfo::GetName() const {
	return GetInfo(AI_INTERFACE_PROPERTY_NAME);
}
const std::string& CAIInterfaceLibraryInfo::GetDescription() const {
	return GetInfo(AI_INTERFACE_PROPERTY_DESCRIPTION);
}
const std::string& CAIInterfaceLibraryInfo::GetURL() const {
	return GetInfo(AI_INTERFACE_PROPERTY_URL);
}
const std::string& CAIInterfaceLibraryInfo::GetInfo(const std::string& key) const {

	bool found = false;
	std::map<std::string, std::string>::const_iterator strPair;

	// get real key through lower case key
	strPair = keyLower_key.find(StringToLower(key));
	found = (strPair != keyLower_key.end());

	// get value
	if (found) {
		strPair = key_value.find(strPair->second);
		found = (strPair != key_value.end());
	}

	if (!found) {
		logOutput.Print("AI Interface property '%s' could not be found.", key.c_str());
		return DEFAULT_VALUE;
	} else {
		return strPair->second;
	}
}


void CAIInterfaceLibraryInfo::SetDataDir(const std::string& dataDir) {
	SetInfo(AI_INTERFACE_PROPERTY_DATA_DIR, dataDir);
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
		const std::string& value, const std::string& description) {

	static const std::string snKey = StringToLower(AI_INTERFACE_PROPERTY_SHORT_NAME);
	static const std::string vKey = StringToLower(AI_INTERFACE_PROPERTY_VERSION);

	std::string keyLower = StringToLower(key);
	if (keyLower == snKey || keyLower == vKey) {
		if (value.find_first_of(BAD_CHARS) != std::string::npos) {
		logOutput.Print("Error, AI interface property (%s or %s)\n"
				"contains illegal characters (%s).",
				AI_INTERFACE_PROPERTY_SHORT_NAME, AI_INTERFACE_PROPERTY_VERSION,
				BAD_CHARS);
			return false;
		}
	}

	// only add the key if it is not yet present
	if (key_value.find(key) == key_value.end()) {
		keys.push_back(key);
	}
	keyLower_key[keyLower] = key;
	key_value[key] = value;
	key_description[key] = description;

	return true;
}
