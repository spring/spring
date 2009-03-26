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

#ifndef _INTERFACEDEFINES_H
#define _INTERFACEDEFINES_H

#ifdef __cplusplus
extern "C" {
#endif

#define JAVA_SKIRMISH_AI_PROPERTY_CLASS_NAME "className"

#define MY_LOG_FILE "interface-log.txt"
#define JAVA_AI_INTERFACE_LIBRARY_FILE_NAME "AIInterface.jar"
#define JAVA_LIBS_DIR "jlib"
#define NATIVE_LIBS_DIR "lib"
#define JRE_LOCATION_FILE "jre-location.txt"

#include <stddef.h> // for NULL

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _INTERFACEDEFINES_H
