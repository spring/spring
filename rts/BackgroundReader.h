// BackgroundReader.h
///////////////////////////////////////////////////////////////////////////

#ifndef __BACKGROUND_READER_H__
#define __BACKGROUND_READER_H__

#include <deque>
#include <string>
#ifdef _WIN32
#include <windows.h>
#elif defined(HAS_LIBAIO)
#include <libaio.h>
#endif

class CBackgroundReader
{
public:
	CBackgroundReader(void);
	~CBackgroundReader(void);
	void ReadFile(const char* filename, unsigned char* buffer, int length, int priority, int* reportReady);

	struct FileToRead {
		std::string name;
		unsigned char* buf;
		int length;
		int* reportReady;
	};

	std::deque<FileToRead> quedFiles;
	FileToRead curFile;
#ifdef _WIN32
	OVERLAPPED curReadInfo;
	HANDLE curHandle;
#elif defined(HAS_LIBAIO)
	io_context_t io_ctx;
	int srcfd;
	void aio_setup(int n);
	int sync_submit(struct iocb *iocb);
#endif
	void Update(void);
};

extern CBackgroundReader backgroundReader;

#endif //__BACKGROUND_READER_H__
