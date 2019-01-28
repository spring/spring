/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_MEM_POOL_H_
#define LUA_MEM_POOL_H_

#include <cstddef>
#include <vector>

#include "System/bitops.h"
#include "System/MemPoolTypes.h"
#include "System/UnorderedMap.hpp"

#define LMP_USE_CHUNK_TABLE 0

class CLuaHandle;
class LuaMemPool {
public:
	explicit LuaMemPool(bool isEnabled);
	explicit LuaMemPool(size_t lmpIndex);

	~LuaMemPool() {
		Clear();

		if (!LuaMemPool::enabled)
			return;

		poolImpl.Kill();
	}

	LuaMemPool(const LuaMemPool& p) = delete;
	LuaMemPool(LuaMemPool&& p) = delete;

	LuaMemPool& operator = (const LuaMemPool& p) = delete;
	LuaMemPool& operator = (LuaMemPool&& p) = delete;

public:
	static size_t GetPoolCount();

	static LuaMemPool* GetSharedPtr();
	static LuaMemPool* AcquirePtr(bool shared, bool owned);
	static void ReleasePtr(LuaMemPool* p, const CLuaHandle* o);

	static void FreeShared();
	static void InitStatic(bool enable);
	static void KillStatic();

public:
	void Clear() {
		DeleteBlocks();
		ClearStats(true);
		ClearTables();
	}

	void Reserve(size_t size) {
		#if (LMP_USE_CHUNK_TABLE == 1)
		freeChunksTable.reserve(size);
		chunkCountTable.reserve(size);

		#if 1
		allocBlocks.reserve(size / 16);
		#endif
		#endif
	}

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

		#if (LMP_USE_CHUNK_TABLE == 0)
		poolImpl.numAllocs.fill(0);
		poolImpl.allocSums.fill(0);
		#endif
	}

	void ClearTables() {
		#if (LMP_USE_CHUNK_TABLE == 1)
		freeChunksTable.clear();
		chunkCountTable.clear();
		#endif
	}

	size_t  GetGlobalIndex() const { return globalIndex; }
	size_t  GetSharedCount() const { return sharedCount; }
	size_t& GetSharedCount()       { return sharedCount; }

public:
	static constexpr size_t MIN_ALLOC_SIZE = sizeof(void*);
	static constexpr size_t MAX_ALLOC_SIZE = 1 << 26;
	// static constexpr size_t MAX_ALLOC_SIZE = (1024 * 1024) - 1;

	static bool enabled;

private:
	#if (LMP_USE_CHUNK_TABLE == 1)
	spring::unsynced_map<size_t, void*> freeChunksTable;
	spring::unsynced_map<size_t, size_t> chunkCountTable;

	std::vector<void*> allocBlocks;
	#endif


	#if (LMP_USE_CHUNK_TABLE == 0)
	struct PoolImpl {
	public:
		static constexpr uint32_t NUM_POOLS = 32;

		// all N's intentionally over-dimensioned by a factor 1<<10
		static constexpr std::array<uint32_t, NUM_POOLS> NUM_CHUNKS = {
			1 << 12,  1 << 12,  1 << 12,  1 << 12,  1 << 12,  1 << 12,  1 << 12,  1 << 12,
			1 << 13,  1 << 13,  1 << 13,  1 << 13,  1 << 13,  1 << 13,  1 << 13,  1 << 13,
			1 << 14,  1 << 14,  1 << 14,  1 << 14,  1 << 14,  1 << 14,  1 << 14,  1 << 14,
			1 << 15,  1 << 15,  1 << 15,  1 << 15,  1 << 15,  1 << 15,  1 << 15,  1 << 15,
		};
		static constexpr std::array<uint32_t, NUM_POOLS> NUM_PAGES = {
			1 << 15,  1 << 15,  1 << 14,  1 << 14,  1 << 13,  1 << 13,  1 << 12,  1 << 12,
			1 << 11,  1 << 11,  1 << 10,  1 << 10,  1 <<  9,  1 <<  9,  1 <<  8,  1 <<  8,
			1 <<  7,  1 <<  7,  1 <<  6,  1 <<  6,  1 <<  5,  1 <<  5,  1 <<  4,  1 <<  4,
			1 <<  3,  1 <<  3,  1 <<  2,  1 <<  2,  1 <<  1,  1 <<  1,  1 <<  0,  1 <<  0,
		};

		std::array<uint8_t[sizeof(FixedDynMemPool<0, NUM_CHUNKS[NUM_POOLS - 1], 0>)], NUM_POOLS> memPools;
		std::array<void*, NUM_POOLS> poolPtrs;

		std::array<size_t, NUM_POOLS + 1> numAllocs;
		std::array<size_t, NUM_POOLS + 1> allocSums;

	public:
		static uint32_t CalcPoolIndex(uint32_t alloc) {
			// skip first few pools due to page-overhead
			return (std::max(2u + (MIN_ALLOC_SIZE == 8), log_base_2(alloc)));
		}


		template<size_t i, typename PoolType = FixedDynMemPool<1 << i, NUM_CHUNKS[i], NUM_PAGES[i]>>
		PoolType* NewPool() {
			static_assert(sizeof(memPools[i]) >= sizeof(PoolType), "");
			return (new (memPools[i]) PoolType());
		}

		template<size_t i, typename PoolType = FixedDynMemPool<1 << i, NUM_CHUNKS[i], NUM_PAGES[i]>>
		PoolType* GetPool() {
			return (static_cast<PoolType*>(poolPtrs[i]));
		}

		template<size_t i, typename PoolType = FixedDynMemPool<1 << i, NUM_CHUNKS[i], NUM_PAGES[i]>>
		void KillPool() {
			GetPool<i>()->~PoolType();
		}


		void Init();
		void Kill();

		void* Alloc(uint32_t size);
		void Free(void* ptr, uint32_t size);
	};

	PoolImpl poolImpl;
	#endif


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

