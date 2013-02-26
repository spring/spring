/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "EngineTypeHandler.h"

#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/Platform/Misc.h"
#include "System/Util.h"
#include <boost/format.hpp>

namespace EngineTypeHandler {

	std::vector<EngineTypeInfo> engineTypes;
	std::string restartExecutable;
	std::string restartErrorMessage;
	const unsigned short ENGINE_TYPE = 0; // support for multiple engine types that are not sync compatible

	bool Init() {
		engineTypes.push_back(EngineTypeInfo("Spring", "http://springrts.com/wiki/Download", "spring")); // 0
		engineTypes.push_back(EngineTypeInfo("Spring MT", "http://springrts.com/wiki/Spring_MT", "spring-mt")); // 1
		// add more engine types here
		return true;
	}
	bool ethdummy = Init();

	EngineTypeInfo* GetEngineTypeInfo(unsigned int enginetype) {
		if (enginetype >= engineTypes.size())
			return NULL;
		return &engineTypes[enginetype];
	}

	unsigned int GetCurrentEngineType() { return ENGINE_TYPE; }

	std::string GetEngine(unsigned short enginetype, unsigned short engineversionmajor, unsigned short engineversionminor, unsigned short enginepatchset, bool brief) {
		std::string minorversion = (engineversionminor == 0) ? "" : boost::str(boost::format("-%d") %(int)engineversionminor);
		EngineTypeInfo* et = GetEngineTypeInfo(enginetype);
		if (et == NULL)
			return boost::str(boost::format("Unknown (%d) %d%s.%d") %(int)enginetype %(int)engineversionmajor %minorversion %(int)enginepatchset);
		return boost::str(boost::format("%s %d%s.%d%s") %et->name %(int)engineversionmajor %minorversion %(int)enginepatchset %(brief ? "" : ("   [ " + et->info + " ]")));
	}

	bool RestartEngine(const std::string& exe, std::vector<std::string>& args) {
		const std::string springExeDir = Platform::GetProcessExecutablePath();
		std::string springExeFile = exe;
		const std::string curExe = FileSystem::GetFilename(Platform::GetProcessExecutableFile());
#ifdef WIN32
		springExeFile += std::string(".exe"); 
		for (int i = 0; i < args.size(); ++i) {
			if (args[i].find('\"') == std::string::npos) {
				args[i] = "\"" + args[i] + "\"";
			}
		}
		if (StringToLower(curExe) == StringToLower(springExeFile))
			return false;
#else // prevent infinite loop, restarting the same engine over and over again
		if (curExe == springExeFile)
			return false;
#endif

		const std::string execError = Platform::ExecuteProcess(springExeDir + springExeFile, args);

		return execError.empty();
	}

	void SetRestartExecutable(const std::string& exe, const std::string& errmsg) {
		restartExecutable = exe;
		restartErrorMessage = errmsg;
	}
	
	const std::string& GetRestartExecutable() {
		return restartExecutable;
	}

	const std::string& GetRestartErrorMessage() {
		return restartErrorMessage;
	}

}