/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// The structs in this file relate to *Info.lua files.
// They are used for AIs (-> AIInfo.lua) for example.
// This file is used (at least) by unitsync and the engine.

#ifndef _INFO_H
#define _INFO_H

#include "System/FileSystem/VFSModes.h"

#include <string>
#include <vector>
#include <set>

enum InfoValueType {
	INFO_VALUE_TYPE_STRING,
	INFO_VALUE_TYPE_INTEGER,
	INFO_VALUE_TYPE_FLOAT,
	INFO_VALUE_TYPE_BOOL,
};

struct InfoItem {
	InfoItem()
		: valueType(INFO_VALUE_TYPE_STRING)
	{
	}
	InfoItem(const std::string& _key, const std::string& _desc, const std::string _value)
		: key(_key)
		, desc(_desc)
		, valueType(INFO_VALUE_TYPE_STRING)
		, valueTypeString(_value)
	{}

	InfoItem(const std::string& _key, const std::string& _desc, int _value)
		: key(_key)
		, desc(_desc)

		, valueType(INFO_VALUE_TYPE_INTEGER)
	{
		value.typeInteger = _value;
	}

	InfoItem(const std::string& _key, const std::string& _desc, float _value)
		: key(_key)
		, desc(_desc)
		, valueType(INFO_VALUE_TYPE_FLOAT)
	{
		value.typeFloat = _value;
	}

	InfoItem(const std::string& _key, const std::string& _desc, bool _value)
		: key(_key)
		, desc(_desc)
		, valueType(INFO_VALUE_TYPE_BOOL)
	{
		value.typeBool = _value;
	}

public:
	std::string key;
	std::string desc;
	InfoValueType valueType;
	union Value {
		int            typeInteger;
		float          typeFloat;
		bool           typeBool;
	} value;
	/** It is not possible to use a type with destructor in a union */
	std::string valueTypeString;

public:
	std::string GetValueAsString(const bool convBooltoInt = true) const;
};

void info_convertToStringValue(InfoItem* infoItem);

const char* info_convertTypeToString(InfoValueType infoValueType);

void info_parseInfo(
		std::vector<InfoItem>& options,
		const std::string& fileName,
		const std::string& fileModes = SPRING_VFS_RAW,
		const std::string& accessModes = SPRING_VFS_RAW,
		std::set<std::string>* infoSet = NULL);

std::vector<InfoItem> info_parseInfo(
		const std::string& fileName,
		const std::string& fileModes = SPRING_VFS_RAW,
		const std::string& accessModes = SPRING_VFS_RAW,
		std::set<std::string>* infoSet = NULL);

#endif // _INFO_H
