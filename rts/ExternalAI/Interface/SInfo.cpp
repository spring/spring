/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "SInfo.h"

#include "System/Util.h"
#include <string.h>
#include <stdlib.h>

struct InfoItem copyInfoItem(const struct InfoItem* const orig) {
	
	struct InfoItem copy;
	
	copy.key = mallocCopyString(orig->key);
	
	copy.value = mallocCopyString(orig->value);
	
	if (orig->desc != NULL) {
		copy.desc = mallocCopyString(orig->desc);
	} else {
		copy.desc = NULL;
	}
	
	return copy;
}
void deleteInfoItem(const struct InfoItem* const info) {
	
	freeString(info->key);
	freeString(info->value);
	freeString(info->desc);
	
	InfoItem* const mutableInfo = const_cast<InfoItem* const>(info);
	mutableInfo->key = NULL;
	mutableInfo->value = NULL;
	mutableInfo->desc = NULL;
}

#if	defined(__cplusplus) && !defined(BUILDING_AI) && !defined(BUILDING_AI_INTERFACE)

#include "Lua/LuaParser.h"

#include <set>

static const char* badKeyChars = " =;\r\n\t";

bool ParseInfo(const LuaTable& root, int index, InfoItem& info, std::set<std::string> infoSet)
{
	const LuaTable& infoTbl = root.SubTable(index);
	if (!infoTbl.IsValid()) {
		return false;
	}

	// info properties
	std::string info_key = infoTbl.GetString("key", "");
	if (info_key.empty() ||
	    (info_key.find_first_of(badKeyChars) != std::string::npos)) {
		return false;
	}
	std::string keyLower = StringToLower(info_key);
	if (infoSet.find(keyLower) != infoSet.end()) {
		return false;
	}
	info.key = mallocCopyString(info_key.c_str());
	
	std::string info_value = infoTbl.GetString("value", "");
	if (info_value.empty()) {
		return false;
	}
	info.value = mallocCopyString(info_value.c_str());
	
	std::string info_desc = infoTbl.GetString("desc", "");
	if (info_desc.empty()) {
		info.desc = NULL;
	} else {
		info.desc = mallocCopyString(info_desc.c_str());
	}

	infoSet.insert(keyLower);

	return true;
}

unsigned int ParseInfo(
		const char* fileName,
		const char* fileModes,
		const char* accessModes,
		InfoItem info[], unsigned int max)
{
	LuaParser luaParser(fileName, fileModes, accessModes);
		
	if (!luaParser.Execute()) {
		printf("ParseInfo(%s) ERROR: %s\n",
		       fileName, luaParser.GetErrorLog().c_str());
		return 0;
	}

	const LuaTable root = luaParser.GetRoot();
	if (!root.IsValid()) {
		return 0;
	}

	unsigned int i = 0;
	std::set<std::string> infoSet;
	for (int index = 1; root.KeyExists(index) && i < max; index++) {
		bool parseOk = ParseInfo(root, index, info[i], infoSet);
		if (!parseOk) {
			info[i].key = NULL;
			info[i].value = NULL;
			info[i].desc = NULL;
		}
		i++;
	}
	
	return i;
}
unsigned int ParseInfoRawFileSystem(
		const char* fileName,
		InfoItem info[], unsigned int max) {
	return ParseInfo(fileName, SPRING_VFS_RAW, SPRING_VFS_RAW, info, max);
}

#endif // defined(__cplusplus) && !defined(BUILDING_AI) && !defined(BUILDING_AI_INTERFACE)
