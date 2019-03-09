/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COB_FILE_H
#define COB_FILE_H

#include <array>
#include <vector>
#include <string>

#include "Lua/LuaHashString.h"
#include "CobScriptNames.h"
#include "System/UnorderedMap.hpp"

class CFileHandler;

class CCobFile
{
public:
	CCobFile(CFileHandler& in, const std::string& scriptName);
	CCobFile(CCobFile&& f) { *this = std::move(f); }

	CCobFile& operator = (CCobFile&& f) {
		numStaticVars = f.numStaticVars;

		code = std::move(f.code);
		scriptNames = std::move(f.scriptNames);
		scriptOffsets = std::move(f.scriptOffsets);

		scriptLengths = std::move(f.scriptLengths);
		pieceNames = std::move(f.pieceNames);
		scriptIndex = std::move(f.scriptIndex);
		sounds = std::move(f.sounds);
		luaScripts = std::move(f.luaScripts);
		scriptMap = std::move(f.scriptMap);

		name = std::move(f.name);
		return *this;
	}

	int GetFunctionId(const std::string& name);

public:
	int numStaticVars = 0;

	std::vector<int> code;
	std::vector<std::string> scriptNames;
	std::vector<int> scriptOffsets;
	/// Assumes that the scripts are sorted by offset in the file
	std::vector<int> scriptLengths;
	std::vector<std::string> pieceNames;
	std::array<int, COBFN_NumUnitFuncs> scriptIndex;
	std::vector<int> sounds;
	std::vector<LuaHashString> luaScripts;
	spring::unordered_map<std::string, int> scriptMap;

	std::string name;
};

#endif // COB_FILE_H

