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


// global, affects all pool instances
bool LuaMemPool::enabled = false;

static LuaMemPool gSharedPool(-1);

static std::vector<LuaMemPool*> gPools;
static std::vector<size_t> gIndcs;
static spring::mutex gMutex;


// Lua code tends to perform many smaller *short-lived* allocations
// this frees us from having to handle all possible sizes, just the
// most common
static bool AllocInternal(size_t size) { return ((size * CHECK_MAX_ALLOC_SIZE) <= LuaMemPool::MAX_ALLOC_SIZE); }
static bool AllocExternal(size_t size) { return (!LuaMemPool::enabled || !AllocInternal(size)); }


LuaMemPool* LuaMemPool::AcquirePtr(bool shared)
{
	LuaMemPool* p = GetSharedPoolPtr();

	if (!shared) {
		// caller can be any thread; cf LuaParser context-data ctors
		// (the shared pool must *not* be used by different threads)
		gMutex.lock();

		if (gIndcs.empty()) {
			gPools.push_back(p = new LuaMemPool(gPools.size()));
		} else {
			p = gPools[gIndcs.back()];
			gIndcs.pop_back();
		}

		gMutex.unlock();
	}

	// only wipe statistics; blocks will be recycled
	p->ClearStats((p->GetSharedCount() += shared) <= 1);
	return p;
}

void LuaMemPool::ReleasePtr(LuaMemPool* p)
{
	if (p == GetSharedPoolPtr()) {
		p->GetSharedCount() -= 1;
		return;
	}

	gMutex.lock();
	gIndcs.push_back(p->GetGlobalIndex());
	gMutex.unlock();
}

void LuaMemPool::InitStatic(bool enable) { LuaMemPool::enabled = enable; }
void LuaMemPool::KillStatic()
{
	for (LuaMemPool*& p: gPools) {
		spring::SafeDelete(p);
	}

	gPools.clear();
	gIndcs.clear();
}

LuaMemPool* LuaMemPool::GetSharedPoolPtr() { return &gSharedPool; }



LuaMemPool::LuaMemPool(size_t lmpIndex): globalIndex(lmpIndex)
{
	if (!LuaMemPool::enabled)
		return;

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
		"[LuaMemPool::%s][handle=%s (%s)] index=%lu {blocks,sizes}={%lu,%lu} {int,ext,rec}Allocs={%lu,%lu,%lu} {chunk,block}Bytes={%lu,%lu}",
		__func__,
		handle,
		lctype,
		(unsigned long) globalIndex,
		(unsigned long) allocBlocks.size(),
		(unsigned long) chunkCountTable.size(),
		(unsigned long) allocStats[STAT_NIA],
		(unsigned long) allocStats[STAT_NEA],
		(unsigned long) allocStats[STAT_NRA],
		(unsigned long) allocStats[STAT_NCB],
		(unsigned long) allocStats[STAT_NBB]
	);
}


void LuaMemPool::DeleteBlocks()
{
	#if 1
	for (void* p: allocBlocks) {
		::operator delete(p);
	}
	#endif

	ClearStats(true);
}

void* LuaMemPool::Alloc(size_t size)
{
	if (AllocExternal(size)) {
		allocStats[STAT_NEA] += 1;
		return ::operator new(size);
	}

	allocStats[STAT_NIA] += 1;
	allocStats[STAT_NCB] += (size = std::max(size, size_t(MIN_ALLOC_SIZE)));

	auto freeChunksTablePair = std::make_pair(freeChunksTable.find(size), false);

	if (freeChunksTablePair.first == freeChunksTable.end())
		freeChunksTablePair = freeChunksTable.insert(size, nullptr);

	void* ptr = (freeChunksTablePair.first)->second;

	if (ptr != nullptr) {
		(freeChunksTablePair.first)->second = (*(void**) ptr);

		allocStats[STAT_NRA] += 1;
		return ptr;
	}

	auto chunkCountTablePair = std::make_pair(chunkCountTable.find(size), false);

	if (chunkCountTablePair.first == chunkCountTable.end())
		chunkCountTablePair = chunkCountTable.insert(size, 8);

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

	allocStats[STAT_NBB] += numBytes;
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
	if (ptr == nullptr)
		return;

	if (AllocExternal(size)) {
		::operator delete(ptr);
		return;
	}

	allocStats[STAT_NCB] -= (size = std::max(size, size_t(MIN_ALLOC_SIZE)));

	*(void**) ptr = freeChunksTable[size];
	freeChunksTable[size] = ptr;
}

