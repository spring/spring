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

// The structs in this files relate to *Info.lua files
// They are used for AIs (-> AIInfo.lua) for example;
// This file is used (at least) by unitsync and the engine

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
