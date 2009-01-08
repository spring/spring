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

#ifndef _SAIINTERFACELIBRARY_H
#define	_SAIINTERFACELIBRARY_H

/*
 * All this is not needed when building an AI,
 * as it is strictly AI Interface related,
 * and therefore only matters to the engine and AI Interfaces.
 */
#if !defined BUILDING_AI

#include "creg/creg_cond.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define AI_INTERFACE_PROPERTY_DATA_DIR               "dataDir"               // [string] absolute data dir containing the AIs AIInfo.lua file. this property is by the engine, not read from any file.
#define AI_INTERFACE_PROPERTY_SHORT_NAME             "shortName"             // [string: [a-zA-Z0-9_.]*]
#define AI_INTERFACE_PROPERTY_VERSION                "version"               // [string: [a-zA-Z0-9_.]*]
#define AI_INTERFACE_PROPERTY_NAME                   "name"                  // [string]
#define AI_INTERFACE_PROPERTY_DESCRIPTION            "description"           // [string]
#define AI_INTERFACE_PROPERTY_URL                    "url"                   // [string]
#define AI_INTERFACE_PROPERTY_SUPPORTED_LANGUAGES    "supportedLanguages"    // [string]
#define AI_INTERFACE_PROPERTY_ENGINE_VERSION         "engineVersion"         // [int] the engine version number the AI was compiled, but may work with newer or older ones too

/*
 * Everythign following is only interesting for the engine,
 * not for AI Interfaces.
 */
#if !defined BUILDING_AI_INTERFACE

#include "ELevelOfSupport.h"
#include "SSAILibrary.h"
#include "System/exportdefines.h"

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
	 * See also releaseStatic().
	 *
	 * NOTE: param staticGlobalData is guaranteed to be valid till
	 * releaseStatic() is called.
	 *
	 * NOTE: this method is optional. An AI Interface not exporting this
	 * function is still valid.
	 *
	 * @param	staticGlobalData	contains global data about hte engine
	 *								and the environment
	 * @return	ok: 0, error: != 0
	 */
	int (CALLING_CONV *initStatic)(
			unsigned int infoSize,
			const char** infoKeys, const char** infoValues,
			const struct SStaticGlobalData* staticGlobalData);

	/**
	 * This function is called right right before the library is unloaded.
	 * It can be used to deinitialize variables and to cleanup the environment,
	 * for example the filesystem.
	 *
	 * See also initStatic().
	 *
	 * NOTE: this method is optional. An AI Interface not exporting this
	 * function is still valid.
	 *
	 * @return	ok: 0, error: != 0
	 */
	int (CALLING_CONV *releaseStatic)();

	/**
	 * Level of Support for a specific engine version.
	 *
	 * NOTE: this method is optional. An AI Interface not exporting this
	 * function is still valid.
	 */
	enum LevelOfSupport (CALLING_CONV *getLevelOfSupportFor)(
			const char* engineVersionString, int engineVersionNumber);


	// skirmish AI methods

	/**
	 * Loads the specified Skirmish AI.
	 *
	 * @return	ok: 0, error: != 0
	 */
	const struct SSAILibrary* (CALLING_CONV *loadSkirmishAILibrary)(
			unsigned int infoSize,
			const char** infoKeys, const char** infoValues);

	/**
	 * Unloads the specified Skirmish AI.
	 *
	 * @return	ok: 0, error: != 0
	 */
	int (CALLING_CONV *unloadSkirmishAILibrary)(
			unsigned int infoSize,
			const char** infoKeys, const char** infoValues);

	/**
	 * Unloads all Skirmish AI libraries currently loaded by this interface.
	 *
	 * @return	ok: 0, error: != 0
	 */
	int (CALLING_CONV *unloadAllSkirmishAILibraries)();
};

#endif // !defined BUILDING_AI_INTERFACE

#ifdef	__cplusplus
} // extern "C"
#endif

#endif // !defined BUILDING_AI
#endif // _SAIINTERFACELIBRARY_H
