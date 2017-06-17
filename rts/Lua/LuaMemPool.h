/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_MEM_POOL_H_
#define LUA_MEM_POOL_H_

#include <cstddef>
#include <vector>

#include "System/UnorderedMap.hpp"

class LuaMemPool {
public:
	LuaMemPool();
	~LuaMemPool();

	void* Alloc(size_t size);
	void* Realloc(void* ptr, size_t nsize, size_t osize);
	void Free(void* ptr, size_t size);

private:
	static constexpr size_t MIN_ALLOC_SIZE = sizeof(void*);
	static constexpr size_t MAX_ALLOC_SIZE = (1024 * 1024) - 1;

	static bool CanAlloc(size_t size) { return (size <= MAX_ALLOC_SIZE); }

private:
	spring::unsynced_map<size_t, void*> nextFreeChunk;
	spring::unsynced_map<size_t, size_t> poolNumChunks;

	#if 1
	std::vector<void*> allocBlocks;
	#endif
};

#endif

