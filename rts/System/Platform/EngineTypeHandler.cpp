/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "EngineTypeHandler.h"

#include "Game/GameVersion.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/Platform/Misc.h"
#include "System/Util.h"
#include "System/VersionGenerated.h"
#include <boost/format.hpp>

namespace EngineTypeHandler {

//#define CUSTOM_ENGINE_TYPE

	std::vector<EngineTypeInfo> engineTypes;
	std::string restartExecutable;
	std::string restartErrorMessage;
	int reqEngineType = -1;
	int reqEngineMinor = -1;


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

	std::string GetEngine(const EngineTypeVersion &etv, bool brief) {
		std::string minorversion = (etv.minorv == 0) ? "" : boost::str(boost::format("-%d") %(int)etv.minorv);
		EngineTypeInfo* et = GetEngineTypeInfo(etv.type);
		if (et == NULL)
			return boost::str(boost::format("Unknown (%d) %d%s.%d") %(int)etv.type %(int)etv.majorv %minorversion %(int)etv.patch);
		return boost::str(boost::format("%s %d%s.%d%s") %et->name %(int)etv.majorv %minorversion %(int)etv.patch %(brief ? "" : ("   [ " + et->info + " ]")));
	}

	void SetRequestedEngineType(int engineType, int engineMinor) {
		reqEngineType = engineType;
		reqEngineMinor = engineMinor;
		LOG("Server requested engine type: %d, minor version: %d", engineType, engineMinor);
	}

	bool WillRestartEngine(const std::string& message) {
		if ((reqEngineType >= 0 && (reqEngineType != GetCurrentEngineType())) || 
			(reqEngineMinor >= 0 && (reqEngineMinor != GetMinorVersion()))) {
				EngineTypeInfo* eti = GetEngineTypeInfo(reqEngineType);
				if (eti == NULL) {
					LOG_L(L_ERROR, "Unknown engine type: %d", reqEngineType);
				} else {
					std::string newexe = eti->exe;
					if (reqEngineMinor > 0)
						newexe += boost::str(boost::format("-%d") %reqEngineMinor);
					LOG("Preparing to start %s", newexe.c_str());
					SetRestartExecutable(newexe, message);
					return true;
				}
		}
		return false;
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

	const std::string GetMinorModifier()
	{
#if defined(UNITSYNC) || !defined(CUSTOM_ENGINE_TYPE) 
		return "";
#endif
		return (GetMinorVersion() == 0) ? "" : ("-" + GetMinorVersion());
	}

	const std::string& GetVersion() {
		static const std::string base = SpringVersion::IsRelease()
			? SpringVersion::GetMajor() + GetMinorModifier()
			: (SpringVersion::GetMajor() + GetMinorModifier() + "." + SpringVersion::GetPatchSet() + ".1");
		return base;
	}

	const std::string& GetSyncVersion() {
		static const std::string sync = SpringVersion::IsRelease()
			? SpringVersion::GetMajor() + GetMinorModifier()
			: SPRING_VERSION_ENGINE;
		return sync;
	}

}