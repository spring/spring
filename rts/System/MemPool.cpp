/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "MemPool.h"
#include <assert.h>
////#include "mmgr.h"

CMemPool mempool;

CMemPool::CMemPool()
{
	for(size_t a=0; a < MAX_MEM_SIZE+1; a++){
		nextFree[a]=0;
		poolSize[a]=10;
	}
}

void* CMemPool::Alloc(size_t n)
{
	if(n > MAX_MEM_SIZE || n<4) {
#ifdef USE_MMGR
		return (void*)new char[n];
#else
		return ::operator new(n);
#endif
	}
	void* p=nextFree[n];

	if (p) {
		nextFree[n]=(*(void**)p);
	} else {
		#ifdef USE_MMGR
		void* newBlock= (void*)new char[n*poolSize[n]];
		#else
		void* newBlock= ::operator new(n*poolSize[n]);
		#endif
		for (int i=0; i < poolSize[n]-1; ++i) {
			*(void**)&((char*)newBlock)[(i)*n] = (void*)&((char*)newBlock)[(i+1)*n];
		}

		*(void**)&((char*)newBlock)[(poolSize[n]-1)*n] = 0;

		p = newBlock;
		nextFree[n] = (*(void**)p);
		poolSize[n] *= 2;
	}
	return p;
}

void CMemPool::Free(void* p,size_t n)
{
	if(p==0) return;

	if(n>MAX_MEM_SIZE || n<4) {
		#ifdef USE_MMGR
		delete[] (char*)p;
		#else
		::operator delete(p);
		#endif
		return;
	}
	*(void**)p = nextFree[n];
	nextFree[n] = p;
}

CMemPool::~CMemPool()
{
}
