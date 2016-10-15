/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _DIR_ARCHIVE_H
#define _DIR_ARCHIVE_H

#include <map>

#include "ArchiveFactory.h"
#include "IArchive.h"


/**
 * Creates file-system/dir oriented archives.
 * @see CDirArchive
 */
class CDirArchiveFactory : public IArchiveFactory {
public:
	CDirArchiveFactory();
private:
	IArchive* DoCreateArchive(const std::string& filePath) const;
};


/**
 * Archive implementation which falls back to the regular file-system.
 * ie. a directory and all its contents are treated as an archive by this
 * class.
 */
class CDirArchive : public IArchive
{
public:
	CDirArchive(const std::string& archiveName);
	virtual ~CDirArchive();

	virtual bool IsOpen();

	virtual unsigned int NumFiles() const;
	virtual bool GetFile(unsigned int fid, std::vector<std::uint8_t>& buffer);
	virtual void FileInfo(unsigned int fid, std::string& name, int& size) const;

private:
	/// "ExampleArchive.sdd/"
	std::string dirName;

	std::vector<std::string> searchFiles;
};

#endif // _DIR_ARCHIVE_H
