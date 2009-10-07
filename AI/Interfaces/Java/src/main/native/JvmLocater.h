/*
	Copyright (c) 2009 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _JVMLOCATER_H
#define _JVMLOCATER_H

#if !defined bool
#include <stdbool.h>
#endif
#if !defined size_t
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the arch dir name, used eg here:
 * .../jre/lib/${arch}/client/libjvm.so
 *
 * @return sparc, sparcv9, i386, amd64 or ia64 (windows only)
 */
const char* GetArchPath();

/*
 * Given a JRE location and a JVM type, construct what the name the
 * JVM shared library will be.
 *
 * @param jvmType "/", "\\", "client", "server"
 * @param arch see GetArchPath(), use NULL for the default value
 * @return true, if the JVM library was found, false otherwise.
 */
bool GetJVMPath(const char* jrePath, const char* jvmType,
		char* jvmPath, size_t jvmPathSize, const char* arch);

/**
 * Find the path to a JRE install dir, using platform dependent means.
 *
 * @param configFile path to a simple text file containing only a path to the
 *                   JRE installation to use, or NULL
 * @param arch see GetArchPath(), use NULL for the default value
 * @return true, if a JRE was found, false otherwise.
 */
bool GetJREPath(char* path, size_t pathSize, const char* configFile,
		const char* arch);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _JVMLOCATER_H
