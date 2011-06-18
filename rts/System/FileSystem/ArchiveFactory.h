/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ARCHIVE_FACTORY_H
#define _ARCHIVE_FACTORY_H

#include <map>
#include <string>

class IArchive;

/**
 * Specific archive type side interface for loading of archives.
 * An implementation od this is responsible for parsing a single, specific
 * type of archives.
 */
class IArchiveFactory
{
protected:
	/**
	 * @param defaultExtension for example "sdz"
	 */
	IArchiveFactory(const std::string& defaultExtension)
		: defaultExtension(defaultExtension)
	{}

public:
	virtual ~IArchiveFactory() {}

	/**
	 * @return for example "sdz"
	 */
	const std::string& GetDefaultExtension() const { return defaultExtension; }

	/**
	 * Parses a single archive, denoted by a path.
	 */
	IArchive* CreateArchive(const std::string& filePath) const {
		return DoCreateArchive(filePath);
	}

private:
	/**
	 * Parses a single archive of a specific type.
	 */
	virtual IArchive* DoCreateArchive(const std::string& filePath) const = 0;

	const std::string defaultExtension;
};

#define archiveLoader CArchiveLoader::GetInstance()

#endif // _ARCHIVE_FACTORY_H
