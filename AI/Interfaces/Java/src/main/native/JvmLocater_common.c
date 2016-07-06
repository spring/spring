/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

#include "CUtils/Util.h"

#include "JvmLocater.h"

#define MAXPATHLEN 2048
#define JRE_PATH_PROPERTY "jre.path"

#include <limits.h> // for CHAR_BIT
#define CURRENT_DATA_MODEL (CHAR_BIT * sizeof(void*))

#include "CUtils/SimpleLog.h"
#include "System/maindefines.h"
#include "System/SafeCStrings.h"

// implemented in the OS specific files
const char* GetArchPath();
bool GetJREPathFromBase(char* path, size_t pathSize, const char* basePath,
		const char* arch);
bool GetJREPathOSSpecific(char* path, size_t pathSize, const char* arch);

bool FileExists(const char* filePath)
{
	struct stat s;
	return (stat(filePath, &s) == 0);
}

//#define LOC_PROP_FILE
bool GetJREPathFromConfig(char* path, size_t pathSize, const char* configFile)
{
#if defined LOC_PROP_FILE
	// assume the config file is in properties file format
	// and that the property JRE_PATH_PROPERTY contains
	// the absolute path to the JRE to use
	static const size_t props_sizeMax = 64;

	const char* props_keys[props_sizeMax];
	const char* props_values[props_sizeMax];

	const size_t props_size = util_parsePropertiesFile(configFile, props_keys,
			props_values, props_sizeMax);

	const char* jvmLocation = util_map_getValueByKey(
			props_size, props_keys, props_values,
			JRE_PATH_PROPERTY);

	if (jvmLocation == NULL) {
		simpleLog_logL(LOG_LEVEL_DEBUG, "JRE not found in config file!");
		return false;
	} else {
		simpleLog_logL(LOG_LEVEL_NOTICE, "JRE found in config file!");
		STRCPY_T(path, pathSize, jvmLocation);
		return true;
	}
#else // defined LOC_PROP_FILE
	// assume the config file is a plain text file containing nothing but
	// the absolute path to the JRE to use in the first line

	bool found = false;

	FILE* cfp = fopen(configFile, "r");
	if (cfp == NULL) {
		return found;
	}

	// parse results
	static const size_t line_sizeMax = 1024;
	char line[line_sizeMax];
	if (fgets(line, line_sizeMax, cfp)) {
		size_t line_size = strlen(line);
		if (*(line+line_size-1) == '\n') {
			// remove trailing '\n'
			*(line+line_size-1) = '\0';
			line_size--;
		}

		simpleLog_logL(LOG_LEVEL_NOTICE,
				"Fetched JRE location from \"%s\"!", configFile);

		if (line_size > 0 && *line == '/') {
			*(line+line_size-1) = '\0'; // remove trailing '/'
		}
		STRCPY_T(path, pathSize, line);
		found = true;
	}
	fclose(cfp);

	return found;
#endif // defined LOC_PROP_FILE
}


bool GetJREPathFromEnvVars(char* path, size_t pathSize, const char* arch)
{
	bool found = false;

	static const size_t possLoc_sizeMax = 32;
	char* possLoc[possLoc_sizeMax];
	size_t possLoc_i = 0;

	possLoc[possLoc_i++] = util_allocStrCpy("JAVA_HOME");
	possLoc[possLoc_i++] = util_allocStrCpy("JDK_HOME");
	possLoc[possLoc_i++] = util_allocStrCpy("JRE_HOME");

	size_t l;
	for (l=0; l < possLoc_i; ++l) {
		const char* envPath = getenv(possLoc[l]);
		if (envPath != NULL) {
			found = GetJREPathFromBase(path, pathSize, envPath, arch);
			if (found) {
				simpleLog_logL(LOG_LEVEL_NOTICE, "JRE found in env var \"%s\"!", possLoc[l]);
				goto locSearchEnd;
			} else {
				simpleLog_logL(LOG_LEVEL_WARNING, "Unusable JRE from env var \"%s\"=\"%s\"!", possLoc[l], envPath);
			}
		}
	}
 locSearchEnd:

	// cleanup
	for (l=0; l < possLoc_i; ++l) {
		free(possLoc[l]);
		possLoc[l] = NULL;
	}

	return found;
}



bool GetJREPath(char* path, size_t pathSize, const char* configFile,
		const char* arch)
{
	bool found = false;

	if (arch == NULL) {
		arch = GetArchPath();
	}

	// check if a JRE location is specified in the config file
	if (!found && configFile != NULL) {
		found = GetJREPathFromConfig(path, pathSize, configFile);
	}

	// check if a JRE is specified in an ENV var (eg. JAVA_HOME)
	if (!found) {
		found = GetJREPathFromEnvVars(path, pathSize, arch);
	}

	// check if a JRE is located in a common location
	if (!found) {
		found = GetJREPathOSSpecific(path, pathSize, arch);
	}

	return found;
}

int main(int argc, const char* argv[]) {

	//simpleLog_init(NULL, false, LOG_LEVEL_DEBUG, false);

	static const size_t path_sizeMax = 1024;
	char path[path_sizeMax];
	bool found = GetJREPath(path, path_sizeMax, NULL, NULL);
	if (found) {
		printf("JRE found: %s\n", path);

		static const size_t jvmPath_sizeMax = 1024;
		char jvmPath[jvmPath_sizeMax];
		bool jvmFound = GetJVMPath(path, "client", jvmPath, jvmPath_sizeMax, NULL);
		if (jvmFound) {
			printf("JVM found: %s\n", jvmPath);
		} else {
			printf("JVM not found.\n");
		}
	} else {
		printf("JRE not found.\n");
	}

	return 0;
}
