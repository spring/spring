/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _MEM_POOL_H_
#define _MEM_POOL_H_

#include <new>
#include <cstring> // for size_t

static const size_t MAX_MEM_SIZE = 200;

/**
 * Allows Implementing some kind of garbage collector.
 * The implementation of this garbage collector was never even started, only
 * this basic framework was implemented, which by itself, serves no purpose.
 */
class CMemPool
{
public:
	CMemPool();
	~CMemPool();

	void* Alloc(size_t numBytes);
	void Free(void* pnt, size_t numBytes);

private:
	static bool UseExternalMemory(size_t numBytes) {
		return (numBytes > MAX_MEM_SIZE) || (numBytes < 4);
	}

	void* nextFree[MAX_MEM_SIZE + 1];
	int poolSize[MAX_MEM_SIZE + 1];
};

extern CMemPool mempool;

#endif // _MEM_POOL_H_

