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

#include "InterfaceUtil.h"

#include "ExternalAI/Interface/SStaticGlobalData.h" // for sPS

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

static const char* myDataDir = NULL;
static const char* myDataDirVers = NULL;

char* util_setDataDirs(const char* unversioned, const char* versioned) {

	myDataDir = unversioned;
	myDataDirVers = versioned;
}
const char* util_getDataDirUnversioned() {
	return myDataDir;
}
const char* util_getDataDirVersioned() {
	return myDataDirVers;
}

char* util_allocStr(unsigned int length) {
	return (char*) calloc(length+1, sizeof(char));
}

char* util_allocStrCpy(const char* toCopy) {
	
	char* copy = (char*) calloc(strlen(toCopy)+1, sizeof(char));
	STRCPY(copy, toCopy);
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
#define JARS_REGEX "/*.jar"

unsigned int util_listFiles(const char* dir, const char* suffix,
		char** fileNames, const unsigned int maxFileNames) {

	unsigned int i = 0;
	
	char mDir[strlen(dir) + strlen("/*") + strlen(suffix) + 1];
	strcpy(mDir, dir);
	strcat(mDir, "/*");
	strcat(mDir, suffix);

	struct _finddata_t fileInfo;
	int handle = _findfirst(mDir, &fileInfo);
	if (handle == -1L) {
		return i;
	}
	fileNames[i++] = util_allocStrCpy(fileInfo.name);
	while (_findnext(handle, &fileInfo) == 0) {
		fileNames[i++] = util_allocStrCpy(fileInfo.name);
	}
	_findclose(handle);

	return i;
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

unsigned int util_listFiles(const char* dir, const char* suffix,
		char** fileNames, const unsigned int maxFileNames) {

	unsigned int numFiles;
	
	//struct dirent* files[maxFileNames];
	struct dirent** files;
	util_initFileSelector(suffix);
	numFiles = util_listFilesU(dir, &files);
	if (numFiles > 0) {
		unsigned int cnt;
		for (cnt = 0; cnt < numFiles; ++cnt) {
			fileNames[cnt] = util_allocStrCpy(files[cnt]->d_name);
		}
	}
/*
	DIR *dir_p;
	struct dirent* dir_entry_p = NULL;
	dir_p = opendir(dir);
	for (numFiles = 0; (dir_entry_p = readdir(dir_p)) != NULL; ) {
		if (util_endsWith(dir_entry_p->d_name, suffix)) {
			fileNames[numFiles++] = util_allocStrCpy(dir_entry_p->d_name);
		}
	}
*/

	return numFiles;
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
	
	if (!fileExists(dirPath)) {
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
	
	unsigned int d;
	for (d=0; d < numDirs && !found; ++d) {
		// do the following: tmpPath = dirs[d] + sPS + relativeFilePath
		char* tmpPath = util_allocStr(strlen(dirs[d]) + 1 + strlen(relativeFilePath));
		//char tmpPath[strlen(dirs[d]) + 1 + strlen(relativeFilePath) + 1];
		tmpPath[0]= '\0';
		tmpPath = strcat(tmpPath, dirs[d]);
		tmpPath = strcat(tmpPath, sPS);
		tmpPath = strcat(tmpPath, relativeFilePath);
		
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
	
	if (searchOnlyWriteable && numDirs > 1) {
		numDirs = 1;
	}
	
	unsigned int d;
	for (d=0; d < numDirs && !found; ++d) {
		// do the following: tmpPath = dirs[d] + sPS + relativeFilePath
		char* tmpPath = util_allocStr(strlen(dirs[d]) + 1 + strlen(relativeDirPath));
		//char tmpPath[strlen(dirs[d]) + 1 + strlen(relativeDirPath) + 1];
		tmpPath[0]= '\0';
		tmpPath = strcat(tmpPath, dirs[d]);
		tmpPath = strcat(tmpPath, sPS);
		tmpPath = strcat(tmpPath, relativeDirPath);
		
		if (util_fileExists(tmpPath)) {
			STRCPY(absoluteDirPath, tmpPath);
			found = true;
		}
		
		free(tmpPath);
	}
	
	// not found -> create it
	if (!found && create && numDirs >= 1) {
		strcat(absoluteDirPath, dirs[0]);
		strcat(absoluteDirPath, sPS);
		strcat(absoluteDirPath, relativeDirPath);
		found = util_makeDir(absoluteDirPath);
	}
	
	return found;
}

/*
bool fitsThisInterface(const char* reqShortName, const char* reqVersion) {
	
	bool shortNameFits = false;
	bool versionFits = false;
	
	InfoItem info[MAX_INFOS];
	int num = GetInfo(info, MAX_INFOS);
	
	std::string shortNameKey(AI_INTERFACE_PROPERTY_SHORT_NAME);
	std::string versionKey(AI_INTERFACE_PROPERTY_VERSION);
	for (int i=0; i < num; ++i) {
		if (shortNameKey == info[i].key) {
			if (requestedShortName == info[i].value) {
				shortNameFits = true;
			}
		} else if (versionKey == info[i].key) {
			if (requestedVersion == info[i].value) {
				versionFits = true;
			}
		}
	}
	
	return shortNameFits && versionFits;
}
*/

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

