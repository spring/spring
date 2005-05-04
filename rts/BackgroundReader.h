#pragma once

#include <deque>
#include <string>
#include <windows.h>

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

	OVERLAPPED curReadInfo;
	HANDLE curHandle;
	void Update(void);
};

extern CBackgroundReader backgroundReader;
