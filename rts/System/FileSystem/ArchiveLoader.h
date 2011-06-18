/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ARCHIVE_LOADER_H
#define _ARCHIVE_LOADER_H

#include <map>
#include <string>

class IArchive;
class IArchiveFactory;

/**
 * Engine side interface for loading of different archive types.
 * This loader is responsible for offering access to different archive types,
 * without the rest of the engine having to know anything about those types.
 */
class CArchiveLoader
{
	CArchiveLoader();
	~CArchiveLoader();

public:
	static CArchiveLoader& GetInstance();

	/// Returns true if the indicated file is in fact an archive
	bool IsArchiveFile(const std::string& fileName) const;

	/// Returns a pointer to a new'ed suitable subclass of IArchive
	IArchive* OpenArchive(const std::string& fileName,
			const std::string& type = "") const;

	/**
	 * Registers an archive factory, which handles a single type of archive.
	 * @param archiveFactory has to be new'ed, will be delete'ed
	 */
	void AddFactory(IArchiveFactory* archiveFactory);

private:
	/// maps the default-extension to the corresponding archive factory
	std::map<std::string, IArchiveFactory*> archiveFactories;
};

#define archiveLoader CArchiveLoader::GetInstance()

#endif // _ARCHIVE_LOADER_H
