// BackgroundReader.h
///////////////////////////////////////////////////////////////////////////

#ifndef __BACKGROUND_READER_H__
#define __BACKGROUND_READER_H__

#include "archdef.h"

#include <deque>
#include <string>
/* TODO (Dave#1#): Remove dependancy on windows.h */
#ifdef ARCHDEF_PLATFORM_WINDOWS
    #include <windows.h>
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
#ifndef NO_WINSTUFF
	OVERLAPPED curReadInfo;
	HANDLE curHandle;
#endif
	void Update(void);
};

extern CBackgroundReader backgroundReader;

#endif //__BACKGROUND_READER_H__
