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

#include "Util.h"

#include "ExternalAI/Interface/SStaticGlobalData.h" // for sPS
#include "ExternalAI/Interface/SAIInterfaceLibrary.h" // for AI_INTERFACE_PROPERTY_DATA_DIR

#include <string.h>	// strcpy(), str...()
#include <stdlib.h>	// malloc(), calloc(), free()
#include <sys/stat.h>	// used for check if a file exists
#ifdef	WIN32
#include <io.h>		// needed for dir listing
#include <direct.h>	// mkdir()
#else	// WIN32
#include <sys/stat.h>	// mkdir()
#include <sys/types.h>	// mkdir()
#include <dirent.h>		// needed for dir listing
#endif	// WIN32

static unsigned int myInfoSize;
static const char** myInfoKeys;
static const char** myInfoValues;

static const char* myDataDirVersioned = NULL;
static const char* myDataDirUnversioned = NULL;

void util_setMyInfo(
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues) {

	myInfoSize = infoSize;
	myInfoKeys = infoKeys;
	myInfoValues = infoValues;

	myDataDirVersioned = util_getMyInfo(AI_INTERFACE_PROPERTY_DATA_DIR);

	const char* lastSlash = strrchr(myDataDirVersioned, '/');
	if (lastSlash == NULL) {
		lastSlash = strrchr(myDataDirVersioned, '\\');
	}
	myDataDirUnversioned = util_allocStrSubCpyByPointers(
			myDataDirVersioned, myDataDirVersioned, lastSlash);
}
const char* util_getMyInfo(const char* key) {
	return util_map_getValueByKey(myInfoSize, myInfoKeys, myInfoValues, key);
}

/*
void util_setDataDirs(const char* unversioned, const char* versioned) {

	myDataDir = unversioned;
	myDataDirVers = versioned;
}
*/
const char* util_getDataDirVersioned() {
	return myDataDirVersioned;
}
const char* util_getDataDirUnversioned() {
	return myDataDirUnversioned;
}

char* util_allocStr(unsigned int length) {
	return (char*) calloc(length+1, sizeof(char));
}

char* util_allocStrCpy(const char* toCopy) {

	char* copy = (char*) calloc(strlen(toCopy)+1, sizeof(char));
	STRCPY(copy, toCopy);
	return copy;
}

char* util_allocStrSubCpy(const char* toCopy, int fromPos, int toPos) {

	if (toPos < 0) {
		toPos = strlen(toCopy);
	}

	if (fromPos < 0 || toPos < fromPos) {
		return NULL;
	}

	unsigned int newSize = toPos - fromPos;
	char* copy = (char*) calloc(newSize+1, sizeof(char));
	unsigned int i;
	for (i=0; i < newSize; ++i) {
		copy[i] = toCopy[fromPos + i];
	}

	return copy;
}

char* util_allocStrSubCpyByPointers(const char* toCopy,
		const char* fromPos, const char* toPos) {

	unsigned int newSize = toPos - fromPos;
	char* copy = (char*) calloc(newSize+1, sizeof(char));
	unsigned int i;
	for (i=0; i < newSize; ++i) {
		copy[i] = *(fromPos + i);
	}

	return copy;
}

char* util_allocStrCpyCat(const char* toPart1, const char* toPart2) {

	char* copy = (char*) calloc(strlen(toPart1)+strlen(toPart2)+1, sizeof(char));
	STRCPY(copy, toPart1);
	STRCAT(copy, toPart2);
	return copy;
}

void util_strReplace(char* toChange, char toFind, char replacer) {

	const unsigned int len = strlen(toChange);
	unsigned int i;
	for (i = 0; i < len; i++) {
		if (toChange[i] == toFind) {
			toChange[i] = replacer;
		}
	}
}

bool util_endsWith(const char* str, const char* suffix) {

	bool endsWith = false;

	const unsigned int l_str = strlen(str);
	const unsigned int l_suffix = strlen(suffix);

	if (l_str > l_suffix) {
		endsWith = true;

		unsigned int i;
		for (i = 1; i <= l_suffix; ++i) {
			if (str[l_str - i] != suffix[l_suffix - i]) {
				endsWith = false;
				break;
			}
		}
	}

	return endsWith;
}


#ifdef WIN32
static bool util_isFile(const struct _finddata_t* fileInfo) {
	return !(fileInfo->attrib & _A_SUBDIR)
			&& fileInfo->attrib & (_A_NORMAL | _A_HIDDEN | _A_ARCH);
}
static bool util_isNormalDir(const struct _finddata_t* fileInfo) {
	return strcmp(fileInfo->name, ".") != 0
			&& strcmp(fileInfo->name, "..") != 0
			&& (fileInfo->attrib & _A_SUBDIR);
}
static unsigned int util_listFilesRec(const char* dir, const char* suffix,
		char** fileNames, bool recursive, const unsigned int maxFileNames,
		unsigned int numFileNames, const char* relPath) {

	if (numFileNames >= maxFileNames) {
		return numFileNames;
	}

	struct _finddata_t fileInfo;
	int handle;

	// look for files which end in: suffix
	char suffixFilesSpec[strlen(dir) + strlen("\\*") + strlen(suffix) + 1];
	STRCPY(suffixFilesSpec, dir);
	STRCAT(suffixFilesSpec, "\\*");
	STRCAT(suffixFilesSpec, suffix);
	handle = _findfirst(suffixFilesSpec, &fileInfo);
	if (handle != -1L) {
		if (util_isFile(&fileInfo)) {
			fileNames[numFileNames++] = util_allocStrCpy(fileInfo.name);
		}
		while (_findnext(handle, &fileInfo) == 0
				&& numFileNames < maxFileNames) {
			if (util_isFile(&fileInfo)) {
				char fileRelPath[strlen(relPath) + strlen(fileInfo.name) + 1];
					STRCPY(fileRelPath, relPath);
					STRCAT(fileRelPath, fileInfo.name);
				fileNames[numFileNames++] = util_allocStrCpy(fileRelPath);
			}
		}
		_findclose(handle);
	}

	// search in sub-directories
	if (recursive) {
		char subDirsSpec[strlen(dir) + strlen("\\*.*") + 1];
		STRCPY(subDirsSpec, dir);
		STRCAT(subDirsSpec, "\\*.*");
		handle = _findfirst(subDirsSpec, &fileInfo);
		if (handle != -1L) {
			// check if not current or parent directories
			if (util_isNormalDir(&fileInfo)) {
				char subDir[strlen(dir) + strlen("\\")
						+ strlen(fileInfo.name) + 1];
				STRCPY(subDir, dir);
				STRCAT(subDir, "\\");
				STRCAT(subDir, fileInfo.name);
				char subRelPath[strlen(relPath) + strlen(fileInfo.name)
						+ strlen("\\") + 1];
				STRCPY(subRelPath, relPath);
				STRCAT(subRelPath, fileInfo.name);
				STRCAT(subRelPath, "\\");
				numFileNames = util_listFilesRec(subDir, suffix, fileNames,
						recursive, maxFileNames, numFileNames, subRelPath);
			}
			while (_findnext(handle, &fileInfo) == 0
					&& numFileNames < maxFileNames) {
				if (util_isNormalDir(&fileInfo)) {
					char subDir[strlen(dir) + strlen("\\")
							+ strlen(fileInfo.name) + 1];
					STRCPY(subDir, dir);
					STRCAT(subDir, "\\");
					STRCAT(subDir, fileInfo.name);
					char subRelPath[strlen(relPath) + strlen(fileInfo.name)
							+ strlen("\\") + 1];
					STRCPY(subRelPath, relPath);
					STRCAT(subRelPath, fileInfo.name);
					STRCAT(subRelPath, "\\");
					numFileNames = util_listFilesRec(subDir, suffix, fileNames,
							recursive, maxFileNames, numFileNames, subRelPath);
				}
			}
			_findclose(handle);
		}
	}

	return numFileNames;
}
unsigned int util_listFiles(const char* dir, const char* suffix,
		char** fileNames, bool recursive, const unsigned int maxFileNames) {
	return util_listFilesRec(
			dir, suffix, fileNames, recursive, maxFileNames, 0, "");
}
#else

static const char* fileSelectorSuffix = NULL;

static void util_initFileSelector(const char* suffix) {
	fileSelectorSuffix = suffix;
}
static int util_fileSelector(const struct dirent* fileDesc) {
	return util_endsWith(fileDesc->d_name, fileSelectorSuffix);
}

static unsigned int util_listFilesU(const char* dir, struct dirent*** files) {

	int foundDirs = scandir(dir, files, util_fileSelector, alphasort);

	if (foundDirs < 0) { // error, act as if no file found
		foundDirs = 0;
	}

	return (unsigned int) foundDirs;
}

static unsigned int util_listFilesRec(const char* dir, const char* suffix,
		char** fileNames, bool recursive, const unsigned int maxFileNames,
		unsigned int numFiles, const char* relPath) {

	struct dirent** files;
	util_initFileSelector(suffix);
	unsigned int currentNumFiles = util_listFilesU(dir, &files);
	unsigned int f;
	for (f = 0; f < currentNumFiles && numFiles < maxFileNames; ++f) {
		char fileRelPath[strlen(relPath) + strlen(files[f]->d_name) + 1];
			STRCPY(fileRelPath, relPath);
			STRCAT(fileRelPath, files[f]->d_name);
		fileNames[numFiles++] = util_allocStrCpy(fileRelPath);
	}
/*
	for (; f < currentNumFiles; ++f) {
		free(files[f]);
	}
*/

	if (recursive) {
		struct stat dirStat;
		util_initFileSelector("");
		currentNumFiles = util_listFilesU(dir, &files);
		for (f = 0; f < currentNumFiles && numFiles < maxFileNames; ++f) {
			if (strcmp(files[f]->d_name, ".") == 0
					|| strcmp(files[f]->d_name, "..") == 0) {
				continue;
			}
			char subDir[strlen(dir) + strlen("/") + strlen(relPath)
					+ strlen(files[f]->d_name) + 1];
			STRCPY(subDir, dir);
			STRCAT(subDir, "/");
			STRCAT(subDir, relPath);
			STRCAT(subDir, files[f]->d_name);
			char subRelPath[strlen(relPath) + strlen(files[f]->d_name)
					+ strlen("/") + 1];
			STRCPY(subRelPath, relPath);
			STRCAT(subRelPath, files[f]->d_name);
			STRCAT(subRelPath, "/");
			int retStat = stat(subDir, &dirStat);
			if (retStat == 0 && S_ISDIR(dirStat.st_mode)) {
				numFiles = util_listFilesRec(subDir, suffix, fileNames,
						recursive, maxFileNames, numFiles, subRelPath);
			}
		}
	}

	return numFiles;
}
unsigned int util_listFiles(const char* dir, const char* suffix,
		char** fileNames, bool recursive, const unsigned int maxFileNames) {
	return util_listFilesRec(
			dir, suffix, fileNames, recursive, maxFileNames, 0, "");
}
#endif

bool util_fileExists(const char* filePath) {

	struct stat fileInfo;
	bool exists;
	int intStat;

	// Attempt to get the file attributes
	intStat = stat(filePath, &fileInfo);
	if (intStat == 0) {
		// We were able to get the file attributes
		// so the file obviously exists.
		exists = true;
	} else {
		// We were not able to get the file attributes.
		// This may mean that we don't have permission to
		// access the folder which contains this file. If you
		// need to do that level of checking, lookup the
		// return values of stat which will give you
		// more details on why stat failed.
		exists = false;
	}

	return exists;
}

bool util_makeDir(const char* dirPath) {

	#ifdef	WIN32
		int mkStat = _mkdir(dirPath);
		if (mkStat == 0) {
			return true;
		} else {
			return false;
		}
	#else	// WIN32
		// with read/write/search permissions for owner and group,
		// and with read/search permissions for others
		int mkStat = mkdir(dirPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (mkStat == 0) {
			return true;
		} else {
			return false;
		}
	#endif	// WIN32
}

bool util_makeDirRecursive(const char* dirPath) {

	if (!util_fileExists(dirPath)) {
		char parentDir[strlen(dirPath)+1];
		bool hasParent = util_getParentDir(dirPath, parentDir);
		if (hasParent) {
			bool parentExists = util_makeDirRecursive(parentDir);
			if (parentExists) {
				return util_makeDir(dirPath);
			}
		}
		return false;
	}

	return true;
}

bool util_getParentDir(const char* path, char* parentPath) {

	//size_t pos = strcspn(dirPath, "/\\");
	char* ptr = strrchr(path, '/'); // search char from end reverse
	if (ptr == NULL) {
		ptr = strrchr(path, '\\'); // search char from end reverse
		if (ptr == NULL) {
			return false;
		}
	}

	// copy the parent substring to parentPath
	unsigned int i;
	for (i = 0; &(path[i+1]) != ptr; i++) {
		parentPath[i] = path[i];
	}
	parentPath[i] = '\0';

	return true;
}

bool util_findFile(const char* dirs[], unsigned int numDirs,
		const char* relativeFilePath, char* absoluteFilePath) {

	bool found = false;

	// check if it is an absolute file path
	if (util_fileExists(relativeFilePath)) {
		STRCPY(absoluteFilePath, relativeFilePath);
		found = true;
	}

	unsigned int d;
	for (d=0; d < numDirs && !found; ++d) {
		// do the following: tmpPath = dirs[d] + sPS + relativeFilePath
		char* tmpPath = util_allocStr(strlen(dirs[d]) + 1 + strlen(relativeFilePath));
		//char tmpPath[strlen(dirs[d]) + 1 + strlen(relativeFilePath) + 1];
		tmpPath[0]= '\0';
		tmpPath = STRCAT(tmpPath, dirs[d]);
		tmpPath = STRCAT(tmpPath, sPS);
		tmpPath = STRCAT(tmpPath, relativeFilePath);

		if (util_fileExists(tmpPath)) {
			STRCPY(absoluteFilePath, tmpPath);
			found = true;
		}

		free(tmpPath);
	}

	return found;
}

bool util_findDir(const char* dirs[], unsigned int numDirs,
		const char* relativeDirPath, char* absoluteDirPath,
		bool searchOnlyWriteable, bool create) {

	bool found = false;

	// check if it is an absolute file path
	if (util_fileExists(relativeDirPath)) {
		STRCPY(absoluteDirPath, relativeDirPath);
		found = true;
	}

	if (searchOnlyWriteable && numDirs > 1) {
		numDirs = 1;
	}

	unsigned int d;
	for (d=0; d < numDirs && !found; ++d) {
		// do the following: tmpPath = dirs[d] + sPS + relativeFilePath
		char* tmpPath = util_allocStr(strlen(dirs[d]) + 1 + strlen(relativeDirPath));
		//char tmpPath[strlen(dirs[d]) + 1 + strlen(relativeDirPath) + 1];
		tmpPath[0]= '\0';
		tmpPath = STRCAT(tmpPath, dirs[d]);
		tmpPath = STRCAT(tmpPath, sPS);
		tmpPath = STRCAT(tmpPath, relativeDirPath);

		if (util_fileExists(tmpPath)) {
			STRCPY(absoluteDirPath, tmpPath);
			found = true;
		}

		free(tmpPath);
	}

	// not found -> create it
	if (!found && create && numDirs >= 1) {
		STRCAT(absoluteDirPath, dirs[0]);
		STRCAT(absoluteDirPath, sPS);
		STRCAT(absoluteDirPath, relativeDirPath);
		found = util_makeDir(absoluteDirPath);
	}

	return found;
}

const char* util_map_getValueByKey(
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues,
		const char* key) {

	const char* value = NULL;

	unsigned int i;
	for (i = 0; i < infoSize; i++) {
		if (strcmp(infoKeys[i], key) == 0) {
			value = infoValues[i];
			break;
		}
	}

	return value;
}

