/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ARCHIVE_LOADER_H
#define _ARCHIVE_LOADER_H

#include <string>

class CArchiveBase;

/**
 * Engine side interface for loading of different archive types.
 * This loader is responsible for offering access to different archive types,
 * without the rest of the engine having to know anything about those types.
 */
class CArchiveLoader
{
	CArchiveLoader() {}
public:
	static const CArchiveLoader& GetInstance();

	/// Returns true if the indicated file is in fact an archive
	bool IsArchiveFile(const std::string& fileName) const;

	/// Returns a pointer to a new'ed suitable subclass of CArchiveBase
	CArchiveBase* OpenArchive(const std::string& fileName,
			const std::string& type = "") const;
};

#define archiveLoader CArchiveLoader::GetInstance()

#endif // _ARCHIVE_LOADER_H
