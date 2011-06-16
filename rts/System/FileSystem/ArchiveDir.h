/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ARCHIVE_DIR_H
#define _ARCHIVE_DIR_H

#include <map>

#include "ArchiveBase.h"

/**
 * Archive implementation which falls back to the regular filesystem.
 * ie. a directory and all it's contents are treated as an archive by this
 * class.
 */
class CArchiveDir : public CArchiveBase
{
public:
	CArchiveDir(const std::string& archiveName);
	virtual ~CArchiveDir();
	
	virtual bool IsOpen();
	
	virtual unsigned int NumFiles() const;
	virtual bool GetFile(unsigned int fid, std::vector<boost::uint8_t>& buffer);
	virtual void FileInfo(unsigned int fid, std::string& name, int& size) const;
	
private:
	/// "ExampleArchive.sdd/"
	std::string dirName;

	std::vector<std::string> searchFiles;
};

#endif // _ARCHIVE_DIR_H
