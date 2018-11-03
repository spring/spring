/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Info.h"

#include "System/StringUtil.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "Lua/LuaParser.h"

#include <cassert>

static const char* InfoItem_badKeyChars = " =;\r\n\t";

std::string InfoItem::GetValueAsString(const bool convBooltoInt) const {
	std::string stringValue;

	switch (valueType) {
		case INFO_VALUE_TYPE_STRING: {
			stringValue = valueTypeString;
		} break;
		case INFO_VALUE_TYPE_INTEGER: {
			stringValue = IntToString(value.typeInteger);
		} break;
		case INFO_VALUE_TYPE_FLOAT: {
			stringValue = FloatToString(value.typeFloat);
		} break;
		case INFO_VALUE_TYPE_BOOL: {
			if (convBooltoInt) {
				stringValue = IntToString((int)value.typeBool);
			} else {
				stringValue = (value.typeBool) ? "true" : "false";
			}
		} break;
		default: {
			stringValue = "unknown_error";
		}
	}

	return stringValue;
}

void info_convertToStringValue(InfoItem* infoItem) {

	assert(infoItem != nullptr);

	infoItem->valueTypeString = infoItem->GetValueAsString();
	infoItem->valueType = INFO_VALUE_TYPE_STRING;
}

const char* info_convertTypeToString(InfoValueType infoValueType) {

	const char* typeString = nullptr;

	switch (infoValueType) {
		case INFO_VALUE_TYPE_STRING: {
			typeString = "string";
		} break;
		case INFO_VALUE_TYPE_INTEGER: {
			typeString = "integer";
		} break;
		case INFO_VALUE_TYPE_FLOAT: {
			typeString = "float";
		} break;
		case INFO_VALUE_TYPE_BOOL: {
			typeString = "bool";
		} break;
	}

	return typeString;
}

static bool info_parseInfoItem(const LuaTable& root, int index, InfoItem& inf,
		std::set<string>& infoSet)
{
	const LuaTable& infsTbl = root.SubTable(index);
	if (!infsTbl.IsValid()) {
		LOG_L(L_WARNING, "parseInfoItem: subtable %d invalid", index);
		return false;
	}

	// common info properties
	inf.key = infsTbl.GetString("key", "");
	if (inf.key.empty()
			|| (inf.key.find_first_of(InfoItem_badKeyChars) != string::npos)) {
		LOG_L(L_WARNING,
				"parseInfoItem: empty key or key contains bad characters");
		return false;
	}
	std::string lowerKey = StringToLower(inf.key);
	if (infoSet.find(inf.key) != infoSet.end()) {
		LOG_L(L_WARNING, "parseInfoItem: key toLowerCase(%s) exists already",
				inf.key.c_str());
		return false;
	}
	// TODO add support for info value types other then string
	inf.valueType = INFO_VALUE_TYPE_STRING;
	inf.valueTypeString = infsTbl.GetString("value", "");
	if (inf.valueTypeString.empty()) {
		LOG_L(L_WARNING, "parseInfoItem: %s: empty value", inf.key.c_str());
		return false;
	}
	inf.desc = infsTbl.GetString("desc", "");

	infoSet.insert(lowerKey);

	return true;
}


void info_parseInfo(
		std::vector<InfoItem>& info,
		const std::string& fileName,
		const std::string& fileModes,
		const std::string& accessModes,
		std::set<std::string>* infoSet)
{
	LuaParser luaParser(fileName, fileModes, accessModes);

	if (!luaParser.Execute()) {
		throw content_error("luaParser.Execute() failed: "
				+ luaParser.GetErrorLog());
	}

	const LuaTable root = luaParser.GetRoot();
	if (!root.IsValid()) {
		throw content_error("root table invalid");
	}

	std::set<std::string>* myInfoSet = nullptr;
	if (infoSet == nullptr) {
		myInfoSet = new std::set<std::string>();
	} else {
		myInfoSet = infoSet;
	}
	for (int index = 1; root.KeyExists(index); index++) {
		InfoItem inf;
		if (info_parseInfoItem(root, index, inf, *myInfoSet)) {
			info.push_back(inf);
		}
	}
	if (infoSet == nullptr) {
		delete myInfoSet;
		myInfoSet = nullptr;
	}
}

std::vector<InfoItem> info_parseInfo(
		const std::string& fileName,
		const std::string& fileModes,
		const std::string& accessModes,
		std::set<std::string>* infoSet)
{
	std::vector<InfoItem> info;

	info_parseInfo(info, fileName, fileModes, accessModes, infoSet);

	return info;
}
