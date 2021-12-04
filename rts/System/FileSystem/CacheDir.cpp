/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CacheDir.h"

#include "FileSystem.h"

#include <cstdio>
#include <string>

// as defiend here (DEAD LINK): http://www.brynosaurus.com/cachedir/spec.html
const std::string CacheDir::tagFile_name          = "CACHEDIR.TAG";
const std::string CacheDir::tagFile_content       = "Signature: 8a477f597d28d172789f06886806bc55";
const size_t      CacheDir::tagFile_content_size  = 43;
const std::string CacheDir::defaultAdditionalText = "# This file is a cache directory tag created by Spring.\n"
                                                    "# For information about cache directory tags, see:\n"
                                                    "# http://www.brynosaurus.com/cachedir/";


bool CacheDir::IsCacheDir(const std::string& dir) {
	const std::string cacheFile = GetCacheTagFilePath(dir);
	bool isTagged = CacheDir::FileContentStartsWith(cacheFile, CacheDir::tagFile_content, CacheDir::tagFile_content_size);

	return isTagged;
}

bool CacheDir::SetCacheDir(const std::string& dir, bool wantedCacheState, const std::string& additionalText, bool forceRewrite) {
	bool requestedStatePresent = false;

	const bool currentCacheState = CacheDir::IsCacheDir(dir);

	const std::string cacheFile = GetCacheTagFilePath(dir);
	if (currentCacheState == wantedCacheState) {
		// requested state was already present
		if (wantedCacheState && forceRewrite) {
			requestedStatePresent = CacheDir::WriteCacheTagFile(cacheFile, additionalText);
		} else {
			requestedStatePresent = true;
		}
	} else {
		// we have to swap the state
		if (wantedCacheState) {
			requestedStatePresent = CacheDir::WriteCacheTagFile(cacheFile, additionalText);
		} else {
			requestedStatePresent = FileSystem::DeleteFile(cacheFile);
		}
	}

	return requestedStatePresent;
}

bool CacheDir::FileContentStartsWith(const std::string& filePath, const std::string& content, size_t content_size) {
	bool startsWith = false;

	FILE* fileH = ::fopen(filePath.c_str(), "r");
	if (fileH != nullptr) {
		content_size = ((content_size > content.size()) ? content.size() : content_size);
		char currFileChar;
		size_t i = 0;
		startsWith = true;
		while (((currFileChar = fgetc(fileH)) != EOF) && (i < content_size)) {
			if (currFileChar != content[i]) {
				startsWith = false;
				break;
			}
			i++;
		}
		fclose(fileH);
	}

	return startsWith;
}

bool CacheDir::WriteCacheTagFile(const std::string& filePath, const std::string& additionalText) {
	bool fileWritten = false;

	FILE* fileH = ::fopen(filePath.c_str(), "w");
	if (fileH != nullptr) {
		int ret;
		ret = fputs(CacheDir::tagFile_content.c_str(), fileH);
		if (!additionalText.empty() && (ret != EOF)) {
			ret = fputs("\n", fileH);
			if (ret != EOF) {
				ret = fputs(additionalText.c_str(), fileH);
			}
		}
		fileWritten = (ret != EOF);
		fclose(fileH);
	}

	return fileWritten;
}

std::string CacheDir::GetCacheTagFilePath(const std::string& dir) {
	std::string cacheFile = dir;
	FileSystem::EnsurePathSepAtEnd(cacheFile);
	cacheFile = cacheFile + CacheDir::tagFile_name;

	return cacheFile;
}
