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

#ifndef _CUTILS_UTIL_H
#define _CUTILS_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

// BEGINN: String realated functions

/**
 * Allocates fresh memory for storing a C string of the specified length.
 */
char* util_allocStr(unsigned int length);

/**
 * Allocates fresh memory and copies the supplied string into it.
 * @return  the freshly allocated memory containing the string,
 *          or NULL, if the arguemtn was NULL.
 */
char* util_allocStrCpy(const char* toCopy);

/**
 * Allocates fresh memory and copies a part of the supplied string into it.
 */
char* util_allocStrSubCpy(const char* toCopy, int fromPos, int toPos);
/**
 * Allocates fresh memory and copies a part of the supplied string into it.
 */
char* util_allocStrSubCpyByPointers(const char* toCopy,
		const char* fromPos, const char* toPos);

/**
 * Concatenates a variable number of C strings into newly allocated memory.
 * The number of strings to concatenate has to be supplied as first argument.
 */
char* util_allocStrCat(int numParts, const char* first, ...);
/**
 * Concatenates a variable number of C strings into newly allocated memory.
 * The last argument has to be NULL, as there would be no way to determine
 * the number of arguments otherwise.
 */
char* util_allocStrCat_nt(const char* first, ...);

/**
 * Concatenates a variable number of C strings,
 * which represent file system parts (eg. dirs or files)
 * into newly allocated memory.
 * The number of parts to concatenate has to be supplied as first argument.
 * sample call:
 * (3, "/home/user/tmp", "appName/", "/i/am/fine")
 * output:
 * "/home/user/tmp/appName/i/am/fine"
 */
char* util_allocStrCatFSPath(int numParts, const char* first, ...);

bool util_isWhiteSpace(char c);

void util_strLeftTrim(char* toTrim);
void util_strRightTrim(char* toTrim);
void util_strTrim(char* toTrim);
char* util_allocStrTrimed(const char* toTrim);

void util_strReplaceChar(char* toChange, char toFind, char replacer);
char* util_allocStrReplaceStr(const char* toChange, const char* toFind,
		const char* replacer);

bool util_startsWith(const char* str, const char* prefix);
bool util_endsWith(const char* str, const char* suffix);

bool util_strToBool(const char* str);

// END: String realated functions


// BEGINN: File system realated functions

/**
 * Checks whether a string contians an absolute path.
 * The corresponding matcher-regexes are:
 * on Windows: "^.:(\\|/).*"
 * otherwise:  "^/.*"
 *
 * @param  path  the dir or file path to be checked for absoluteness
 * @return  true if the path is absolute; it does not have to exist
 */
bool util_isPathAbsolute(const char* path);

/**
 * Lists all files found below a directory.
 *
 * Example:
 * char* foundFiles[64];
 * (".../AI/Interface/Java/0.1/jlib/", ".jar", foundFiles, true, 64)
 *    ".../AI/Interface/Java/0.1/jlib/vecmath.jar"
 *    ".../AI/Interface/Java/0.1/jlib/jna/jna.jar"
 *    ".../AI/Interface/Java/0.1/jlib/jna/linux-i386.jar"
 *
 * @param dir          root directory to search for files
 * @param suffix       files to list have to end with this suffix, eg. ".jar"
 * @param fileNames    resulting array with filenames
 * @param recursive    list files recursively
 * @param maxFileNames max files to return
 */
unsigned int util_listFiles(const char* dir, const char* suffix,
		char** fileNames, bool recursive, const unsigned int maxFileNames);

/**
 * Removes a slash ('/' or '\') from the end of a string, if there is one.
 */
void util_removeTrailingSlash(char* fsPath);

/**
 * Returns true if the file specified in filePath exists.
 * @param filePath path to the file
 */
bool util_fileExists(const char* filePath);

/**
 * Creates the dir specified in dirPath, if its parent directory exists
 * and we have write access to it.
 *
 * @param  recursive  whether ancestor dirs shall be created aswell,
 *                    if they do not yet exist
 * @return true if the dir could be created or existed already.
 */
bool util_makeDir(const char* dirPath, bool recursive);

/**
 * Saves the parent directory of a file or dir specified in path
 * into parentPath.
 *
 * Example:
 * before: "/home/user/games/spring/AI/Skirmish/RAI/0.1/log.txt"
 * after:  "/home/user/games/spring/AI/Skirmish/RAI/0.1"
 */
bool util_getParentDir(char* path);

/**
 * Finds a file or directory under dirs with the relativeFilePath
 * and saves the resulting path in absoluteFilePath.
 * If searchOnlyWriteable is set, only the first entry in dirs
 * is used for the search, as it is assumed to contain
 * the writable directory.
 *
 * @return  true if the file exists
 */
bool util_findFile(const char* dirs[], unsigned int numDirs,
		const char* relativeFilePath, char* absoluteFilePath,
		bool searchOnlyWriteable);

/**
 * Finds a directory under dirs with the relativeDirPath
 * and saves the resulting path in absoluteDirPath.
 * If searchOnlyWriteable is set, only the first entry in dirs
 * is used for the search, as it is assumed to contain
 * the writable directory.
 *
 * @return  true if the file existed or was created
 */
bool util_findDir(const char* dirs[], unsigned int numDirs,
		const char* relativeDirPath, char* absoluteDirPath,
		bool searchOnlyWriteable, bool create);

/**
 * Parses a properties file into a C [string, string] map.
 * Lines that contain only white-spaces and lines beginning
 * with '#' or ';' are ignored.
 * Regex for a valid property:
 * ^[\\w]*[^;#][^=]*=.+$
 * The strings in the \<keys\> and \<values\> arrays have to
 * be freed by the callee.
 */
int util_parsePropertiesFile(const char* propertiesFile,
		const char* keys[], const char* values[], int maxProperties);

// END: File system realated functions


/**
 * Find the value assigned to a key in a C [string, string] map.
 * @return  the assigned value or NULL if the key was not found
 */
const char* util_map_getValueByKey(
		unsigned int mapSize,
		const char** mapKeys, const char** mapValues,
		const char* key);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // _CUTILS_UTIL_H
