/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_MEM_POOL_H_
#define LUA_MEM_POOL_H_

#include <cstddef>
#include <vector>

#include "System/UnorderedMap.hpp"

class LuaMemPool {
public:
	LuaMemPool(size_t lmpIndex);
	~LuaMemPool();

	LuaMemPool(const LuaMemPool& p) = delete;
	LuaMemPool(LuaMemPool&& p) = delete;

	LuaMemPool& operator = (const LuaMemPool& p) = delete;
	LuaMemPool& operator = (LuaMemPool&& p) = delete;

public:
	static LuaMemPool* AcquirePtr(bool shared);
	static void ReleasePtr(LuaMemPool* p);

	static void InitStatic(bool enable);
	static void KillStatic();

	static const LuaMemPool* GetSharedPoolConstPtr() { return (GetSharedPoolPtr()); }
	static       LuaMemPool* GetSharedPoolPtr();

public:
	void DeleteBlocks();
	void* Alloc(size_t size);
	void* Realloc(void* ptr, size_t nsize, size_t osize);
	void Free(void* ptr, size_t size);

	void LogStats(const char* handle, const char* lctype) const;
	void ClearStats(bool b) {
		allocStats[STAT_NIA] *= (1 - b);
		allocStats[STAT_NEA] *= (1 - b);
		allocStats[STAT_NRA] *= (1 - b);
		allocStats[STAT_NCB] *= (1 - b);
		allocStats[STAT_NBB] *= (1 - b);
	}

	size_t  GetGlobalIndex() const { return globalIndex; }
	size_t  GetSharedCount() const { return sharedCount; }
	size_t& GetSharedCount()       { return sharedCount; }

public:
	static constexpr size_t MIN_ALLOC_SIZE = sizeof(void*);
	static constexpr size_t MAX_ALLOC_SIZE = (1024 * 1024) - 1;

	static bool enabled;

private:
	spring::unsynced_map<size_t, void*> freeChunksTable;
	spring::unsynced_map<size_t, size_t> chunkCountTable;

	std::vector<void*> allocBlocks;

	enum {
		STAT_NIA = 0, // number of internal allocs
		STAT_NEA = 1, // number of external allocs
		STAT_NRA = 2, // number of recycled allocs
		STAT_NCB = 3, // number of chunk bytes currently in use
		STAT_NBB = 4, // number of block bytes alloced in total
	};

	size_t allocStats[5] = {0, 0, 0, 0, 0};
	size_t globalIndex = 0;
	size_t sharedCount = 0;
};

#endif

