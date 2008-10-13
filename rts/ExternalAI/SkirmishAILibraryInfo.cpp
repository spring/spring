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
#include "Interface/SAIInterfaceLibrary.h"
#include "ISkirmishAILibrary.h"

#include "Platform/errorhandler.h"
#include "FileSystem/VFSModes.h"

CSkirmishAILibraryInfo::CSkirmishAILibraryInfo(const ISkirmishAILibrary& ai,
		const SAIInterfaceSpecifier& interfaceSpecifier) {
	info = ai.GetInfo();
	options = ai.GetOptions();
	//levelOfSupport = ai.GetLevelOfSupportFor(std::string(ENGINE_VERSION_STRING),
	//		ENGINE_VERSION_NUMBER, interfaceSpecifier);
}

CSkirmishAILibraryInfo::CSkirmishAILibraryInfo(
		const CSkirmishAILibraryInfo& aiInfo) {
	info = std::map<std::string, InfoItem>(
			aiInfo.info.begin(),
			aiInfo.info.end());
	options = std::vector<Option>(
			aiInfo.options.begin(),
			aiInfo.options.end());
	//levelOfSupport = aiInfo.levelOfSupport;
}

CSkirmishAILibraryInfo::CSkirmishAILibraryInfo(
		const std::string& aiInfoFile,
		const std::string& aiOptionFile) {
	
	InfoItem tmpInfo[MAX_INFOS];
	unsigned int num = ParseInfo(aiInfoFile.c_str(), SPRING_VFS_RAW,
			SPRING_VFS_RAW, tmpInfo, MAX_INFOS);
    for (unsigned int i=0; i < num; ++i) {
		info[std::string(tmpInfo[i].key)] = tmpInfo[i];
    }
	
	if (!aiOptionFile.empty()) {
		Option tmpOptions[MAX_OPTIONS];
		num = ParseOptions(aiOptionFile.c_str(), SPRING_VFS_RAW, SPRING_VFS_RAW,
				"", tmpOptions, MAX_OPTIONS);
		for (unsigned int i=0; i < num; ++i) {
			options.push_back(tmpOptions[i]);
		}
	}
	//options = ParseOptions(aiOptionFile, fileModes, accessModes);
}

/*
LevelOfSupport CSkirmishAILibraryInfo::GetLevelOfSupportForCurrentEngineAndSetInterface() const {
	return levelOfSupport;
}
*/

SSAISpecifier CSkirmishAILibraryInfo::GetSpecifier() const {
	
	const char* sn = info.at(SKIRMISH_AI_PROPERTY_SHORT_NAME).value;
	const char* v = info.at(SKIRMISH_AI_PROPERTY_VERSION).value;
	SSAISpecifier specifier = {sn, v};
	return specifier;
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
	return info.at(key).value;
}
const std::map<std::string, InfoItem>* CSkirmishAILibraryInfo::GetInfo() const {
	return &info;
}

const std::vector<Option>* CSkirmishAILibraryInfo::GetOptions() const {
	return &options;
}


unsigned int CSkirmishAILibraryInfo::GetInfoCReference(InfoItem cInfo[],
		unsigned int maxInfoItems) const {
	
	unsigned int i=0;
	
	std::map<std::string, InfoItem>::const_iterator infs;
	for (infs=info.begin(); infs != info.end() && i < maxInfoItems; ++infs) {
		cInfo[i++] = infs->second;
    }
	
	return i;
}
unsigned int CSkirmishAILibraryInfo::GetOptionsCReference(Option cOptions[],
		unsigned int maxOptions) const {
	
	unsigned int i=0;
	
	std::vector<Option>::const_iterator ops;
	for (ops=options.begin(); ops != options.end() && i < maxOptions; ++ops) {
		cOptions[i++] = *ops;
    }
	
	return i;
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
void CSkirmishAILibraryInfo::SetInterfaceShortName(const std::string& interfaceShortName) {
	SetInfo(SKIRMISH_AI_PROPERTY_INTERFACE_SHORT_NAME, interfaceShortName);
}
void CSkirmishAILibraryInfo::SetInterfaceVersion(const std::string& interfaceVersion) {
	SetInfo(SKIRMISH_AI_PROPERTY_INTERFACE_VERSION, interfaceVersion);
}
bool CSkirmishAILibraryInfo::SetInfo(const std::string& key, const std::string& value) {
	
	if (key == SKIRMISH_AI_PROPERTY_SHORT_NAME ||
			key == SKIRMISH_AI_PROPERTY_VERSION) {
		if (value.find_first_of("\t _#") != std::string::npos) {
			handleerror(NULL, "Error", "Skirmish AI info (shortName or version) contains illegal characters ('_', '#' or white spaces)", MBF_OK | MBF_EXCL);
			return false;
		}
	}
	
	InfoItem ii = {key.c_str(), value.c_str(), NULL};
	info[key] = ii;
	return true;
}

void CSkirmishAILibraryInfo::SetOptions(const std::vector<Option>& _options) {
	options = std::vector<Option>(_options.begin(), _options.end()); // implicit convertible types -> range-ctor can be used
}
