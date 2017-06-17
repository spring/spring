/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm> // std::min
#include <cstdint> // std::uint8_t
#include <cstring> // std::mem{cpy,set}
#include <new>

#include "LuaMemPool.h"

// if 1, places an upper limit on pool allocation size
// needed most when free chunks are stored in an array
// rather than a hmap since the former would be highly
// sparse; it also prevents the larger (ie more rarely
// requested, so not often recycled either) allocations
// from accumulating
#define CHECK_MAX_ALLOC_SIZE 1

LuaMemPool::LuaMemPool()
{
	nextFreeChunk.reserve(16384);
	poolNumChunks.reserve(16384);

	#if 1
	allocBlocks.reserve(1024);
	#endif
}

LuaMemPool::~LuaMemPool()
{
	#if 1
	for (void* p: allocBlocks) {
		::operator delete(p);
	}
	#endif
}


void* LuaMemPool::Alloc(size_t size)
{
	#ifdef UNITSYNC
	return ::operator new(size);
	#endif

	#if (CHECK_MAX_ALLOC_SIZE == 1)
	if (!AllocFromPool(size = std::max(size, size_t(MIN_ALLOC_SIZE))))
		return ::operator new(size);
	#else
	size = std::max(size, size_t(MIN_ALLOC_SIZE));
	#endif

	auto nextFreeChunkPair = std::make_pair(nextFreeChunk.find(size), false);

	if (nextFreeChunkPair.first == nextFreeChunk.end())
		nextFreeChunkPair = nextFreeChunk.insert(size, nullptr);

	void* ptr = (nextFreeChunkPair.first)->second;

	if (ptr != nullptr) {
		(nextFreeChunkPair.first)->second = (*(void**) ptr);
		return ptr;
	}

	auto poolNumChunksPair = std::make_pair(poolNumChunks.find(size), false);

	if (poolNumChunksPair.first == poolNumChunks.end())
		poolNumChunksPair = poolNumChunks.insert(size, 16);

	const size_t numChunks = (poolNumChunksPair.first)->second;
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

	ptr = newBlock;

	nextFreeChunk[size] = (*(void**) ptr);
	poolNumChunks[size] *= 2; // geometric increase
	return ptr;
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
	if (!AllocFromPool(size = std::max(size, size_t(MIN_ALLOC_SIZE)))) {
		::operator delete(ptr);
		return;
	}
	#else
	size = std::max(size, size_t(MIN_ALLOC_SIZE));
	#endif

	*(void**) ptr = nextFreeChunk[size];
	nextFreeChunk[size] = ptr;
}

