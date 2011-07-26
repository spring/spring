/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _JVM_LOCATER_COMMON_H
#define _JVM_LOCATER_COMMON_H

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

#endif // _JVM_LOCATER_COMMON_H
