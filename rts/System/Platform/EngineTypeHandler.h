/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ENGINE_TYPE_HANDLER_H
#define ENGINE_TYPE_HANDLER_H

#include <string>
#include <vector>

namespace EngineTypeHandler {

	struct EngineTypeInfo {
		EngineTypeInfo(const std::string& tname, const std::string& tinfo, const std::string& texe) :
	name(tname), info(tinfo), exe(texe) {}
		std::string name, info, exe;
	};

	EngineTypeInfo* GetEngineTypeInfo(unsigned int enginetype);

	inline unsigned int GetCurrentEngineType() { return 0; } // support for multiple engine types that are not sync compatible

	std::string GetEngine(unsigned short enginetype, unsigned short engineversionmajor, unsigned short engineversionminor, unsigned short enginepatchset, bool brief = false);

	bool RestartEngine(const std::string& exe, std::vector<std::string>& args);

	void SetRestartExecutable(const std::string& exe, const std::string& errmsg);
	
	const std::string& GetRestartExecutable();

	const std::string& GetRestartErrorMessage();

}

#endif // ENGINE_TYPE_HANDLER_H