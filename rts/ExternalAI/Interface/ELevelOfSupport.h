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

#ifndef _ELEVELOFSUPPORT_H
#define	_ELEVELOFSUPPORT_H

#ifdef	__cplusplus
extern "C" {
#endif

enum LevelOfSupport {
	LOS_None,		// 0: will (possibly) result in a crash
	LOS_Bad,		// 1: does not work correctly, eg.: does nothing, just stand around, but neither does crash
	LOS_Working,	// 2: does work, but may not use all info the engine and ai interface supply
	LOS_Compleet,	// 3: does work and use the engine and ai interface to the fullest,
					//    but is optimised for newer versions
	LOS_Optimal,	// 4: is made optimally suitet for this engine and ai interface version
	LOS_Unknown		// 5: not evaluated (used eg when a library is not accessed directly,
					//    but info is loaded from a file)
};

#ifdef	__cplusplus
}	// extern "C"
#endif

#endif	// _ELEVELOFSUPPORT_H

