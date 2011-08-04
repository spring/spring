/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// IMPORTANT NOTE: external systems sed -i this file, so DO NOT CHANGE without
// major thought in advance, and deliberation with bibim and tvo/Tobi!

#include "GameVersion.h"

#include <cstring>
#include <boost/version.hpp>
#include <boost/config.hpp>

/**
 * @brief Defines the current version string.
 * Take special care when moving this file.
 * The build-bot refers to this file to append the exact SCM version.
 */
namespace SpringVersion
{

const std::string& GetMajor()
{
	static const std::string major = "0.82+";
	return major;
}

const std::string& GetMinor()
{
	static const std::string minor = "4";
	return minor;
}

const std::string& GetPatchSet()
{
	static const std::string patchSet = "0";
	return patchSet;
}

const std::string& GetAdditional()
{
	static const std::string additional = "" // Build-Bot will write in here before compiling

#if !defined GV_ADD_SPACE
	// Build-Bot should set this to " " if it put something into the above line
	#define GV_ADD_SPACE ""
#endif

#if defined DEBUG
	GV_ADD_SPACE "Debug"
	#undef  GV_ADD_SPACE
	#define GV_ADD_SPACE " "
#endif

#if defined USE_MMGR
	GV_ADD_SPACE "mmgr"
	#undef  GV_ADD_SPACE
	#define GV_ADD_SPACE " "
#endif

#if defined USE_GML
	GV_ADD_SPACE "MT"
	#undef  GV_ADD_SPACE
	#define GV_ADD_SPACE " "
#endif
#if defined USE_GML_SIM
	"-Sim"
#endif
#if defined USE_GML_DEBUG
	"+Debug"
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

#if defined HEADLESS
	GV_ADD_SPACE "Headless"
	#undef  GV_ADD_SPACE
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

const std::string& GetBuildTime()
{
	static const std::string buildTime = __DATE__ " " __TIME__;
	return buildTime;
}

#define QUOTEME_(x) #x
#define QUOTEME(x) QUOTEME_(x)

const std::string& GetCompiler()
{
	static const std::string compiler = ""
#ifdef __GNUC__
	//"gcc-" QUOTEME(__GNUC__) "." QUOTEME(__GNUC_MINOR__) "." QUOTEME(__GNUC_PATCHLEVEL__);
	"gcc-" __VERSION__;
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

const std::string& GetBuildEnvironment(){
	static const std::string environment = "boost-"
#ifdef BOOST_VERSION
	QUOTEME(BOOST_VERSION)
#else
	"unknown"
#endif
	", "
#ifdef BOOST_STDLIB
	BOOST_STDLIB;
#else
	"unknown stdlib";
#endif
	return environment;
}

const std::string& Get()
{
	static const std::string version = GetMajor() + "." + GetMinor();
	return version;
}

const std::string& GetFull()
{
	static const std::string full = Get() + "." + GetPatchSet()
			+ (GetAdditional().empty() ? "" : (" (" + GetAdditional() + ")"));

	return full;
}

}
