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

#ifndef _SSTATICGLOBALDATA_H
#define _SSTATICGLOBALDATA_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * Directories stored in this struct are guaranteed
 * to come with no trailing '/' or '\\':
 * like this:      /home/username/.spring
 * NOT like this:  /home/username/.spring/
 */
struct SStaticGlobalData {
	unsigned int maxTeams;
	const char* springVersion;
	unsigned int numDataDirs;
	/**
	 * Contains al the data directory to use.
	 * A data directory contains spring source-files,
	 * and it may have a structure like this:
	 * maps/
	 * mods/
	 * AI/Interfaces/
	 * AI/Skirmish/
	 *
	 * The first entry is the writeable data-dir,
	 * while all the others are read only.
	 *
	 * Example content:
	 * dataDirs[0] (rw): /home/user/.spring/
	 * dataDirs[1] (r):  /usr/share/games/spring/
	 * dataDirs[2] (r):  ./
	 *
	 * @see numDataDirs
	*/
	const char** dataDirs;
	/**
	 * The following four members define the Skirmish AI libraries
	 * that will be used in the currently running game.
	 * This is used by AI Interface libs to prepare the environment.
	 * The Java AI Interface needs this info to prepare the classpath eg.
	 */
	unsigned int numSkirmishAIs;
	unsigned int* skirmishAIInfosSizes;
	const char*** skirmishAIInfosKeys;
	const char*** skirmishAIInfosValues;
};

// define the OS specific path separator
#if defined WIN32
#define PS '\\'
#define sPS "\\"
#else // defined WIN32
#define PS '/'
#define sPS "/"
#endif // defined WIN32

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#if defined __cplusplus && !defined BUILDING_AI
struct SStaticGlobalData* createStaticGlobalData();
void freeStaticGlobalData(struct SStaticGlobalData* staticGlobalData);
#endif // defined __cplusplus && !defined BUILDING_AI

#endif // _SSTATICGLOBALDATA_H
