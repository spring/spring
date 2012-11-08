/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/MemPool.h"
//#include "System/mmgr.h"

CMemPool mempool;

CMemPool::CMemPool()
{
	for (size_t a = 0; a < (MAX_MEM_SIZE + 1); a++) {
		nextFree[a] = NULL;
		poolSize[a] = 10;
	}
}

void* CMemPool::Alloc(size_t numBytes)
{
	if (UseExternalMemory(numBytes)) {
		return ::operator new(numBytes);
	} else {
		void* pnt = nextFree[numBytes];

		if (pnt) {
			nextFree[numBytes] = (*(void**)pnt);
		} else {
			void* newBlock = ::operator new(numBytes * poolSize[numBytes]);
			for (int i = 0; i < (poolSize[numBytes] - 1); ++i) {
				*(void**)&((char*)newBlock)[(i) * numBytes] = (void*)&((char*)newBlock)[(i + 1) * numBytes];
			}

			*(void**)&((char*)newBlock)[(poolSize[numBytes] - 1) * numBytes] = 0;

			pnt = newBlock;
			nextFree[numBytes] = (*(void**)pnt);
			poolSize[numBytes] *= 2;
		}
		return pnt;
	}
}

void CMemPool::Free(void* pnt, size_t numBytes)
{
	if (pnt == NULL) {
		return;
	}

	if (UseExternalMemory(numBytes)) {
		::operator delete(pnt);
	} else {
		*(void**)pnt = nextFree[numBytes];
		nextFree[numBytes] = pnt;
	}
}

CMemPool::~CMemPool()
{
}
