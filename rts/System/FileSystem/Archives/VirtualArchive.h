/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _VIRTUAL_ARCHIVE_H
#define _VIRTUAL_ARCHIVE_H

#include "ArchiveFactory.h"
#include "IArchive.h"
#include <map>
#include <string>

class CVirtualArchive;

/**
 * Creates virtual non-existing archives which can be created during runtime
 */
class CVirtualArchiveFactory : public IArchiveFactory {
public:
	CVirtualArchiveFactory();
	virtual ~CVirtualArchiveFactory();
	void Clear();

	/**
	 * Registers and creates a new virtual archive to the factory
	 * @param filePath Path with which the archive can be found and opened
	 */
	CVirtualArchive* AddArchive(const std::string& fileName);
	CVirtualArchive* GetArchive(const std::string& fileName) const;
	bool RemoveArchive(const std::string& fileName);
	const std::map<std::string, CVirtualArchive*>& GetAllArchives() const;

private:
	IArchive* DoCreateArchive(const std::string& fileName) const;

	std::map<std::string, CVirtualArchive*> archives;
};

extern CVirtualArchiveFactory* virtualArchiveFactory;

/**
 * A file in a virtual archive
 */
class CVirtualFile
{
public:
	CVirtualFile(int fid, const std::string& name);
	virtual ~CVirtualFile();

	std::vector<std::uint8_t> buffer;

	const std::string& GetName() const
	{ return name; }

	int GetFID() const
	{ return fid; }

private:
	const std::string name;
	int fid;
};

/**
 * An opened virtual archive, as archives get deleted after being
 * opened with IArchiveFactory::DoCreateArchive all the data in a virtual archive is lost.
 * Therefore an 'opened' class is created and the actual archive is preserved
 */
class CVirtualArchiveOpen : public IArchive
{
public:
	CVirtualArchiveOpen(CVirtualArchive* archive, const std::string& fileName);
	virtual ~CVirtualArchiveOpen();

	virtual bool IsOpen();
	virtual unsigned int NumFiles() const;
	virtual bool GetFile(unsigned int fid, std::vector<std::uint8_t>& buffer);
	virtual void FileInfo(unsigned int fid, std::string& name, int& size) const;

private:
	CVirtualArchive* archive;
};

/**
 * A virtual archive
 */
class CVirtualArchive
{
public:
	CVirtualArchive(const std::string& fileName);
	virtual ~CVirtualArchive();

	CVirtualArchiveOpen* Open();
	CVirtualFile* AddFile(const std::string& file);
	CVirtualFile* GetFile(const std::string& file);

	unsigned int NumFiles() const;
	bool GetFile(unsigned int fid, std::vector<std::uint8_t>& buffer);
	void FileInfo(unsigned int fid, std::string& name, int& size) const;

	const std::string& GetArchiveName() const
	{ return fileName; }

	const std::map<std::string, unsigned int>& GetNameIndex() const
	{ return lcNameIndex; }

	void WriteToFile();

private:
	friend class CVirtualArchiveOpen;

	std::vector<CVirtualFile*> files;
	std::string fileName;
	std::map<std::string, uint> lcNameIndex;
};

#endif // _VIRTUAL_ARCHIVE_H
