/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _MEM_POOL_H_
#define _MEM_POOL_H_

#include <new>
#include <cstring> // for size_t
#include <vector>

static const size_t MAX_MEM_SIZE = 200;

/**
 * Speeds-up for memory-allocation of often allocated/deallocated structs
 * or classes, or other memory blocks of equal size.
 * You may think of this as something like a very primitive garbage collector.
 * Instead of actually freeing memory, it is kept allocated, and is just
 * reassigned next time an alloc of the same size is performed.
 * The maximum memory held by this class is approximately MAX_MEM_SIZE^2 bytes.
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
	std::vector<void *> allocated;
};

extern CMemPool mempool;

#endif // _MEM_POOL_H_

