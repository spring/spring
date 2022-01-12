/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AIInterfaceLibraryInfo.h"

#include "AIInterfaceKey.h"
#include "Interface/aidefines.h"
#include "Interface/SAIInterfaceLibrary.h"
#include "System/StringUtil.h"
#include "System/Info.h"
#include "System/Log/ILog.h"

#include "System/FileSystem/VFSModes.h"


static const char* BAD_CHARS = "\t _#";
static const std::string DEFAULT_VALUE;


CAIInterfaceLibraryInfo::CAIInterfaceLibraryInfo(
		const CAIInterfaceLibraryInfo& interfaceInfo) = default;

CAIInterfaceLibraryInfo::CAIInterfaceLibraryInfo(
		const std::string& interfaceInfoFile) {

	std::vector<InfoItem> tmpInfo;
	info_parseInfo(tmpInfo, interfaceInfoFile);
	std::vector<InfoItem>::iterator ii;
	for (ii = tmpInfo.begin(); ii != tmpInfo.end(); ++ii) {
		// TODO remove this, once we support non-string value types for AI Interface info
		info_convertToStringValue(&(*ii));
		SetInfo(ii->key, ii->valueTypeString, ii->desc);
	}
}

CAIInterfaceLibraryInfo::~CAIInterfaceLibraryInfo() = default;

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
const std::string& CAIInterfaceLibraryInfo::GetDataDirCommon() const {
	return GetInfo(AI_INTERFACE_PROPERTY_DATA_DIR_COMMON);
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
bool CAIInterfaceLibraryInfo::IsLookupSupported() const {

	bool lookupSupported = false;

	const std::string& lookupSupportedStr = GetInfo(AI_INTERFACE_PROPERTY_SUPPORTS_LOOKUP);
	if (lookupSupportedStr != DEFAULT_VALUE) {
		lookupSupported = StringToBool(lookupSupportedStr);
	}

	return lookupSupported;
}
const std::string& CAIInterfaceLibraryInfo::GetInfo(const std::string& key) const {

	std::map<std::string, std::string>::const_iterator strPair;

	// get real key through lower case key
	strPair = keyLower_key.find(StringToLower(key));
	bool found = (strPair != keyLower_key.end());

	// get value
	if (found) {
		strPair = key_value.find(strPair->second);
		found = (strPair != key_value.end());
	}

	if (!found) {
		LOG_L(L_WARNING, "AI Interface property '%s' could not be found.",
				key.c_str());
		return DEFAULT_VALUE;
	} else {
		return strPair->second;
	}
}


void CAIInterfaceLibraryInfo::SetDataDir(const std::string& dataDir) {
	SetInfo(AI_INTERFACE_PROPERTY_DATA_DIR, dataDir);
}
void CAIInterfaceLibraryInfo::SetDataDirCommon(const std::string& dataDirCommon) {
	SetInfo(AI_INTERFACE_PROPERTY_DATA_DIR_COMMON, dataDirCommon);
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
		LOG_L(L_WARNING, "AI interface property (%s or %s)\n"
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
