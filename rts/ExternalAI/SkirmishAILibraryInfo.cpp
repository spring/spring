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
#include "Interface/SSAILibrary.h"
#include "SkirmishAIKey.h"
#include "ISkirmishAILibrary.h"
#include "Util.h"

#include "Platform/errorhandler.h"
#include "FileSystem/VFSModes.h"

static const char* BAD_CHARS = "\t _#";

static const std::string& info_getValue(
		const std::map<std::string, InfoItem>& info,
		const std::string& key, const std::string& defValue) {

	std::map<std::string, InfoItem>::const_iterator inf
			= info.find(StringToLower(key));
	if (inf == info.end()) {
		return defValue;
	} else {
		return inf->second.value;
	}
}

CSkirmishAILibraryInfo::CSkirmishAILibraryInfo(
		const CSkirmishAILibraryInfo& aiInfo)
		: info(aiInfo.info), infoKeys_c(NULL), infoValues_c(NULL),
		options(aiInfo.options) {}

CSkirmishAILibraryInfo::CSkirmishAILibraryInfo(
		const std::string& aiInfoFile,
		const std::string& aiOptionFile)
		: infoKeys_c(NULL), infoValues_c(NULL) {

	std::vector<InfoItem> tmpInfo;
	parseInfo(tmpInfo, aiInfoFile);
	std::vector<InfoItem>::const_iterator ii;
	for (ii = tmpInfo.begin(); ii != tmpInfo.end(); ++ii) {
		info[StringToLower(ii->key)] = *ii;
	}

	if (aiOptionFile != "") {
		parseOptions(options, aiOptionFile);
	}
}

CSkirmishAILibraryInfo::~CSkirmishAILibraryInfo() {

	FreeCReferences();
}

SkirmishAIKey CSkirmishAILibraryInfo::GetKey() const {

	static const std::string defVal = "";
	static const std::string snKey = StringToLower(SKIRMISH_AI_PROPERTY_SHORT_NAME);
	static const std::string vKey = StringToLower(SKIRMISH_AI_PROPERTY_VERSION);

	const std::string& sn = info_getValue(info, snKey, defVal);
	const std::string& v = info_getValue(info, vKey, defVal);

	SkirmishAIKey key = SkirmishAIKey(sn, v);
	return key;
}

std::string CSkirmishAILibraryInfo::GetDataDir() const {
	return GetInfo(SKIRMISH_AI_PROPERTY_DATA_DIR);
}
std::string CSkirmishAILibraryInfo::GetFileName() const {
	return GetInfo(SKIRMISH_AI_PROPERTY_FILE_NAME);
}
std::string CSkirmishAILibraryInfo::GetShortName() const { // restrictions: none of the following: spaces, '_', '#'
	return GetInfo(SKIRMISH_AI_PROPERTY_SHORT_NAME);
}
std::string CSkirmishAILibraryInfo::GetVersion() const { // restrictions: none of the following: spaces, '_', '#'
	return GetInfo(SKIRMISH_AI_PROPERTY_VERSION);
}
std::string CSkirmishAILibraryInfo::GetName() const {
	return GetInfo(SKIRMISH_AI_PROPERTY_NAME);
}
std::string CSkirmishAILibraryInfo::GetDescription() const {
	return GetInfo(SKIRMISH_AI_PROPERTY_DESCRIPTION);
}
std::string CSkirmishAILibraryInfo::GetURL() const {
	return GetInfo(SKIRMISH_AI_PROPERTY_URL);
}
std::string CSkirmishAILibraryInfo::GetInterfaceShortName() const {
	return GetInfo(SKIRMISH_AI_PROPERTY_INTERFACE_SHORT_NAME);
}
std::string CSkirmishAILibraryInfo::GetInterfaceVersion() const {
	return GetInfo(SKIRMISH_AI_PROPERTY_INTERFACE_VERSION);
}
std::string CSkirmishAILibraryInfo::GetInfo(const std::string& key) const {

	static const std::string defVal = "";
	const std::string& val = info_getValue(info, key, defVal);
	if (val == defVal) {
		std::string errorMsg = std::string("Skirmish AI property '") + key
				+ "' could not be found.\n";
		handleerror(NULL, errorMsg.c_str(), "Skirmish AI Info Error",
				MBF_OK | MBF_EXCL);
	}

	return val;
}
const std::map<std::string, InfoItem>& CSkirmishAILibraryInfo::GetInfo() const {
	return info;
}

const std::vector<Option>& CSkirmishAILibraryInfo::GetOptions() const {
	return options;
}

void CSkirmishAILibraryInfo::CreateCReferences() {

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
void CSkirmishAILibraryInfo::FreeCReferences() {

	free(infoKeys_c);
	infoKeys_c = NULL;
	free(infoValues_c);
	infoValues_c = NULL;
}

const char** CSkirmishAILibraryInfo::GetCInfoKeys() const {
	return infoKeys_c;
}
const char** CSkirmishAILibraryInfo::GetCInfoValues() const {
	return infoValues_c;
}


void CSkirmishAILibraryInfo::SetDataDir(const std::string& dataDir) {
	SetInfo(SKIRMISH_AI_PROPERTY_DATA_DIR, dataDir);
}
void CSkirmishAILibraryInfo::SetFileName(const std::string& fileName) {
	SetInfo(SKIRMISH_AI_PROPERTY_FILE_NAME, fileName);
}
void CSkirmishAILibraryInfo::SetShortName(const std::string& shortName) { // restrictions: none of the following: spaces, '_', '#'
	SetInfo(SKIRMISH_AI_PROPERTY_SHORT_NAME, shortName);
}
void CSkirmishAILibraryInfo::SetVersion(const std::string& version) { // restrictions: none of the following: spaces, '_', '#'
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
				const std::string& value) {

	static const std::string snKey = StringToLower(SKIRMISH_AI_PROPERTY_SHORT_NAME);
	static const std::string vKey = StringToLower(SKIRMISH_AI_PROPERTY_VERSION);

	std::string lowerKey = StringToLower(key);
	if (lowerKey == snKey || key == vKey) {
		if (value.find_first_of(BAD_CHARS) != std::string::npos) {
			std::string msg = "Skirmish AI property (shortName or version)\n";
			msg += "contains illegal characters (";
			msg += BAD_CHARS;
			msg += ")";
			handleerror(NULL, msg.c_str(), "Skirmish AI Info Error",
					MBF_OK | MBF_EXCL);
			return false;
		}
	}

	InfoItem ii = {key, value, ""};
	info[lowerKey] = ii;

	return true;
}

void CSkirmishAILibraryInfo::SetOptions(const std::vector<Option>& _options) {

	// implicit convertible types -> range-ctor can be used
	options = std::vector<Option>(_options.begin(), _options.end());
}
