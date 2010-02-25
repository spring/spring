/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ARCHIVEDIR_H
#define ARCHIVEDIR_H

#include <map>

#include "ArchiveBase.h"

/**
 * Archive implementation which falls back to the regular filesystem.
 * ie. a directory and all it's contents is treated as an archive by this class.
 */
class CArchiveDir : public CArchiveBase
{
public:
	CArchiveDir(const std::string& archiveName);
	virtual ~CArchiveDir(void);
	
	virtual bool IsOpen();
	
	virtual unsigned NumFiles() const;
	virtual bool GetFile(unsigned fid, std::vector<boost::uint8_t>& buffer);
	virtual void FileInfo(unsigned fid, std::string& name, int& size) const;
	
private:
	std::string archiveName; ///< "ExampleArchive.sdd/"

	std::vector<std::string> searchFiles;
};

#endif
