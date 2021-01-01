/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// IMPORTANT NOTE: external systems sed -i this file, so DO NOT CHANGE without
// major thought in advance, and deliberation with bibim and tvo/Tobi!

#include "GameVersion.h"

#include "System/VersionGenerated.h"

#include <ciso646> // _LIBCPP*
#include <cstring>
#include <cstdio>

/**
 * @brief Defines the current version string.
 * Take special care when moving this file.
 * The build-bot refers to this file to append the exact SCM version.
 */
namespace SpringVersion
{

const std::string& GetMajor()
{
	static const std::string major = SPRING_VERSION_ENGINE_MAJOR;
	return major;
}

const std::string& GetMinor()
{
	static const std::string minor = "0";
	return minor;
}

const std::string& GetPatchSet()
{
	static const std::string patchSet = SPRING_VERSION_ENGINE_PATCH_SET;
	return patchSet;
}

const std::string& GetCommits()
{
	static const std::string patchSet = SPRING_VERSION_ENGINE_COMMITS;
	return patchSet;
}

const std::string& GetHash()
{
	static const std::string patchSet = SPRING_VERSION_ENGINE_HASH;
	return patchSet;
}

const std::string& GetBranch()
{
	static const std::string patchSet = SPRING_VERSION_ENGINE_BRANCH;
	return patchSet;
}

inline const std::string CreateAdditionalVersion()
{
	std::string additional = SPRING_VERSION_ENGINE_ADDITIONAL;
	additional += additional.empty() ? "" : " ";

	additional += ""
#define GV_ADD_SPACE ""

#if defined DEBUG
	GV_ADD_SPACE "Debug"
	#undef  GV_ADD_SPACE
	#define GV_ADD_SPACE " "
#endif

#if defined PROFILE
	GV_ADD_SPACE "Profile"
	#undef  GV_ADD_SPACE
	#define GV_ADD_SPACE " "
#endif

#if defined TRACE_SYNC
	GV_ADD_SPACE "Sync-Trace"
	#undef  GV_ADD_SPACE
	#define GV_ADD_SPACE " "
#endif

#if defined SYNCDEBUG
	GV_ADD_SPACE "Sync-Debug"
	#undef  GV_ADD_SPACE
	#define GV_ADD_SPACE " "
#endif

#if !defined SYNCCHECK
	GV_ADD_SPACE "Sync-Check-Disabled"
	#undef  GV_ADD_SPACE
	#define GV_ADD_SPACE " "
#endif

#if defined  __SUPPORT_SNAN__
	GV_ADD_SPACE "Signal-NaNs"
	#undef  GV_ADD_SPACE
	#define GV_ADD_SPACE " "
#endif

#if defined HEADLESS && !defined DEDICATED
	GV_ADD_SPACE "Headless"
	#undef  GV_ADD_SPACE
	#define GV_ADD_SPACE " "
#endif

#if defined DEDICATED
	GV_ADD_SPACE "Dedicated"
	#undef GV_ADD_SPACE
	#define GV_ADD_SPACE " "
#endif

#if defined UNITSYNC
	GV_ADD_SPACE "Unitsync"
	#undef  GV_ADD_SPACE
	#define GV_ADD_SPACE " "
#endif
	;

	return additional;
}

const std::string& GetAdditional()
{
	const static std::string additional(CreateAdditionalVersion());

	return additional;
}

#define QUOTEME_(x) #x
#define QUOTEME(x) QUOTEME_(x)

const std::string& GetCompiler()
{
	static const std::string compiler = ""
#ifdef __GNUC__
	"gcc-" __VERSION__;
#elif defined(__clang__)
	"clang-" __clang_version__ ;
#elif defined(_MSC_VER)
	#ifdef _MSC_FULL_VER
		"msvc-" QUOTEME(_MSC_FULL_VER);
	#else
		"msvc-" QUOTEME(_MSC_VER);
	#endif
#elif defined(__VERSION__)
	"unknown-" __VERSION__;
#else
	"unknown";
#endif
	return compiler;
}

const std::string& GetBuildEnvironment()
{
#if (defined(__GNUC__) || defined(__clang__))
	#if (defined(_LIBCPP_VERSION))
		static const std::string environment = "clang libc++ version " QUOTEME(_LIBCPP_VERSION);
	#elif (defined(__GLIBCXX__) || defined(__GLIBCPP__))
		#ifdef __GLIBCXX__
		static const std::string environment = "gcc libstdc++ version " QUOTEME(__GLIBCXX__);
		#else
		static const std::string environment = "gcc libstdc++ version " QUOTEME(__GLIBCPP__);
		#endif
	#else
		#error "undefined lib{std}c++ version"
	#endif
#elif (defined(_MSC_VER))
	// _CPPLIB_VER no longer officially exists, _CLR_VER is not what we want
	static const std::string environment = "msvc++ version " QUOTEME(_MSC_VER);
#else
	static const std::string environment = "unknown";
#endif

	return environment;
}

bool IsRelease()
{
	static const bool release = SPRING_VERSION_ENGINE_RELEASE;
	return release;
}

bool IsHeadless()
{
#ifdef HEADLESS
	return true;
#else
	return false;
#endif
}

bool IsUnitsync()
{
#ifdef UNITSYNC
	return true;
#else
	return false;
#endif
}

const std::string& Get()
{
	static const std::string base = IsRelease()
			? GetMajor()
			: (GetMajor() + "." + GetPatchSet() + ".1");

	return base;
}

const std::string& GetSync()
{
	static const std::string sync = IsRelease()
			? GetMajor()
			: SPRING_VERSION_ENGINE;

	return sync;
}

const std::string& GetFull()
{
	static const std::string full = SPRING_VERSION_ENGINE
			+ (GetAdditional().empty() ? "" : (" (" + GetAdditional() + ")"));

	return full;
}

}
