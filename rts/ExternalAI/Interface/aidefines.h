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

#ifndef _AIDEFINES_H
#define	_AIDEFINES_H

#ifdef	__cplusplus
extern "C" {
#endif	/* __cplusplus */

#include "System/exportdefines.h"

#include "SAIFloat3.h"

#include "Game/GameVersion.h"

#define ENGINE_VERSION_STRING VERSION_STRING
#define ENGINE_VERSION_NUMBER 1000

// TODO: comment toggle the two following lines
// !!!absolute paths are needed when debugging!!!
//#define PATH_TO_SPRING_HOME "/home/robin/svn_work/robin/Development/Projects/Others/spring_C_AI_interface/game_linux/"
//#define PATH_TO_SPRING_HOME "P:/svn_work/robin/Development/Projects/Others/spring_current/game/"
//#define PATH_TO_SPRING_HOME ""

/**
 * @brief max skirmish AIs
 *
 * Defines the maximum number of skirmish AIs.
 * As there can not be more then spring allows teams, this is the upper limit.
 * (currently (July 2008) 16 real teams)
 */
//const unsigned int MAX_SKIRMISH_AIS = MAX_TEAMS - 1;
#define MAX_SKIRMISH_AIS 16

//const char* const AI_INTERFACES_DIR = "AI/interfaces";
//#define AI_INTERFACES_DIR "AI/interfaces"
#define AI_INTERFACES_IMPLS_DIR "AI/Interfaces/impls"
#define AI_INTERFACES_DATA_DIR "AI/Interfaces/data"
//#define AI_INTERFACES_LOGS_DIR "AI/Interfaces/logs"

#define SKIRMISH_AI_IMPLS_DIR "AI/Skirmish/impls"
#define SKIRMISH_AI_DATA_DIR "AI/Skirmish/data"

#define GROUP_AI_IMPLS_DIR "AI/Group/impls"
#define GROUP_AI_DATA_DIR "AI/Group/data"

#ifdef	__cplusplus
}		/* extern "C" */
#endif	/* __cplusplus */

#endif	/* _AIDEFINES_H */
