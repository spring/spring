/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SkirmishAILibraryInfo.h"

#include "Interface/aidefines.h"
#include "Interface/SSkirmishAILibrary.h"
#include "SkirmishAIKey.h"
#include "System/StringUtil.h"
#include "System/Info.h"
#include "System/Option.h"
#include "System/Log/ILog.h"

#include "System/FileSystem/VFSModes.h"


static const char* BAD_CHARS = "\t _#";
static const std::string DEFAULT_VALUE;

CSkirmishAILibraryInfo::CSkirmishAILibraryInfo(const CSkirmishAILibraryInfo& aiInfo) = default;

CSkirmishAILibraryInfo::CSkirmishAILibraryInfo(
	const std::string& aiInfoFile,
	const std::string& aiOptionFile
) {
	std::vector<InfoItem> tmpInfo;
	info_parseInfo(tmpInfo, aiInfoFile);

	for (auto& info: tmpInfo) {
		info_convertToStringValue(&info);
		SetInfo(info.key, info.valueTypeString, info.desc);
	}

	if (!aiOptionFile.empty()) {
		option_parseOptions(options, aiOptionFile);
	}
}

CSkirmishAILibraryInfo::CSkirmishAILibraryInfo(
	const std::map<std::string, std::string>& aiInfo,
	const std::string& aiOptionLua
) {
	for (const auto& info: aiInfo) {
		SetInfo(info.first, info.second);
	}

	if (!aiOptionLua.empty()) {
		option_parseOptionsLuaString(options, aiOptionLua);
	}
}


const std::string& CSkirmishAILibraryInfo::GetKeyAt(size_t index) const {
	if (index < info_keys.size())
		return *(info_keys.begin() + index);

	return DEFAULT_VALUE;
}

const std::string& CSkirmishAILibraryInfo::GetValueAt(size_t index) const {
	if (index < info_keys.size())
		return info_key_value.find(GetKeyAt(index))->second;

	return DEFAULT_VALUE;
}

const std::string& CSkirmishAILibraryInfo::GetDescriptionAt(size_t index) const {
	if (index < info_keys.size())
		return info_key_description.find(GetKeyAt(index))->second;

	return DEFAULT_VALUE;
}


SkirmishAIKey CSkirmishAILibraryInfo::GetKey() const {
	return SkirmishAIKey(GetShortName(), GetVersion());
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

bool CSkirmishAILibraryInfo::IsLuaAI() const {
	const std::string& isLuaStr = GetInfo("isLuaAI");

	if (!isLuaStr.empty())
		return (StringToBool(isLuaStr));

	return false;
}

const std::string& CSkirmishAILibraryInfo::GetInterfaceShortName() const {
	return GetInfo(SKIRMISH_AI_PROPERTY_INTERFACE_SHORT_NAME);
}
const std::string& CSkirmishAILibraryInfo::GetInterfaceVersion() const {
	return GetInfo(SKIRMISH_AI_PROPERTY_INTERFACE_VERSION);
}

const std::string& CSkirmishAILibraryInfo::GetInfo(const std::string& key) const {
	// get real key through lower case key
	auto strPair = info_keyLower_key.find(StringToLower(key));
	bool found = (strPair != info_keyLower_key.end());

	// get value
	if (found) {
		strPair = info_key_value.find(strPair->second);
		found = (strPair != info_key_value.end());
	}

	if (!found) {
		LOG_L(L_WARNING, "Skirmish AI property '%s' could not be found.", key.c_str());
		return DEFAULT_VALUE;
	}

	return strPair->second;
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
void CSkirmishAILibraryInfo::SetLuaAI(const std::string& isLua) {
	SetInfo("isLuaAI", isLua);
}
void CSkirmishAILibraryInfo::SetLuaAI(const bool isLua) {
	SetLuaAI(std::string(isLua ? "yes" : "no"));
}


void CSkirmishAILibraryInfo::SetInterfaceShortName(const std::string& interfaceShortName) {
	SetInfo(SKIRMISH_AI_PROPERTY_INTERFACE_SHORT_NAME, interfaceShortName);
}

void CSkirmishAILibraryInfo::SetInterfaceVersion(const std::string& interfaceVersion) {
	SetInfo(SKIRMISH_AI_PROPERTY_INTERFACE_VERSION, interfaceVersion);
}


bool CSkirmishAILibraryInfo::SetInfo(
	const std::string& key,
	const std::string& value,
	const std::string& description
) {
	static const std::string snKey = StringToLower(SKIRMISH_AI_PROPERTY_SHORT_NAME);
	static const std::string vKey = StringToLower(SKIRMISH_AI_PROPERTY_VERSION);

	const std::string keyLower = StringToLower(key);

	if (keyLower == snKey || keyLower == vKey) {
		if (value.find_first_of(BAD_CHARS) != std::string::npos) {

			LOG_L(
				L_WARNING, "Skirmish AI property (%s or %s) contains illegal characters (%s).",
				SKIRMISH_AI_PROPERTY_SHORT_NAME, SKIRMISH_AI_PROPERTY_VERSION, BAD_CHARS
			);

			return false;
		}
	}

	// only add the key if it is not yet present
	if (info_key_value.find(key) == info_key_value.end())
		info_keys.push_back(key);

	info_keyLower_key[keyLower] = key;
	info_key_value[key] = value;
	info_key_description[key] = description;

	return true;
}

