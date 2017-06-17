/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm> // std::min
#include <cstdint> // std::uint8_t
#include <cstring> // std::mem{cpy,set}
#include <new>

#include "LuaMemPool.h"

LuaMemPool::LuaMemPool()
{
	// note: indices [0, MIN_ALLOC_SIZE - 1] are unused	
	for (size_t i = 0; i < (MAX_ALLOC_SIZE + 1); i++) {
		nextFreeChunk[i] = nullptr;
		poolNumChunks[i] = 16;
	}

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
	if (!CanAlloc(size = std::max(size, size_t(MIN_ALLOC_SIZE))))
		return ::operator new(size);

	void* ptr = nextFreeChunk[size];

	if (ptr != nullptr) {
		nextFreeChunk[size] = (*(void**) ptr);
		return ptr;
	}

	const size_t numBytes = size * poolNumChunks[size];
	const size_t numChunks = poolNumChunks[size] - 1;

	void* newBlock = ::operator new(numBytes);
	uint8_t* newBytes = reinterpret_cast<uint8_t*>(newBlock);

	#if 1
	allocBlocks.push_back(newBlock);
	#endif

	// new allocation; construct chain of chunks within the memory block
	// (this requires the block size to be at least MIN_ALLOC_SIZE bytes)
	for (size_t i = 0; i < numChunks; ++i) {
		*(void**) &newBytes[i * size] = (void*) &newBytes[(i + 1) * size];
	}

	*(void**) &newBytes[numChunks * size] = nullptr;

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
	if (ptr == nullptr)
		return;

	if (!CanAlloc(size = std::max(size, size_t(MIN_ALLOC_SIZE)))) {
		::operator delete(ptr);
		return;
	}

	*(void**) ptr = nextFreeChunk[size];
	nextFreeChunk[size] = ptr;
}

