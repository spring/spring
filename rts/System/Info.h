/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// The structs in this file relate to *Info.lua files.
// They are used for AIs (-> AIInfo.lua) for example.
// This file is used (at least) by unitsync and the engine.

#ifndef _INFO_H
#define _INFO_H

#include "System/FileSystem/VFSModes.h"
#include "LogOutput.h"

#include <string>
#include <vector>
#include <set>

struct InfoItem {
	std::string key;
	std::string value;
	std::string desc;
};

void parseInfo(
		std::vector<InfoItem>& options,
		const std::string& fileName,
		const std::string& fileModes = SPRING_VFS_RAW,
		const std::string& accessModes = SPRING_VFS_RAW,
		std::set<std::string>* infoSet = NULL,
		CLogSubsystem* logSubsystem = &(CLogOutput::GetDefaultLogSubsystem()));

std::vector<InfoItem> parseInfo(
		const std::string& fileName,
		const std::string& fileModes = SPRING_VFS_RAW,
		const std::string& accessModes = SPRING_VFS_RAW,
		std::set<std::string>* infoSet = NULL,
		CLogSubsystem* logSubsystem = &(CLogOutput::GetDefaultLogSubsystem()));

#endif // _INFO_H
