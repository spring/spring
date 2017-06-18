/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm> // std::min
#include <cstdint> // std::uint8_t
#include <cstring> // std::mem{cpy,set}
#include <new>

#include "LuaMemPool.h"
#include "System/SafeUtil.h"
#include "System/Log/ILog.h"
#include "System/Threading/SpringThreading.h"

// if 1, places an upper limit on pool allocation size
// needed most when free chunks are stored in an array
// rather than a hmap since the former would be highly
// sparse; it also prevents the larger (ie more rarely
// requested, so not often recycled either) allocations
// from accumulating
#define CHECK_MAX_ALLOC_SIZE 1


static std::vector<LuaMemPool*> gPools;
static std::vector<size_t> gIndcs;
static spring::mutex gMutex;

LuaMemPool* LuaMemPool::AcquirePtr()
{
	LuaMemPool* pool = nullptr;

	{
		// caller can be any thread; cf LuaParser context-data ctors
		gMutex.lock();

		if (gIndcs.empty()) {
			gPools.push_back(pool = new LuaMemPool(gPools.size()));
		} else {
			pool = gPools[gIndcs.back()];
			gIndcs.pop_back();
		}

		gMutex.unlock();
	}

	return pool;
}

void LuaMemPool::ReleasePtr(LuaMemPool* p)
{
	// only wipe statistics; blocks will be reused
	// p->LogStats("", "");
	p->ClearStats();

	gMutex.lock();
	gIndcs.push_back(p->globalIndex);
	gMutex.unlock();
}

void LuaMemPool::KillStatic()
{
	for (LuaMemPool*& p: gPools) {
		spring::SafeDelete(p);
	}

	gPools.clear();
	gIndcs.clear();
}



LuaMemPool::LuaMemPool(size_t lmpIndex): globalIndex(lmpIndex)
{
	freeChunksTable.reserve(16384);
	chunkCountTable.reserve(16384);

	#if 1
	allocBlocks.reserve(1024);
	#endif
}

LuaMemPool::~LuaMemPool()
{
	DeleteBlocks();
}


void LuaMemPool::LogStats(const char* handle, const char* lctype) const
{
	LOG(
		"[LuaMemPool::%s][handle=%s (%s)] idx=%lu {Int,Ext,Rec}Allocs={%lu,%lu,%lu}",
		__func__,
		handle,
		lctype,
		(unsigned long) globalIndex,
		(unsigned long) allocStats[ALLOC_INT],
		(unsigned long) allocStats[ALLOC_EXT],
		(unsigned long) allocStats[ALLOC_REC]
	);
}


void LuaMemPool::DeleteBlocks()
{
	#if 1
	for (void* p: allocBlocks) {
		::operator delete(p);
	}
	#endif

	ClearStats();
}

void* LuaMemPool::Alloc(size_t size)
{
	#ifdef UNITSYNC
	return ::operator new(size);
	#endif

	#if (CHECK_MAX_ALLOC_SIZE == 1)
	if (!AllocFromPool(size)) {
		allocStats[ALLOC_EXT] += 1;
		return ::operator new(size);
	}
	#endif

	size = std::max(size, size_t(MIN_ALLOC_SIZE));
	allocStats[ALLOC_INT] += 1;

	auto freeChunksTablePair = std::make_pair(freeChunksTable.find(size), false);

	if (freeChunksTablePair.first == freeChunksTable.end())
		freeChunksTablePair = freeChunksTable.insert(size, nullptr);

	void* ptr = (freeChunksTablePair.first)->second;

	if (ptr != nullptr) {
		(freeChunksTablePair.first)->second = (*(void**) ptr);
		allocStats[ALLOC_REC] += 1;
		return ptr;
	}

	auto chunkCountTablePair = std::make_pair(chunkCountTable.find(size), false);

	if (chunkCountTablePair.first == chunkCountTable.end())
		chunkCountTablePair = chunkCountTable.insert(size, 16);

	const size_t numChunks = (chunkCountTablePair.first)->second;
	const size_t numBytes = size * numChunks;

	void* newBlock = ::operator new(numBytes);
	uint8_t* newBytes = reinterpret_cast<uint8_t*>(newBlock);

	#if 1
	allocBlocks.push_back(newBlock);
	#endif

	// new allocation; construct chain of chunks within the memory block
	// (this requires the block size to be at least MIN_ALLOC_SIZE bytes)
	for (size_t i = 0; i < (numChunks - 1); ++i) {
		*(void**) &newBytes[i * size] = (void*) &newBytes[(i + 1) * size];
	}

	*(void**) &newBytes[(numChunks - 1) * size] = nullptr;

	freeChunksTable[size] = (*(void**) newBlock);
	chunkCountTable[size] *= 2; // geometric increase
	return newBlock;
}

void* LuaMemPool::Realloc(void* ptr, size_t nsize, size_t osize)
{
	void* ret = Alloc(nsize);

	if (ptr == nullptr)
		return ret;

	std::memcpy(ret, ptr, std::min(nsize, osize));
	std::memset(ptr, 0, osize);

	Free(ptr, osize);
	return ret;
}

void LuaMemPool::Free(void* ptr, size_t size)
{
	#ifdef UNITSYNC
	::operator delete(ptr);
	return;
	#endif

	if (ptr == nullptr)
		return;

	#if (CHECK_MAX_ALLOC_SIZE == 1)
	if (!AllocFromPool(size)) {
		::operator delete(ptr);
		return;
	}
	#endif

	size = std::max(size, size_t(MIN_ALLOC_SIZE));

	*(void**) ptr = freeChunksTable[size];
	freeChunksTable[size] = ptr;
}

