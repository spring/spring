#include "stdafx.h"
#include "backgroundreader.h"
//#include "mmgr.h"

CBackgroundReader backgroundReader;

VOID CALLBACK FileIOComplete(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
	CloseHandle(backgroundReader.curHandle);
	backgroundReader.curHandle=0;
	backgroundReader.curFile.reportReady++;

//	MessageBox(0,"Read complete","rc",0);

	backgroundReader.Update();
}

CBackgroundReader::CBackgroundReader(void)
{
	curFile.length=0;
	curHandle=0;
}

CBackgroundReader::~CBackgroundReader(void) 
{
	while(curHandle)
		SleepEx(10,true);
}

void CBackgroundReader::ReadFile(const char* filename, unsigned char* buffer, int length, int priority, int* reportReady)
{
	FileToRead f;
	f.name=filename;
	f.buf=buffer;
	f.length=length;
	f.reportReady=reportReady;
	quedFiles.push_back(f);

	Update();
}

void CBackgroundReader::Update(void)
{
	if(curHandle==0){
		if(quedFiles.empty()){
			return;
		}
		curFile=quedFiles.front();
		quedFiles.pop_front();

		curHandle=CreateFile(curFile.name.c_str(),GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);

		if(curHandle==INVALID_HANDLE_VALUE){
			MessageBox(0,curFile.name.c_str(),"Failed to open file for background reading",0);
			curFile.reportReady++;
			curHandle=0;
			Update();
			return;
		}
		curReadInfo.Offset=0;
		curReadInfo.OffsetHigh=0;

		if(!ReadFileEx(curHandle,curFile.buf,curFile.length,&curReadInfo,FileIOComplete)){
			MessageBox(0,curFile.name.c_str(),"Failed to read file",0);
			CloseHandle(curHandle);
			curHandle=0;
			Update();
			return;
		}
	}
}
