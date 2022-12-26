/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GIT_ARCHIVE_H
#define _GIT_ARCHIVE_H

#include <map>
#include <git2.h>

#include "IArchiveFactory.h"
#include "IArchive.h"


/**
 * Creates file-system/dir oriented archives.
 * @see CDirArchive
 */
class CGitArchiveFactory : public IArchiveFactory {
public:
	CGitArchiveFactory();
private:
	IArchive* DoCreateArchive(const std::string& filePath) const override;
};

struct searchfile {
	std::string filename;
	git_blob* blob = nullptr;
};

/**
 * Archive implementation which falls back to the regular file-system.
 * ie. a directory and all its contents are treated as an archive by this
 * class.
 */

class CGitArchive : public IArchive
{
public:
	CGitArchive(const std::string& archiveName);
	~CGitArchive();

	int GetType() const override { return ARCHIVE_TYPE_GIT; }

	bool IsOpen() override { return true; }

	unsigned int NumFiles() const override { return (searchFiles.size()); }
	bool GetFile(unsigned int fid, std::vector<std::uint8_t>& buffer) override;
	void FileInfoName(unsigned int fid, std::string& name) const override;
	void FileInfoSize(unsigned int fid, int& size) const override;

private:
	void LoadFilenames(const std::string dirname, git_tree *tree);
	const std::string dirName;
	mutable std::vector<searchfile> searchFiles;
	git_repository * Repo = nullptr;
	git_tree *tree_root = nullptr;
	git_reference *reference_root = nullptr;
};

#endif // _GIT_ARCHIVE_H
