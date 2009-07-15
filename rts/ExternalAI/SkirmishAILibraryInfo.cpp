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

#include "SkirmishAILibraryInfo.h"

#include "Interface/aidefines.h"
#include "Interface/SSkirmishAILibrary.h"
#include "SkirmishAIKey.h"
#include "ISkirmishAILibrary.h"
#include "Util.h"
#include "Info.h"
#include "Option.h"

#include "Platform/errorhandler.h"
#include "FileSystem/VFSModes.h"


static const char* BAD_CHARS = "\t _#";
static const std::string DEFAULT_VALUE = "";

CSkirmishAILibraryInfo::CSkirmishAILibraryInfo(
		const CSkirmishAILibraryInfo& aiInfo) :
		info_keyLower_key(aiInfo.info_keyLower_key),
		info_key_value(aiInfo.info_key_value),
		info_key_description(aiInfo.info_key_description),
		options(aiInfo.options)
		{}

CSkirmishAILibraryInfo::CSkirmishAILibraryInfo(
		const std::string& aiInfoFile,
		const std::string& aiOptionFile) {

	std::vector<InfoItem> tmpInfo;
	parseInfo(tmpInfo, aiInfoFile);
	std::vector<InfoItem>::const_iterator ii;
	for (ii = tmpInfo.begin(); ii != tmpInfo.end(); ++ii) {
		SetInfo(ii->key, ii->value, ii->desc);
	}

	if (!aiOptionFile.empty()) {
		parseOptions(options, aiOptionFile);
	}
}

CSkirmishAILibraryInfo::~CSkirmishAILibraryInfo() {}


size_t CSkirmishAILibraryInfo::size() const {
	return info_keys.size();
}
const std::string& CSkirmishAILibraryInfo::GetKeyAt(size_t index) const {

	if (index < info_keys.size()) {
		return *(info_keys.begin() + index);
	} else {
		return DEFAULT_VALUE;
	}
}
const std::string& CSkirmishAILibraryInfo::GetValueAt(size_t index) const {

	if (index < info_keys.size()) {
		return info_key_value.find(GetKeyAt(index))->second;
	} else {
		return DEFAULT_VALUE;
	}
}
const std::string& CSkirmishAILibraryInfo::GetDescriptionAt(size_t index) const {

	if (index < info_keys.size()) {
		return info_key_description.find(GetKeyAt(index))->second;
	} else {
		return DEFAULT_VALUE;
	}
}

SkirmishAIKey CSkirmishAILibraryInfo::GetKey() const {

	const std::string& sn = GetShortName();
	const std::string& v = GetVersion();
	SkirmishAIKey key = SkirmishAIKey(sn, v);

	return key;
}

const std::string& CSkirmishAILibraryInfo::GetDataDir() const {
	return GetInfo(SKIRMISH_AI_PROPERTY_DATA_DIR);
}
const std::string& CSkirmishAILibraryInfo::GetDataDirCommon() const {
	return GetInfo(SKIRMISH_AI_PROPERTY_DATA_DIR_COMMON);
}
const std::string& CSkirmishAILibraryInfo::GetShortName() const {
	return GetInfo(SKIRMISH_AI_PROPERTY_SHORT_NAME);
}
const std::string& CSkirmishAILibraryInfo::GetVersion() const {
	return GetInfo(SKIRMISH_AI_PROPERTY_VERSION);
}
const std::string& CSkirmishAILibraryInfo::GetName() const {
	return GetInfo(SKIRMISH_AI_PROPERTY_NAME);
}
const std::string& CSkirmishAILibraryInfo::GetDescription() const {
	return GetInfo(SKIRMISH_AI_PROPERTY_DESCRIPTION);
}
const std::string& CSkirmishAILibraryInfo::GetURL() const {
	return GetInfo(SKIRMISH_AI_PROPERTY_URL);
}
const std::string& CSkirmishAILibraryInfo::GetInterfaceShortName() const {
	return GetInfo(SKIRMISH_AI_PROPERTY_INTERFACE_SHORT_NAME);
}
const std::string& CSkirmishAILibraryInfo::GetInterfaceVersion() const {
	return GetInfo(SKIRMISH_AI_PROPERTY_INTERFACE_VERSION);
}
const std::string& CSkirmishAILibraryInfo::GetInfo(const std::string& key) const {

	bool found = false;
	std::map<std::string, std::string>::const_iterator strPair;

	// get real key through lower case key
	strPair = info_keyLower_key.find(StringToLower(key));
	found = (strPair != info_keyLower_key.end());

	// get value
	if (found) {
		strPair = info_key_value.find(strPair->second);
		found = (strPair != info_key_value.end());
	}

	if (!found) {
		logOutput.Print("Skirmish AI property '%s' could not be found.", key.c_str());
		return DEFAULT_VALUE;
	} else {
		return strPair->second;
	}
}


void CSkirmishAILibraryInfo::SetDataDir(const std::string& dataDir) {
	SetInfo(SKIRMISH_AI_PROPERTY_DATA_DIR, dataDir);
}
void CSkirmishAILibraryInfo::SetDataDirCommon(const std::string& dataDirCommon) {
	SetInfo(SKIRMISH_AI_PROPERTY_DATA_DIR_COMMON, dataDirCommon);
}
void CSkirmishAILibraryInfo::SetShortName(const std::string& shortName) {
	SetInfo(SKIRMISH_AI_PROPERTY_SHORT_NAME, shortName);
}
void CSkirmishAILibraryInfo::SetVersion(const std::string& version) {
	SetInfo(SKIRMISH_AI_PROPERTY_VERSION, version);
}
void CSkirmishAILibraryInfo::SetName(const std::string& name) {
	SetInfo(SKIRMISH_AI_PROPERTY_NAME, name);
}
void CSkirmishAILibraryInfo::SetDescription(const std::string& description) {
	SetInfo(SKIRMISH_AI_PROPERTY_DESCRIPTION, description);
}
void CSkirmishAILibraryInfo::SetURL(const std::string& url) {
	SetInfo(SKIRMISH_AI_PROPERTY_URL, url);
}
void CSkirmishAILibraryInfo::SetInterfaceShortName(
		const std::string& interfaceShortName) {
	SetInfo(SKIRMISH_AI_PROPERTY_INTERFACE_SHORT_NAME, interfaceShortName);
}
void CSkirmishAILibraryInfo::SetInterfaceVersion(
		const std::string& interfaceVersion) {
	SetInfo(SKIRMISH_AI_PROPERTY_INTERFACE_VERSION, interfaceVersion);
}
bool CSkirmishAILibraryInfo::SetInfo(const std::string& key,
		const std::string& value, const std::string& description) {

	static const std::string snKey = StringToLower(SKIRMISH_AI_PROPERTY_SHORT_NAME);
	static const std::string vKey = StringToLower(SKIRMISH_AI_PROPERTY_VERSION);

	std::string keyLower = StringToLower(key);
	if (keyLower == snKey || keyLower == vKey) {
		if (value.find_first_of(BAD_CHARS) != std::string::npos) {
		logOutput.Print("Error, Skirmish AI property (%s or %s)\n"
				"contains illegal characters (%s).",
				SKIRMISH_AI_PROPERTY_SHORT_NAME, SKIRMISH_AI_PROPERTY_VERSION,
				BAD_CHARS);
			return false;
		}
	}

	// only add the key if it is not yet present
	if (info_key_value.find(key) == info_key_value.end()) {
		info_keys.push_back(key);
	}
	info_keyLower_key[keyLower] = key;
	info_key_value[key] = value;
	info_key_description[key] = description;

	return true;
}


const std::vector<Option>& CSkirmishAILibraryInfo::GetOptions() const {
	return options;
}