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

#include "GroupAILibraryInfo.h"

#include "Interface/aidefines.h"
#include "Interface/SGAILibrary.h"
#include "Interface/SAIInterfaceLibrary.h"
#include "IGroupAILibrary.h"

#include "Platform/errorhandler.h"
#include "FileSystem/VFSModes.h"

/*
CGroupAILibraryInfo::CGroupAILibraryInfo(const IGroupAILibrary& ai,
		const SAIInterfaceSpecifier& interfaceSpecifier) {
	info = ai.GetInfo();
	options = ai.GetOptions();
	//levelOfSupport = ai.GetLevelOfSupportFor(std::string(ENGINE_VERSION_STRING),
	//		ENGINE_VERSION_NUMBER, interfaceSpecifier);
}
*/

CGroupAILibraryInfo::CGroupAILibraryInfo(const CGroupAILibraryInfo& aiInfo) {
	info = std::map<std::string, InfoItem>(
			aiInfo.info.begin(),
			aiInfo.info.end());
	options = std::vector<Option>(
			aiInfo.options.begin(),
			aiInfo.options.end());
	//levelOfSupport = aiInfo.levelOfSupport;
}

CGroupAILibraryInfo::CGroupAILibraryInfo(
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
}

/*
LevelOfSupport CGroupAILibraryInfo::GetLevelOfSupportForCurrentEngine() const {
	return levelOfSupport;
}
*/

SGAISpecifier CGroupAILibraryInfo::GetSpecifier() const {
	
	const char* sn = info.at(GROUP_AI_PROPERTY_SHORT_NAME).value;
	const char* v = info.at(GROUP_AI_PROPERTY_VERSION).value;
	SGAISpecifier specifier = {sn, v};
	return specifier;
}

std::string CGroupAILibraryInfo::GetFileName() const {
	return GetInfo(GROUP_AI_PROPERTY_FILE_NAME);
}
std::string CGroupAILibraryInfo::GetShortName() const { // restrictions: none of the following: spaces, '_', '#'
	return GetInfo(GROUP_AI_PROPERTY_SHORT_NAME);
}
std::string CGroupAILibraryInfo::GetVersion() const { // restrictions: none of the following: spaces, '_', '#'
	return GetInfo(GROUP_AI_PROPERTY_VERSION);
}
std::string CGroupAILibraryInfo::GetName() const {
	return GetInfo(GROUP_AI_PROPERTY_NAME);
}
std::string CGroupAILibraryInfo::GetDescription() const {
	return GetInfo(GROUP_AI_PROPERTY_DESCRIPTION);
}
std::string CGroupAILibraryInfo::GetURL() const {
	return GetInfo(GROUP_AI_PROPERTY_URL);
}
std::string CGroupAILibraryInfo::GetInterfaceShortName() const {
	return GetInfo(GROUP_AI_PROPERTY_INTERFACE_SHORT_NAME);
}
std::string CGroupAILibraryInfo::GetInterfaceVersion() const {
	return GetInfo(GROUP_AI_PROPERTY_INTERFACE_VERSION);
}
std::string CGroupAILibraryInfo::GetInfo(const std::string& key) const {
	return info.at(key).value;
}
const std::map<std::string, InfoItem>* CGroupAILibraryInfo::GetInfo() const {
	return &info;
}

const std::vector<Option>* CGroupAILibraryInfo::GetOptions() const {
	return &options;
}


unsigned int CGroupAILibraryInfo::GetInfoCReference(InfoItem cInfo[],
		unsigned int maxInfoItems) const {
	
	unsigned int i=0;
	
	std::map<std::string, InfoItem>::const_iterator infs;
	for (infs=info.begin(); infs != info.end() && i < maxInfoItems; ++infs) {
		cInfo[i++] = infs->second;
    }
	
	return i;
}
unsigned int CGroupAILibraryInfo::GetOptionsCReference(Option cOptions[],
		unsigned int maxOptions) const {
	
	unsigned int i=0;
	
	std::vector<Option>::const_iterator ops;
	for (ops=options.begin(); ops != options.end() && i < maxOptions; ++ops) {
		cOptions[i++] = *ops;
    }
	
	return i;
}


void CGroupAILibraryInfo::SetFileName(const std::string& fileName) {
	SetInfo(GROUP_AI_PROPERTY_FILE_NAME, fileName);
}
void CGroupAILibraryInfo::SetShortName(const std::string& shortName) { // restrictions: none of the following: spaces, '_', '#'
	SetInfo(GROUP_AI_PROPERTY_SHORT_NAME, shortName);
}
void CGroupAILibraryInfo::SetVersion(const std::string& version) { // restrictions: none of the following: spaces, '_', '#'
	SetInfo(GROUP_AI_PROPERTY_VERSION, version);
}
void CGroupAILibraryInfo::SetName(const std::string& name) {
	SetInfo(GROUP_AI_PROPERTY_NAME, name);
}
void CGroupAILibraryInfo::SetDescription(const std::string& description) {
	SetInfo(GROUP_AI_PROPERTY_DESCRIPTION, description);
}
void CGroupAILibraryInfo::SetURL(const std::string& url) {
	SetInfo(GROUP_AI_PROPERTY_URL, url);
}
void CGroupAILibraryInfo::SetInterfaceShortName(
		const std::string& interfaceShortName) {
	SetInfo(GROUP_AI_PROPERTY_INTERFACE_SHORT_NAME, interfaceShortName);
}
void CGroupAILibraryInfo::SetInterfaceVersion(
		const std::string& interfaceVersion) {
	SetInfo(GROUP_AI_PROPERTY_INTERFACE_VERSION, interfaceVersion);
}
bool CGroupAILibraryInfo::SetInfo(const std::string& key,
		const std::string& value) {
	
	if (key == GROUP_AI_PROPERTY_SHORT_NAME ||
			key == GROUP_AI_PROPERTY_VERSION) {
		if (value.find_first_of("\t _#") != std::string::npos) {
			handleerror(NULL, "Error",
					"Group AI info (shortName or version) contains"
					" illegal characters ('_', '#' or white spaces)",
					MBF_OK | MBF_EXCL);
			return false;
		}
	}
	
	InfoItem ii = {key.c_str(), value.c_str(), NULL};
	info[key] = ii;
	return true;
}

void CGroupAILibraryInfo::SetOptions(const std::vector<Option>& _options) {
	// implicit convertible types -> range-ctor can be used
	options = std::vector<Option>(_options.begin(), _options.end());
}
