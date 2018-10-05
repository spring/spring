/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CACHE_DIR_H
#define _CACHE_DIR_H

#include <string>

/**
 * Utility class, which helps in tagging a directory as being cache only,
 * by saving a certain, well defiend file in this dir.
 * This tag can be used when creating backups for example,
 * where backups would exclude dirs tagged with this method.
 * The file used as a tag:
 * name:    "CACHEDIR.TAG"
 * content: "Signature: 8a477f597d28d172789f06886806bc55"
 *
 * This is following a convention described here (DEAD LINKS):
 * http://www.brynosaurus.com/cachedir/
 * and defined here:
 * http://www.brynosaurus.com/cachedir/spec.html
 */
class CacheDir {
	CacheDir() = delete;
	CacheDir(const CacheDir&) = delete;

public:
	static const std::string tagFile_name;
	static const std::string tagFile_content;
	static const size_t      tagFile_content_size;
	static const std::string defaultAdditionalText;

	/**
	 * Checks if the dir is marked as a cache dir.
	 * @return true if the dir exists and is tagged as cache dir, false otherwise
	 */
	static bool IsCacheDir(const std::string& dir);

	/**
	 * Sets or unsets a dir as cache dir.
	 * This removes or creates the tag file.
	 * @param dir directory to be used as cache
	 * @param wantedCacheState if true, writes a CACHEDIR.TAG
	 * @param additionalText only used if (wantedCacheState == true), is appended in the cache tag file
	 * @param forceRewrite if set to true, the tag file will be rewritten even if a valid one already exists (default: false)
	 * @return true if the dir tag file was successfully removed, created or already existed, false otherwise
	 */
	static bool SetCacheDir(const std::string& dir, bool wantedCacheState,
	                        const std::string& additionalText = CacheDir::defaultAdditionalText,
	                        bool forceRewrite = false);

private:
	/**
	 * Checks if the specified files content starts with the first content_size
	 * chars of content.
	 * @param filePath the file which content to compare to
	 * @param content the chars to compare to
	 * @param content_size how many chars to compare
	 * @return true if the this is a cache tag file, false otherwise
	 */
	static bool FileContentStartsWith(const std::string& filePath, const std::string& content, size_t content_size);
	/**
	 * Writes the cache tag file content to filePath.
	 * @return true if all content was successfully written, false otherwise
	 */
	static bool WriteCacheTagFile(const std::string& filePath, const std::string& additionalText);
	static std::string GetCacheTagFilePath(const std::string& dir);
};

#endif // _CACHE_DIR_H
