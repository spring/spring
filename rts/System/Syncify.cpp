#include "StdAfx.h"
#include "Syncify.h"

#ifdef SYNCIFY
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bget.h"
#include "Platform/errorhandler.h"
#include "LogOutput.h"

#ifndef _WIN32
#include <sys/mman.h>
#endif

namespace Syncify{
	void* Alloc(size_t reportedSize);
	void Free(void* reportedAddress);
}

// Not sure this is needed.
#ifdef _WIN32
#define throw(x)
#define nothrow()
#else
#define nothrow() throw()
#endif

void	*operator new(size_t reportedSize, Syncify_t) throw(std::bad_alloc)
{
	void* p = Syncify::Alloc(reportedSize);
	if (!p)
		throw std::bad_alloc();
	return p;
}

void	*operator new[](size_t reportedSize, Syncify_t) throw(std::bad_alloc)
{
	void* p = Syncify::Alloc(reportedSize);
	if (!p)
		throw std::bad_alloc();
	return p;
}

void	operator delete(void *reportedAddress) nothrow()
{
	Syncify::Free(reportedAddress);
}

void	operator delete[](void *reportedAddress) nothrow()
{
	Syncify::Free(reportedAddress);
}

namespace Syncify{

	bool syncifyInitialized=false;

	enum CodeType{
		CodeType_Mixed,
		CodeType_Synced,
		CodeType_Unsynced
	};

	CodeType codeType;

#define SUPER_BLOCK_SIZE (1<<29)
#define ALLOC_BLOCK_SIZE (1<<26)

	void* SyncedMem;
	void* UnsyncedMem;

	unsigned int commitedSynced;		//increase these as more mem is commited (must commit lineary)
	unsigned int commitedUnsynced;

	BGet* heap1;
	BGet* heap2;

	void* Alloc(size_t reportedSize)
	{
		if(!syncifyInitialized)
			return malloc(reportedSize);

		switch(codeType){
			case CodeType_Mixed:
				return malloc(reportedSize);
			case CodeType_Synced:
				return heap1->bget(reportedSize);
			case CodeType_Unsynced:
				return heap2->bget(reportedSize);
		}
		return 0;
	}

	void Free(void *reportedAddress)
	{
		if(!syncifyInitialized)
			return;

		if(reportedAddress>SyncedMem && reportedAddress<((char*)SyncedMem)+SUPER_BLOCK_SIZE){
			heap1->brel(reportedAddress);
		} else if(reportedAddress>UnsyncedMem && reportedAddress<((char*)UnsyncedMem)+SUPER_BLOCK_SIZE){
			heap2->brel(reportedAddress);
		} else {
			free(reportedAddress);
		}
	}

	void* HeapExpand1(bufsize size)
	{
		// no need to do anything on linux
#ifdef _WIN32
		VirtualAlloc((char*)SyncedMem+commitedSynced,ALLOC_BLOCK_SIZE,MEM_COMMIT,PAGE_READWRITE);		//only reserves mem, will need to commit mem as needed
#endif
		commitedSynced+=ALLOC_BLOCK_SIZE;
		assert(commitedSynced <= SUPER_BLOCK_SIZE);
		return (char*)SyncedMem+commitedSynced-ALLOC_BLOCK_SIZE;
	}
	void* HeapExpand2(bufsize size)
	{
		// no need to do anything on linux
#ifdef _WIN32
		VirtualAlloc((char*)UnsyncedMem+commitedUnsynced,ALLOC_BLOCK_SIZE,MEM_COMMIT,PAGE_READWRITE);		//only reserves mem, will need to commit mem as needed
#endif
		commitedUnsynced+=ALLOC_BLOCK_SIZE;
		assert(commitedUnsynced <= SUPER_BLOCK_SIZE);
		return (char*)UnsyncedMem+commitedUnsynced-ALLOC_BLOCK_SIZE;
	}

	void InitSyncify()
	{
		if(syncifyInitialized)
			return;
		codeType=CodeType_Mixed;

#ifdef _WIN32
		SyncedMem=VirtualAlloc(0,SUPER_BLOCK_SIZE,MEM_RESERVE,PAGE_READWRITE);		//only reserves mem, will need to commit mem as needed
		UnsyncedMem=VirtualAlloc(0,SUPER_BLOCK_SIZE,MEM_RESERVE,PAGE_READWRITE);
#else
		// As far as I know theres no such thing as reserving/committing memory on linux.
		// Kernel keeps track of which pages are actually used (ie. have to be in RAM/swap)
		// and which are only "reserved". So this reserves the memory and there's nothing
		// to do in HeapExpand1() and HeapExpand2().
		// void * mmap (void *ADDRESS, size_t LENGTH, int PROTECT, int FLAGS, int FILEDES, off_t OFFSET);
		SyncedMem   = mmap(0, SUPER_BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
		UnsyncedMem = mmap(0, SUPER_BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
		if (SyncedMem == MAP_FAILED || UnsyncedMem == MAP_FAILED)
			handleerror(0, strerror(errno), "Syncify: mmap failed", 0);
#endif

		heap1=new BGet();
		heap1->bectl(HeapExpand1,ALLOC_BLOCK_SIZE);
		heap2=new BGet();
		heap2->bectl(HeapExpand2,ALLOC_BLOCK_SIZE);

		syncifyInitialized=true;
	}

	void EndSyncify()
	{
#ifdef _WIN32
		VirtualFree(SyncedMem,SUPER_BLOCK_SIZE,MEM_RELEASE);
		VirtualFree(UnsyncedMem,SUPER_BLOCK_SIZE,MEM_RELEASE);
#else
		// int munmap (void *ADDR, size_t LENGTH);
		munmap(SyncedMem,   SUPER_BLOCK_SIZE);
		munmap(UnsyncedMem, SUPER_BLOCK_SIZE);
		logOutput.Print("Max synced memory usage   : %uM", commitedSynced/(1024*1024));
		logOutput.Print("Max unsynced memory usage : %uM", commitedUnsynced/(1024*1024));
#endif
		syncifyInitialized=false;
	}

	void EnterSyncedMode()
	{
		if(codeType==CodeType_Synced)
			return;

#ifdef _WIN32
		Uint32 oldProtect;
		if(codeType==CodeType_Unsynced)
			VirtualProtect(SyncedMem,commitedSynced,PAGE_READWRITE,&oldProtect);
		VirtualProtect(UnsyncedMem,commitedUnsynced,PAGE_NOACCESS,&oldProtect);
#else
		if (codeType == CodeType_Unsynced)
			mprotect(SyncedMem, commitedSynced, PROT_READ | PROT_WRITE);
		mprotect(UnsyncedMem, commitedUnsynced, PROT_NONE);
#endif

		codeType=CodeType_Synced;
	}

	void EnterUnsyncedMode()
	{
		if(codeType==CodeType_Unsynced)
			return;

#ifdef _WIN32
		Uint32 oldProtect;
		VirtualProtect(SyncedMem,commitedSynced,PAGE_READONLY,&oldProtect);
		if(codeType==CodeType_Synced)
			VirtualProtect(UnsyncedMem,commitedUnsynced,PAGE_READWRITE,&oldProtect);
#else
		mprotect(SyncedMem, commitedSynced, PROT_READ);
		if (codeType == CodeType_Synced)
			mprotect(UnsyncedMem, commitedUnsynced, PROT_READ | PROT_WRITE);
#endif

		codeType=CodeType_Unsynced;
	}

	void EnterMixedMode()
	{
		if(codeType==CodeType_Mixed)
			return;

#ifdef _WIN32
		Uint32 oldProtect;
		if(codeType==CodeType_Unsynced)
			VirtualProtect(SyncedMem,commitedSynced,PAGE_READWRITE,&oldProtect);
		if(codeType==CodeType_Synced)
			VirtualProtect(UnsyncedMem,commitedUnsynced,PAGE_READWRITE,&oldProtect);
#else
		if (codeType == CodeType_Unsynced)
			mprotect(SyncedMem, commitedSynced, PROT_READ | PROT_WRITE);
		if (codeType == CodeType_Synced)
			mprotect(UnsyncedMem, commitedUnsynced, PROT_READ | PROT_WRITE);
#endif

		codeType=CodeType_Mixed;
	}

	CodeType typeStack[1000];			//cant use stl since it might allocate mem
	int typeStackPointer=0;

	void PushCodeMode()
	{
		typeStack[typeStackPointer++]=codeType;
	}

	void PopCodeMode()
	{
		--typeStackPointer;
		switch(typeStack[typeStackPointer]){
			case CodeType_Mixed:
				EnterMixedMode();
				break;
			case CodeType_Synced:
				EnterSyncedMode();
				break;
			case CodeType_Unsynced:
				EnterUnsyncedMode();
				break;
		}
	}

	void SetCodeModeToMem(void* mem)
	{
		if(mem>SyncedMem && mem<((char*)SyncedMem)+SUPER_BLOCK_SIZE){
			EnterSyncedMode();
		} else if(mem>UnsyncedMem && mem<((char*)UnsyncedMem)+SUPER_BLOCK_SIZE){
			EnterUnsyncedMode();
		} else {
			EnterMixedMode();
		}
	}
	void AssertSyncedMode(char* file,int line)
	{
		if(codeType!=CodeType_Synced){
			char text[500];
			sprintf(text,"Code not in synced mode in %s line %i",file,line);
			handleerror(0,text,"Assertion failure",0);

			int* a=0;
			*a=0;			//make sure we crash for easy access to call stack etc
		}
	}
	void AssertUnsyncedMode(char* file,int line)
	{
		if(codeType!=CodeType_Unsynced){
			char text[500];
			sprintf(text,"Code not in unsynced mode in %s line %i",file,line);
			handleerror(0,text,"Assertion failure",0);

			int* a=0;
			*a=0;			//make sure we crash for easy access to call stack etc
		}
	}
	void AssertMixedMode(char* file,int line)
	{
		if(codeType!=CodeType_Mixed){
			char text[500];
			sprintf(text,"Code not in mixed mode in %s line %i",file,line);
			handleerror(0,text,"Assertion failure",0);

			int* a=0;
			*a=0;			//make sure we crash for easy access to call stack etc
		}
	}
}
#endif //syncify

