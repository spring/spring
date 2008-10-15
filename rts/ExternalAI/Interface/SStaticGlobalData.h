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
#define	_SSTATICGLOBALDATA_H

#ifdef	__cplusplus
extern "C" {
#endif	/* __cplusplus */

// paths/dirs stored in this struct are guaranteed
// to come with no '/' or '\\' at the end.
// they come like this:	/home/username/.spring
// NOT like this:		/home/username/.spring/
struct SStaticGlobalData {
	unsigned int maxTeams;
	const char* springVersion;
	/** The first entry is the writeable data-dir */
	unsigned int numDataDirs;
	const char** dataDirs;
};

// define the OS specific path separator
#ifdef WIN32
#define PS '\\'
#else	/* WIN32 */
#define PS '/'
#endif	/* WIN32 */

#ifdef	__cplusplus
}		/* extern "C" */
#endif	/* __cplusplus */

#if defined	__cplusplus && !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE
struct SStaticGlobalData* createStaticGlobalData();
void freeStaticGlobalData(struct SStaticGlobalData* staticGlobalData);
#endif	/* defined	__cplusplus && !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE */

#endif	/* _SSTATICGLOBALDATA_H */

