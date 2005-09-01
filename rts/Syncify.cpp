#include "Syncify.h"

#ifdef SYNCIFY
#include <windows.h>
#include <stdio.h>
#include "bget.h"

namespace Syncify{
	void* Alloc(size_t reportedSize);
	void Free(void* reportedAddress);
}
void	*operator new(size_t reportedSize)
{
	return Syncify::Alloc(reportedSize);
}

void	*operator new[](size_t reportedSize)
{
	return Syncify::Alloc(reportedSize);
}

void	operator delete(void *reportedAddress)
{
	Syncify::Free(reportedAddress);
}

void	operator delete[](void *reportedAddress)
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

	unsigned int commitedSynced=0;		//increase these as more mem is commited (must commit lineary)
	unsigned int commitedUnsynced=0;

	BGet* heap1;
	BGet* heap2;

	void* Alloc(size_t reportedSize)
	{
		if(!syncifyInitialized)
			return ::malloc(reportedSize);

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
		VirtualAlloc((char*)SyncedMem+commitedSynced,ALLOC_BLOCK_SIZE,MEM_COMMIT,PAGE_READWRITE);		//only reserves mem, will need to commit mem as needed
		commitedSynced+=ALLOC_BLOCK_SIZE;
		return (char*)SyncedMem+commitedSynced-ALLOC_BLOCK_SIZE;
	}
	void* HeapExpand2(bufsize size)
	{
		VirtualAlloc((char*)UnsyncedMem+commitedUnsynced,ALLOC_BLOCK_SIZE,MEM_COMMIT,PAGE_READWRITE);		//only reserves mem, will need to commit mem as needed
		commitedUnsynced+=ALLOC_BLOCK_SIZE;
		return (char*)UnsyncedMem+commitedUnsynced-ALLOC_BLOCK_SIZE;
	}

	void InitSyncify()
	{
		if(syncifyInitialized)
			return;
		codeType=CodeType_Mixed;

		SyncedMem=VirtualAlloc(0,SUPER_BLOCK_SIZE,MEM_RESERVE,PAGE_READWRITE);		//only reserves mem, will need to commit mem as needed
		UnsyncedMem=VirtualAlloc(0,SUPER_BLOCK_SIZE,MEM_RESERVE,PAGE_READWRITE);

		heap1=new BGet();
		heap1->bectl(HeapExpand1,ALLOC_BLOCK_SIZE);
		heap2=new BGet();
		heap2->bectl(HeapExpand2,ALLOC_BLOCK_SIZE);

		syncifyInitialized=true;
	}

	void EndSyncify()
	{
		VirtualFree(SyncedMem,SUPER_BLOCK_SIZE,MEM_RELEASE);
		VirtualFree(UnsyncedMem,SUPER_BLOCK_SIZE,MEM_RELEASE);
		syncifyInitialized=false;
	}

	void EnterSyncedMode()
	{
		if(codeType==CodeType_Synced)
			return;

		Uint32 oldProtect;
		if(codeType==CodeType_Unsynced)
			VirtualProtect(SyncedMem,commitedSynced,PAGE_READWRITE,&oldProtect);
		VirtualProtect(UnsyncedMem,commitedUnsynced,PAGE_NOACCESS,&oldProtect);

		codeType=CodeType_Synced;
	}

	void EnterUnsyncedMode()
	{
		if(codeType==CodeType_Unsynced)
			return;


		Uint32 oldProtect;
		VirtualProtect(SyncedMem,commitedSynced,PAGE_READONLY,&oldProtect);
		if(codeType==CodeType_Synced)
			VirtualProtect(UnsyncedMem,commitedUnsynced,PAGE_READWRITE,&oldProtect);

		codeType=CodeType_Unsynced;
	}

	void EnterMixedMode()
	{
		if(codeType==CodeType_Mixed)
			return;


		Uint32 oldProtect;
		if(codeType==CodeType_Unsynced)
			VirtualProtect(SyncedMem,commitedSynced,PAGE_READWRITE,&oldProtect);
		if(codeType==CodeType_Synced)
			VirtualProtect(UnsyncedMem,commitedUnsynced,PAGE_READWRITE,&oldProtect);

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
			MessageBox(0,text,"Assertion failure",0);

			int* a=0;
			*a=0;			//make sure we crash for easy access to call stack etc
		}
	}
	void AssertUnsyncedMode(char* file,int line)
	{
		if(codeType!=CodeType_Unsynced){
			char text[500];
			sprintf(text,"Code not in unsynced mode in %s line %i",file,line);
			MessageBox(0,text,"Assertion failure",0);

			int* a=0;
			*a=0;			//make sure we crash for easy access to call stack etc
		}
	}
	void AssertMixedMode(char* file,int line)
	{
		if(codeType!=CodeType_Mixed){
			char text[500];
			sprintf(text,"Code not in mixed mode in %s line %i",file,line);
			MessageBox(0,text,"Assertion failure",0);

			int* a=0;
			*a=0;			//make sure we crash for easy access to call stack etc
		}
	}
}
#endif //syncify

