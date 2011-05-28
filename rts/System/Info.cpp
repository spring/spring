/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Info.h"

#include "System/Util.h"
#include "System/Exceptions.h"
#include "Lua/LuaParser.h"

#include <assert.h>

static const char* InfoItem_badKeyChars = " =;\r\n\t";

std::string info_getValueAsString(const InfoItem* infoItem) {

	assert(infoItem != NULL);

	std::string stringValue = "";

	switch (infoItem->valueType) {
		case INFO_VALUE_TYPE_STRING: {
			stringValue = infoItem->valueTypeString;
		} break;
		case INFO_VALUE_TYPE_INTEGER: {
			stringValue = IntToString(infoItem->value.typeInteger);
		} break;
		case INFO_VALUE_TYPE_FLOAT: {
			stringValue = FloatToString(infoItem->value.typeFloat);
		} break;
		case INFO_VALUE_TYPE_BOOL: {
			stringValue = IntToString((int) infoItem->value.typeBool);
		} break;
	}

	return stringValue;
}

void info_convertToStringValue(InfoItem* infoItem) {

	assert(infoItem != NULL);

	infoItem->valueTypeString = info_getValueAsString(infoItem);
	infoItem->valueType = INFO_VALUE_TYPE_STRING;
}

const char* info_convertTypeToString(InfoValueType infoValueType) {

	const char* typeString = NULL;

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

static bool parseInfoItem(const LuaTable& root, int index, InfoItem& inf,
		std::set<string>& infoSet, CLogSubsystem& logSubsystem)
{
	const LuaTable& infsTbl = root.SubTable(index);
	if (!infsTbl.IsValid()) {
		logOutput.Print(logSubsystem,
				"parseInfoItem: subtable %d invalid", index);
		return false;
	}

	// common info properties
	inf.key = infsTbl.GetString("key", "");
	if (inf.key.empty()
			|| (inf.key.find_first_of(InfoItem_badKeyChars) != string::npos)) {
		logOutput.Print(logSubsystem,
				"parseInfoItem: empty key or key contains bad characters");
		return false;
	}
	std::string lowerKey = StringToLower(inf.key);
	if (infoSet.find(inf.key) != infoSet.end()) {
		logOutput.Print(logSubsystem, "parseInfoItem: key toLowerCase(%s) exists already",
				inf.key.c_str());
		return false;
	}
	// TODO add support for info value types other then string
	inf.valueType = INFO_VALUE_TYPE_STRING;
	inf.valueTypeString = infsTbl.GetString("value", inf.key);
	if (inf.valueTypeString.empty()) {
		logOutput.Print(logSubsystem, "parseInfoItem: %s: empty value",
				inf.key.c_str());
		return false;
	}
	inf.desc = infsTbl.GetString("desc", "");

	infoSet.insert(lowerKey);

	return true;
}


void parseInfo(
		std::vector<InfoItem>& info,
		const std::string& fileName,
		const std::string& fileModes,
		const std::string& accessModes,
		std::set<std::string>* infoSet,
		CLogSubsystem* logSubsystem) {

	if (!logSubsystem) {
		assert(logSubsystem);
	}

	LuaParser luaParser(fileName, fileModes, accessModes);

	if (!luaParser.Execute()) {
		throw content_error("luaParser.Execute() failed: "
				+ luaParser.GetErrorLog());
	}

	const LuaTable root = luaParser.GetRoot();
	if (!root.IsValid()) {
		throw content_error("root table invalid");
	}

	std::set<std::string>* myInfoSet = NULL;
	if (infoSet == NULL) {
		myInfoSet = new std::set<std::string>();
	} else {
		myInfoSet = infoSet;
	}
	for (int index = 1; root.KeyExists(index); index++) {
		InfoItem inf;
		if (parseInfoItem(root, index, inf, *myInfoSet, *logSubsystem)) {
			info.push_back(inf);
		}
	}
	if (infoSet == NULL) {
		delete myInfoSet;
		myInfoSet = NULL;
	}
}

std::vector<InfoItem> parseInfo(
		const std::string& fileName,
		const std::string& fileModes,
		const std::string& accessModes,
		std::set<std::string>* infoSet,
		CLogSubsystem* logSubsystem) {

	std::vector<InfoItem> info;

	parseInfo(info, fileName, fileModes, accessModes, infoSet,
			logSubsystem);

	return info;
}
