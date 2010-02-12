#ifndef ARCHIVE_POOL_H
#define ARCHIVE_POOL_H

#include <zlib.h>

#include "ArchiveBuffered.h"

class CArchivePool : public CArchiveBuffered
{
public:
	CArchivePool(const std::string& name);
	virtual ~CArchivePool(void);

	virtual bool IsOpen();

	virtual unsigned NumFiles() const;
	virtual void FileInfo(unsigned fid, std::string& name, int& size) const;
	virtual unsigned GetCrc32(unsigned fid);
	
protected:
	virtual bool GetFileImpl(unsigned fid, std::vector<boost::uint8_t>& buffer);

	struct FileData {
		std::string name;
		unsigned char md5[16];
		unsigned int crc32;
		unsigned int size;
	};

	bool isOpen;
	std::vector<FileData*> files;
};

#endif
