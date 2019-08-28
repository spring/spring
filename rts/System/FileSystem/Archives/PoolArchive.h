/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _POOL_ARCHIVE_H
#define _POOL_ARCHIVE_H

#include <zlib.h>
#include <cstring>

#include "IArchiveFactory.h"
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
 * data directory, called "pool" and "packages". They may look as follows:
 *   /pool/00/00756ec29fe8fc9d3da9b711e76bc9.gz
 *   /pool/00/3427d26f419dabe74eaf7b865407b8.gz
 *   ...
 *   /pool/01/
 *   /pool/02/
 *   ...
 *   /pool/ff/
 *   /packages/a3b3adc55e48aa8ffb723b669d177d2f.sdp
 *   /packages/c8c74d288a3b7000638d5bd8e02292bb.sdp
 *   ...
 *   /packages/selected.list
 *
 * Each .sdp file under packages represents one archive.
 * An .sdp file is only an index referencing all the files it contains.
 * The real content of this archive is in the pool directory, split up
 * into 0x00-0xff sub-dirs to avoid filesystem limits (e.g. the maximum
 * number of files per directory for FAT32).
 *   /pool/\<first 2 hex chars\>/\<last 30 hex chars\>.gz
 *
 * The .sdp (index) file contains one entry per indexed file. These are
 * repeated until EOF and formatted as follows:
 *   \<1 byte real file name length\>\<real file name\>\<16 byte MD5 digest\>\<4 byte CRC32\>\<4 byte file size\>
 * The 16-byte MD5 digest is the reference to the 32 hex-char filename
 * under pool/ which contains the content.
 *
 * @author Chris Clearwater (det) <chris@detrino.org>
 */
class CPoolArchive : public CBufferedArchive
{
public:
	CPoolArchive(const std::string& name);
	~CPoolArchive();

	int GetType() const override { return ARCHIVE_TYPE_SDP; }

	bool IsOpen() override { return isOpen; }

	unsigned NumFiles() const override { return (files.size()); }
	void FileInfo(unsigned int fid, std::string& name, int& size) const override {
		assert(IsFileId(fid));
		name = files[fid].name;
		size = files[fid].size;
	}
	bool CalcHash(uint32_t fid, uint8_t hash[sha512::SHA_LEN], std::vector<std::uint8_t>& fb) override {
		assert(IsFileId(fid));

		const FileData& fd = files[fid];

		// pool-entry hashes are not calculated until GetFileImpl, must check JIT
		if (memcmp(fd.shasum.data(), dummyFileHash.data(), sizeof(fd.shasum)) == 0)
			GetFileImpl(fid, fb);

		memcpy(hash, fd.shasum.data(), sha512::SHA_LEN);
		return (memcmp(fd.shasum.data(), dummyFileHash.data(), sizeof(fd.shasum)) != 0);
	}

protected:
	int GetFileImpl(unsigned int fid, std::vector<std::uint8_t>& buffer) override;

	std::pair<uint64_t, uint64_t> GetSums() const {
		std::pair<uint64_t, uint64_t> p;

		for (size_t n = 0; n < files.size(); n++) {
			p.first  += files[n].size;
			p.second += stats[n].readTime;
		}

		return p;
	}

	struct FileData {
		std::string name;
		std::array<uint8_t,              16> md5sum;
		std::array<uint8_t, sha512::SHA_LEN> shasum;

		uint32_t crc32;
		uint32_t size;
	};
	struct FileStat {
		// inverted cmp for descending order
		bool operator < (const FileStat& s) const { return (readTime > s.readTime); }

		uint64_t fileIndx;
		uint64_t readTime;
	};

private:
	bool isOpen = false;

	std::string poolRootDir;
	std::array<uint8_t, sha512::SHA_LEN> dummyFileHash;

	std::vector<FileData> files;
	std::vector<FileStat> stats;
};

#endif // _POOL_ARCHIVE_H
