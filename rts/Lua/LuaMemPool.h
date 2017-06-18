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

	void LogStats(const char* handle, const char* lctype) const;

private:
	static constexpr size_t MIN_ALLOC_SIZE = sizeof(void*);
	static constexpr size_t MAX_ALLOC_SIZE = (1024 * 1024) - 1;

	// Lua code tends to perform many smaller *short-lived* allocations
	// this frees us from having to handle all possible sizes, just the
	// most common
	static bool AllocFromPool(size_t size) { return (size <= MAX_ALLOC_SIZE); }

private:
	spring::unsynced_map<size_t, void*> nextFreeChunk;
	spring::unsynced_map<size_t, size_t> poolNumChunks;

	#if 1
	std::vector<void*> allocBlocks;
	#endif

	enum {
		ALLOC_INT = 0, // internal
		ALLOC_EXT = 1, // external
		ALLOC_REC = 2, // recycled
	};

	size_t allocStats[3] = {0, 0, 0};
};

#endif

