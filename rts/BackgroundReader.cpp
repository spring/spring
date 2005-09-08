#include "StdAfx.h"
#include "BackgroundReader.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#endif
//#include "mmgr.h"

CBackgroundReader backgroundReader;

#ifdef _WIN32
void FileIOComplete(Uint32 dwErrorCode, Uint32 dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
	CloseHandle(backgroundReader.curHandle);
	backgroundReader.curHandle=0;
	backgroundReader.curFile.reportReady++;

//	MessageBox(0,"Read complete","rc",0);

	backgroundReader.Update();
}
#endif

CBackgroundReader::CBackgroundReader(void)
{
	curFile.length=0;
#ifdef _WIN32
	curHandle=0;
#elif defined(HAS_LIBAIO)
	aio_setup(1024);
#endif
}

CBackgroundReader::~CBackgroundReader(void) 
{
#ifdef _WIN32
	while(curHandle)
		SleepEx(10,true);
#endif
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
#ifdef _WIN32
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
#elif defined(HAS_LIBAIO)
	if(quedFiles.empty()){
		return;
	}
	curFile=quedFiles.front();
	quedFiles.pop_front();
	if ((srcfd=open(curFile.name.c_str(),O_RDONLY))<0) {
		MessageBox(0,curFile.name.c_str(),"Failed to open file for background reading",0);
		curFile.reportReady++;
		Update();
		return;
	}
	if (curFile.length == 0) {
		struct stat st;
		if (fstat(srcfd,&st)<0) {
			MessageBox(0,curFile.name.c_str(),"Failed to read file information",0);
			close(srcfd);
			Update();
			return;
		}
		curFile.length = st.st_size;
	}
	struct iocb iocb;
	io_prep_pread(&iocb,srcfd,curFile.buf,curFile.length,0);
	int res = sync_submit(&iocb);
	if (res<=0) {
		MessageBox(0,curFile.name.c_str(),"Failed to read file",0);
		close(srcfd);
		Update();
		return;
	}
#endif
}

#if !defined(_WIN32) && defined(HAS_LIBAIO)
void CBackgroundReader::aio_setup(int n)
{
	int res = io_queue_init(n, &io_ctx);
}

int CBackgroundReader::sync_submit(struct iocb *iocb)
{
	struct io_event event;
	struct iocb *iocbs[] = { iocb };
	int res;
	struct timespec ts;
	ts.tv_sec = 15;
	ts.tv_nsec = 0;
	res = io_submit(io_ctx,1,iocbs);
	if (res!=1) {
		MessageBox(0,curFile.name.c_str(),"sync_submit: io_submit",0);
		return res;
	}
	res = io_getevents(io_ctx,1,&event,&ts);
	if (res!=1) {
		MessageBox(0,curFile.name.c_str(),"sync_submit: io_getevents",0);
		return res;
	}
	return event.res;
}
#endif
