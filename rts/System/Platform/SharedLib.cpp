/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#if defined BUILDING_AI
#include "System/StringUtil.h"
#else // defined BUILDING_AI
#include "System/Log/ILog.h"
#endif // defined BUILDING_AI

// OS dependent stuff
#ifdef _WIN32
	#include "Win/DllLib.h"
	#define SL_IMPL_CLS DllLib
	#define SL_IMPL_EXT "dll"
#else // _WIN32
	#include "Linux/SoLib.h"
	#define SL_IMPL_CLS SoLib
	#if defined __APPLE__
		#define SL_IMPL_EXT "dylib"
	#else // defined __APPLE__
		#define SL_IMPL_EXT "so"
	#endif // defined __APPLE__
#endif // _WIN32

/**
 * Used to create a platform-specific shared library handler.
 */
SharedLib* SharedLib::Instantiate(const char* fileName)
{
	SharedLib* lib = nullptr;

	lib = new SL_IMPL_CLS(fileName);

	if (lib == nullptr || lib->LoadFailed()) {
		// loading failed
		delete lib;
		lib = nullptr;
	}

	return lib;
}

/**
 * Used to create a platform-specific shared library handler.
 */
SharedLib* SharedLib::Instantiate(const std::string& fileName)
{
	return Instantiate(fileName.c_str());
}


const char* SharedLib::GetLibExtension() 
{
	return SL_IMPL_EXT;
}

void SharedLib::reportError(const char* errorMsg, const char* fileName, int lineNumber, const char* function) {

#if defined BUILDING_AI
	// when used by an AI Interface eg, we define a macro, eg. this:
	// EXTERNAL_LOGGER(msg) printf(msg);
	// so we can log SharedLib errors/failures in our AI if we want so.
	// see the C AI Interface for an example
	// Note: the msg passed to the macro is of type: const char*
	#if defined EXTERNAL_LOGGER
		const int MAX_MSG_LENGTH = 511;
		char s_msg[MAX_MSG_LENGTH + 1];
		SNPRINTF(s_msg, MAX_MSG_LENGTH, "%s:%d: %s: %s", fileName, lineNumber, function, errorMsg);
		EXTERNAL_LOGGER(s_msg)
	#else // defined EXTERNAL_LOGGER
		// DO NOTHING
	#endif // defined EXTERNAL_LOGGER
#else // defined BUILDING_AI
	LOG_L(L_ERROR, "%s:%d: %s: %s", fileName, lineNumber, function, errorMsg);
#endif // defined BUILDING_AI
}

SharedLib::~SharedLib() = default; // subclasses will call Unload in their destructors.

#undef SL_IMPL_CLS
#undef SL_IMPL_EXT
