/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _DIR_ARCHIVE_H
#define _DIR_ARCHIVE_H

#include <map>

#include "IArchiveFactory.h"
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

	int GetType() const override { return ARCHIVE_TYPE_SDD; }

	bool IsOpen() override { return true; }

	unsigned int NumFiles() const override { return (searchFiles.size()); }
	bool GetFile(unsigned int fid, std::vector<std::uint8_t>& buffer) override;
	void FileInfo(unsigned int fid, std::string& name, int& size) const override;
	const std::string& GetOrigFileName(unsigned int fid) const { return searchFiles[fid]; }

private:
	/// "ExampleArchive.sdd/"
	const std::string dirName;

	std::vector<std::string> searchFiles;
};

#endif // _DIR_ARCHIVE_H
