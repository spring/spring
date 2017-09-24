/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#if !defined _WIN32

#include "JvmLocater_common.h"

#include "System/MainDefines.h"
#include "System/SafeCStrings.h"

#include <unistd.h>


#if defined APPLE
#define JVM_LIB "libjvm.dylib"
#else
#define JVM_LIB "libjvm.so"
#endif

#if defined APPLE
#define JAVA_LIB "libjava.dylib"
#else
#define JAVA_LIB "libjava.so"
#endif

#ifdef SPARC
#	define LIBARCH32NAME "sparc"
#	define LIBARCH64NAME "sparcv9"
#else // not SPARC -> LINUX or APPLE
#	define LIBARCH32NAME "i386"
#	define LIBARCH64NAME "amd64"
#endif // SPARC
#if defined __arch64__
#	define LIBARCHNAME LIBARCH64NAME
#else // defined __arch64__
#	define LIBARCHNAME LIBARCH32NAME
#endif // defined __arch64__
//LIBARCH32   solaris only: sparc or i386
//LIBARCH64   solaris only: sparcv9 or amd64
//LIBARCH	 sparc, sparcv9, i386, amd64, or ia64 // last one is windows only


const char* GetArchPath()
{
	switch(CURRENT_DATA_MODEL) {
#ifdef DUAL_MODE
		case 32:
			return LIBARCH32NAME;
		case 64:
			return LIBARCH64NAME;
#endif // DUAL_MODE
		default:
			return LIBARCHNAME;
	}
}

/*
 * On Solaris VM choosing is done by the launcher (java.c).
 */
bool GetJVMPath(const char* jrePath, const char* jvmType,
		char* jvmPath, size_t jvmPathSize, const char* arch)
{
	if (arch == NULL) {
		arch = GetArchPath();
	}

	if (*jvmType == '/') {
		SNPRINTF(jvmPath, jvmPathSize, "%s/"JVM_LIB, jvmType);
	} else {
		SNPRINTF(jvmPath, jvmPathSize, "%s/lib/%s/%s/"JVM_LIB, jrePath, arch,
				jvmType);
	}

	return FileExists(jvmPath);
}


static bool CheckIfJREPath(const char* path, const char* arch)
{
	bool found = false;

	if (path != NULL) {
		char libJava[MAXPATHLEN];

		// Is path a JRE path?
		SNPRINTF(libJava, MAXPATHLEN, "%s/lib/%s/"JAVA_LIB, path, arch);
		if (access(libJava, F_OK) == 0) {
			found = true;
		}
	}

	return found;
}

bool GetJREPathFromBase(char* path, size_t pathSize, const char* basePath,
		const char* arch)
{
	bool found = false;

	if (basePath != NULL) {
		char jrePath[MAXPATHLEN];

		// Is basePath a JRE path?
		STRCPY_T(jrePath, MAXPATHLEN, basePath);
		if (CheckIfJREPath(jrePath, arch)) {
			STRCPY_T(path, pathSize, basePath);
			found = true;
		}

		// Is basePath/jre a JRE path?
		STRCAT_T(jrePath, MAXPATHLEN, "/jre");
		if (CheckIfJREPath(jrePath, arch)) {
			STRCPY_T(path, pathSize, jrePath);
			found = true;
		}
	}

	return found;
}

static size_t ExecFileSystemGlob(char** pathHits, size_t pathHits_sizeMax,
		const char* globPattern)
{
	size_t pathHits_size = 0;

	// assemble the command
	static const size_t cmd_sizeMax = 512;
	char cmd[cmd_sizeMax];
	SNPRINTF(cmd, cmd_sizeMax, "find %s/ -maxdepth 0 2> /dev/null", globPattern);

	// execute
	FILE* cmd_fp = popen(cmd, "r");
	if (cmd_fp == NULL) {
		return pathHits_size;
	}

	// parse results
	static const size_t line_sizeMax = 512;
	char line[line_sizeMax];
	while (fgets(line, line_sizeMax, cmd_fp) && (pathHits_size < pathHits_sizeMax)) {
		size_t line_size = strlen(line);
		if (*(line+line_size-1) == '\n') {
			// remove trailing '\n'
			*(line+line_size-1) = '\0';
			line_size--;
		}

		simpleLog_logL(LOG_LEVEL_DEBUG,
				"glob-hit \"%s\"!", line);

		if (line_size > 0 && *line == '/') {
			*(line+line_size-1) = '\0'; // remove trailing '/'
			pathHits[pathHits_size++] = util_allocStrCpy(line);
		}
	}
	pclose(cmd_fp);

	return pathHits_size;
}
static bool GetJREPathInCommonLocations(char* path, size_t pathSize, const char* arch)
{
	bool found = false;

	static const size_t possLoc_sizeMax = 32;
	char* possLoc[possLoc_sizeMax];
	size_t possLoc_i = 0;

	possLoc[possLoc_i++] = util_allocStrCpy("/usr/local/jdk*");
	possLoc[possLoc_i++] = util_allocStrCpy("/usr/lib/jvm/default-java");
	possLoc[possLoc_i++] = util_allocStrCpy("/usr/lib/jvm/java-?-sun");
	possLoc[possLoc_i++] = util_allocStrCpy("/usr/lib/jvm/java-?-*");
	possLoc[possLoc_i++] = util_allocStrCpy("~/jdk*");
	possLoc[possLoc_i++] = util_allocStrCpy("~/bin/jdk*");
	possLoc[possLoc_i++] = util_allocStrCpy("~/jre*");
	possLoc[possLoc_i++] = util_allocStrCpy("~/bin/jre*");

	static const size_t globHits_sizeMax = 32;
	char* globHits[globHits_sizeMax];
	size_t l, g;
	for (l=0; l < possLoc_i; ++l) {
		const size_t globHits_size = ExecFileSystemGlob(globHits, globHits_sizeMax, possLoc[l]);
		for (g=0; g < globHits_size; ++g) {
			found = GetJREPathFromBase(path, pathSize, globHits[g], arch);
			if (found) {
				simpleLog_logL(LOG_LEVEL_NOTICE,
						"JRE found common location env var \"%s\"!",
						possLoc[l]);
				goto locSearchEnd;
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

static bool GetJREPathWhichJava(char* path, size_t pathSize, const char* arch)
{
	static const char* suf = "/bin/java";
	const size_t suf_size = strlen(suf);

	bool found = false;

	// execute
	FILE* cmd_fp = popen("which java  | sed 's/[\\n\\r]/K/g'", "r");
	if (cmd_fp == NULL) {
		return found;
	}

	// parse results
	static const size_t line_sizeMax = 512;
	char line[line_sizeMax];
	if (fgets(line, line_sizeMax, cmd_fp)) {
		if (*line == '/') { // -> absolute path
			size_t line_size = strlen(line);
			if (*(line+line_size-1) == '\n') {
				// remove trailing '\n'
				*(line+line_size-1) = '\0';
				line_size--;
			}

			simpleLog_logL(LOG_LEVEL_DEBUG,
					"which line \"%s\"!", line);

			if (line_size > suf_size
					&& strcmp(line+(line_size-suf_size), suf) == 0) {
				// line ends with suf
				simpleLog_logL(LOG_LEVEL_NOTICE,
						"JRE found with `which java`!");
				// remove suf
				*(line+(line_size-suf_size)) = '\0';
				found = GetJREPathFromBase(path, pathSize, line, arch);
			}
		}
	}
	pclose(cmd_fp);

	return found;
}

/*
 * Find path to JRE based on .exe's location or registry settings.
 */
bool GetJREPathOSSpecific(char* path, size_t pathSize, const char* arch)
{
	bool found = false;

	// check if a JRE is located in a common location
	if (!found) {
		found = GetJREPathInCommonLocations(path, pathSize, arch);
	}

	// check if a JRE is located in a common location
	if (!found) {
		found = GetJREPathWhichJava(path, pathSize, arch);
	}

	return found;
}

#endif // !defined _WIN32
