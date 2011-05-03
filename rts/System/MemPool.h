/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _MEM_POOL_H_
#define _MEM_POOL_H_

#include <new>
#include <cstring> // for size_t

const size_t MAX_MEM_SIZE=200;

class CMemPool
{
public:
	CMemPool();
	void *Alloc(size_t n);
	void Free(void *p,size_t n);
	~CMemPool();

private:
	void* nextFree[MAX_MEM_SIZE+1];
	int poolSize[MAX_MEM_SIZE+1];
};

extern CMemPool mempool;

#endif // _MEM_POOL_H_

