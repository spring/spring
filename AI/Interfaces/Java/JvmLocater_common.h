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

#ifndef _JVMLOCATERCOMMON_H
#define _JVMLOCATERCOMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "JvmLocater.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <limits.h> // for CHAR_BIT

#include "CUtils/Util.h"
#include "CUtils/SimpleLog.h"

#define MAXPATHLEN 2048
#define JRE_PATH_PROPERTY "jre.path"
#define CURRENT_DATA_MODEL (CHAR_BIT * sizeof(void*))


bool FileExists(const char* filePath);

bool GetJREPathFromEnvVars(char* path, size_t pathSize, const char* arch);

bool GetJREPath(char* path, size_t pathSize, const char* configFile,
		const char* arch);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _JVMLOCATERCOMMON_H
