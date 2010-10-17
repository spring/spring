/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SAIINTERFACELIBRARY_H
#define _SAIINTERFACELIBRARY_H

/*
 * All this is not needed when building an AI,
 * as it is strictly AI Interface related,
 * and therefore only matters to the engine and AI Interfaces.
 */
#if !defined BUILDING_SKIRMISH_AI

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * [string]
 * Absolute data dir containing the AIs AIInfo.lua file.
 * This property is set by the engine, not read from any file.
 * example: "/home/john/spring/AI/Interfaces/C/0.1"
 */
#define AI_INTERFACE_PROPERTY_DATA_DIR               "dataDir"

/**
 * [string]
 * Absolute, version independent data dir.
 * This property is set by the engine, not read from any file.
 * example: "/home/john/spring/AI/Interfaces/C/common"
 */
#define AI_INTERFACE_PROPERTY_DATA_DIR_COMMON        "dataDirCommon"

/**
 * [string: [a-zA-Z0-9_.]*]
 * example: "C"
 */
#define AI_INTERFACE_PROPERTY_SHORT_NAME             "shortName"

/**
 * [string: [a-zA-Z0-9_.]*]
 * example: "0.1"
 */
#define AI_INTERFACE_PROPERTY_VERSION                "version"

/**
 * [string]
 * example: "C/C++"
 */
#define AI_INTERFACE_PROPERTY_NAME                   "name"

/**
 * [string]
 * example: "supports loading native AIs written in C and/or C++"
 */
#define AI_INTERFACE_PROPERTY_DESCRIPTION            "description"

/**
 * [string]
 * example: "http://springrts.com/wiki/AIInterface:C"
 */
#define AI_INTERFACE_PROPERTY_URL                    "url"

/**
 * [string]
 * example: "C, C++"
 */
#define AI_INTERFACE_PROPERTY_SUPPORTED_LANGUAGES    "supportedLanguages"

/**
 * [int]
 * The engine version number the AI Interface was compiled for,
 * though it may work with newer or older engine versions too.
 */
#define AI_INTERFACE_PROPERTY_ENGINE_VERSION         "engineVersion"

/*
 * Everything following is (code wise) only interesting for the engine,
 * not for AI Interfaces.
 */
#if !defined BUILDING_AI

#include "System/exportdefines.h"
//#include "ELevelOfSupport.h"

//enum ELevelOfSupport;
struct SSkirmishAILibrary;
struct SStaticGlobalData;

/**
 * @brief Artificial Intelligence Interface library interface
 *
 * This is the interface between the engine and an implementation of
 * an AI Interface.
 * The engine will address AI Interfaces through this interface,
 * but AI Interfaces will not actually implement it. It is the job
 * of the engine, to make sure it can address AI Interface
 * implementations through instances of this struct.
 *
 * An example of loading the C AI Interface:
 * The C AI Interface library exports functions fitting the
 * function pointers in this struct. When the engine needs therefore
 * C AI Interface, it loads the shared library, eg C-AI-Interface.dll,
 * creates a new instance of this struct, and sets the member function
 * pointers to the addresses of the fitting functions exported
 * by the shared library. The functions of the AI Interface are then
 * allways called through this struct.
 */
struct SAIInterfaceLibrary {

	// static AI interface library functions

	/**
	 * This function is called right after the library is dynamically loaded.
	 * It can be used to initialize variables and to check or prepare
	 * the environment (os, engine, filesystem, ...).
	 * @see releaseStatic()
	 *
	 * [optional]
	 * An AI Interface not exporting this function is still valid.
	 *
	 * @param	staticGlobalData contains global data about the engine and the
	 *                           environment; is guaranteed to be valid till
	 *                           releaseStatic() is called.
	 * @return     0: ok
	 *          != 0: error
	 */
	int (CALLING_CONV *initStatic)(int interfaceId,
			const struct SAIInterfaceCallback* const);

//			unsigned int infoSize,
//			const char** infoKeys, const char** infoValues,
//			const struct SStaticGlobalData* staticGlobalData);

	/**
	 * This function is called right right before the library is unloaded.
	 * It can be used to deinitialize variables and to cleanup the environment,
	 * for example the filesystem.
	 *
	 * See also initStatic().
	 *
	 * [optional]
	 * An AI Interface not exporting this function is still valid.
	 *
	 * @return     0: ok
	 *          != 0: error
	 */
	int (CALLING_CONV *releaseStatic)();

//	/**
//	 * Level of Support for a specific engine version.
//	 *
//	 * [optional]
//	 * An AI Interface not exporting this function is still valid.
//	 */
//	enum LevelOfSupport (CALLING_CONV *getLevelOfSupportFor)(
//			const char* engineVersionString, int engineVersionNumber);


	// skirmish AI methods

	/**
	 * Loads the specified Skirmish AI.
	 *
	 * @return     0: ok
	 *          != 0: error
	 */
	const struct SSkirmishAILibrary* (CALLING_CONV *loadSkirmishAILibrary)(
			const char* const shortName,
			const char* const version);

	/**
	 * Unloads the specified Skirmish AI.
	 *
	 * @return     0: ok
	 *          != 0: error
	 */
	int (CALLING_CONV *unloadSkirmishAILibrary)(
			const char* const shortName,
			const char* const version);

	/**
	 * Unloads all Skirmish AI libraries currently loaded by this interface.
	 *
	 * @return     0: ok
	 *          != 0: error
	 */
	int (CALLING_CONV *unloadAllSkirmishAILibraries)();

};

#endif // !defined BUILDING_AI

#ifdef	__cplusplus
} // extern "C"
#endif

#endif // !defined BUILDING_SKIRMISH_AI
#endif // _SAIINTERFACELIBRARY_H
