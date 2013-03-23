/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ENGINE_TYPE_HANDLER_H
#define ENGINE_TYPE_HANDLER_H

#include "Game/GameVersion.h"
#include <string>
#include <vector>
#include <stdio.h>

namespace EngineTypeHandler {

	struct EngineTypeInfo {
		EngineTypeInfo(const std::string& tname, const std::string& tinfo, const std::string& texe) :
	name(tname), info(tinfo), exe(texe) {}
		std::string name, info, exe;
	};
	struct EngineTypeVersion {
		EngineTypeVersion() : type(-1), majorv(-1), minorv(-1), patch(-1) {}
		EngineTypeVersion(unsigned int tp, unsigned short maj, unsigned short min, unsigned short pat) : type(tp), majorv(maj), minorv(min), patch(pat) {}
		bool operator==(const EngineTypeVersion& other) const { return type == other.type && majorv == other.majorv && minorv == other.minorv; }
		bool operator!=(const EngineTypeVersion& other) const { return !(*this == other); }
		unsigned short type, majorv, minorv, patch;
	};

	EngineTypeInfo* GetEngineTypeInfo(unsigned int enginetype);

	inline unsigned int GetCurrentEngineType() { return 0; } // support for multiple engine types that are not sync compatible

	std::string GetEngine(const EngineTypeVersion &etv, bool brief = false);

	void SetRequestedEngineType(int engineType, int engineMinor);

	bool WillRestartEngine(const std::string& message);

	bool RestartEngine(const std::string& exe, std::vector<std::string>& args);

	void SetRestartExecutable(const std::string& exe, const std::string& errmsg);
	
	const std::string& GetRestartExecutable();

	const std::string& GetRestartErrorMessage();

	inline int GetMajorVersion() {
		int major = 0;
		sscanf(SpringVersion::GetMajor().c_str(), "%d", &major);
		return major;
	}

	inline int GetMinorVersion()	{
		int minor = 0;
		sscanf(SpringVersion::GetMinor().c_str(), "%d", &minor);
		return minor;
	}

	inline int GetPatchSet() {
		int patchSet = 0;
		sscanf(SpringVersion::GetPatchSet().c_str(), "%d", &patchSet);
		return patchSet;
	}

	inline EngineTypeVersion GetCurrentEngineTypeVersion() { return EngineTypeVersion(GetCurrentEngineType(), GetMajorVersion(), GetMinorVersion(), GetPatchSet()); }

	const std::string& GetVersion();
	const std::string& GetSyncVersion();
}

#endif // ENGINE_TYPE_HANDLER_H