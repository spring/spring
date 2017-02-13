/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _POOL_ARCHIVE_H
#define _POOL_ARCHIVE_H

#include <zlib.h>

#include "ArchiveFactory.h"
#include "BufferedArchive.h"


/**
 * Creates pool (aka rapid) archives.
 * @see CPoolArchive
 */
class CPoolArchiveFactory : public IArchiveFactory {
public:
	CPoolArchiveFactory();
private:
	IArchive* DoCreateArchive(const std::string& filePath) const;
};


/**
 * The pool archive format (aka rapid) is specifically developed for spring.
 * It is tailored at incremental downloading of content.
 *
 * Practical example
 * -----------------
 * When first downloading ModX 1.0, then ModX 1.1 and later ModX 1.2, this means:
 * traditional:
 * - downloading ModX10.sd7 (30MB)
 * - downloading ModX11.sd7 (30MB)
 * - downloading ModX12.sd7 (30MB)
 * pool:
 * - downloading all files of ModX10.sd7 (40MB)
 * - downloading only the files that changed between 1.0 and 1.1 (100KB)
 * - downloading only the files that changed between 1.1 and 1.2 (50KB)
 *
 * This is not limited to mods, but is most suitable there, as maps usually do
 * not have a lot of versions which are publicly used.
 *
 * Technical details
 * -----------------
 * The pool system uses two directories, to be found in the root of a spring
 * data directory, pool and packages. They may look as follows:
 * /pool/00/00756ec29fe8fc9d3da9b711e76bc9.gz
 * /pool/00/3427d26f419dabe74eaf7b865407b8.gz
 * ...
 * /pool/01/
 * /pool/02/
 * ...
 * /pool/ff/
 * /packages/a3b3adc55e48aa8ffb723b669d177d2f.sdp
 * /packages/c8c74d288a3b7000638d5bd8e02292bb.sdp
 * ...
 * /packages/selected.list
 *
 * Each .sdp file under packages represents one archive.
 * An .sdp file is only an index, referencing all the files it contains.
 * This referenced files, the real content of this archive is in the pool dir.
 * The splitting up into the 00 till ff sub-dirs is only to prevent possible
 * problems with file-system limits for maximum files per directory (eg. FAT32).
 * /pool/\<first 2 hex chars\>/\<last 30 hex chars\>.gz
 * Format of the .sdp (index) files:
 * There is one entry per indexed file. These are repeated until EOF.
 * The format of one such entry:
 * \<1 byte real file name length\>\<real file name\>\<16 byte MD5 digest\>\<4 byte CRC32\>\<4 byte file size\>
 * The 16 bytes MD5 digest is the reference to the 32 Hex char file name
 * under pool, which contains the content.
 *
 * @author Chris Clearwater (det) <chris@detrino.org>
 */
class CPoolArchive : public CBufferedArchive
{
public:
	CPoolArchive(const std::string& name);
	virtual ~CPoolArchive();

	virtual bool IsOpen();

	virtual unsigned NumFiles() const;
	virtual void FileInfo(unsigned int fid, std::string& name, int& size) const;
	virtual unsigned GetCrc32(unsigned int fid);

protected:
	virtual bool GetFileImpl(unsigned int fid, std::vector<std::uint8_t>& buffer);

	struct FileData {
		std::string name;
		unsigned char md5[16];
		unsigned int crc32;
		unsigned int size;
	};

private:
	bool isOpen;
	std::vector<FileData*> files;
};

#endif // _POOL_ARCHIVE_H
