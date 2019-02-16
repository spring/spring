/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm> // std::min
#include <cstdint> // std::uint8_t
#include <cstring> // std::mem{cpy,set}
#include <new>

#include "LuaMemPool.h"
#include "System/MainDefines.h"
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

static LuaMemPool* gSharedPool = nullptr;

static std::array<uint8_t, sizeof(LuaMemPool)> gSharedPoolMem;
static std::vector<LuaMemPool*> gPools;
static std::vector<size_t> gIndcs;
static std::atomic<size_t> gCount = {0};
static spring::mutex gMutex;


// Lua code tends to perform many smaller *short-lived* allocations
// this frees us from having to handle all possible sizes, just the
// most common
static bool AllocInternal(size_t size) { return ((size * CHECK_MAX_ALLOC_SIZE) <= LuaMemPool::MAX_ALLOC_SIZE); }
static bool AllocExternal(size_t size) { return (!LuaMemPool::enabled || !AllocInternal(size)); }

size_t LuaMemPool::GetPoolCount() { return (gCount.load()); }

LuaMemPool* LuaMemPool::GetSharedPtr() { return gSharedPool; }
LuaMemPool* LuaMemPool::AcquirePtr(bool shared, bool owned)
{
	LuaMemPool* p = GetSharedPtr();

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
	// p->ClearStats((p->GetSharedCount() += shared) <= 1);

	// wipe statistics and blocks if we are the first to request p
	if ((p->GetSharedCount() += shared) <= 1) {
		p->Clear();
		p->Reserve(16384);
	}

	// track the number of active state-owned pools (for /debug)
	gCount += owned;
	return p;
}

void LuaMemPool::ReleasePtr(LuaMemPool* p, const CLuaHandle* o)
{
	gCount -= (o != nullptr);

	if (p == GetSharedPtr()) {
		p->GetSharedCount() -= 1;
		return;
	}

	gMutex.lock();
	gIndcs.push_back(p->GetGlobalIndex());
	gMutex.unlock();
}

void LuaMemPool::FreeShared() { gSharedPool->Clear(); }
void LuaMemPool::InitStatic(bool enable) { gSharedPool = new (gSharedPoolMem.data()) LuaMemPool(LuaMemPool::enabled = enable); }
void LuaMemPool::KillStatic()
{
	for (LuaMemPool*& p: gPools) {
		spring::SafeDelete(p);
	}

	gPools.clear();
	gIndcs.clear();

	spring::SafeDestruct(gSharedPool);
}



LuaMemPool::LuaMemPool(bool isEnabled): LuaMemPool(size_t(-1)) { assert(isEnabled == LuaMemPool::enabled); }
LuaMemPool::LuaMemPool(size_t lmpIndex): globalIndex(lmpIndex)
{
	if (!LuaMemPool::enabled)
		return;

	poolImpl.Init();
	Reserve(16384);
}


void LuaMemPool::LogStats(const char* handle, const char* lctype) const
{
	#if (LMP_USE_CHUNK_TABLE == 1)
		LOG(
			"[LuaMemPool::%s][handle=%s (%s)] index=" _STPF_ " {blocks,sizes}={" _STPF_ "," _STPF_ "} {int,ext,rec}Allocs={" _STPF_ "," _STPF_ "," _STPF_ "} {chunk,block}Bytes={" _STPF_ "," _STPF_ "}",
			__func__,
			handle,
			lctype,
			globalIndex,
			allocBlocks.size(),
			chunkCountTable.size(),
			allocStats[STAT_NIA],
			allocStats[STAT_NEA],
			allocStats[STAT_NRA],
			allocStats[STAT_NCB],
			allocStats[STAT_NBB]
		);
	#else
		LOG(
			"[LuaMemPool::%s][handle=%s (%s)] index=" _STPF_ " {numAllocs[*],allocSums[*]}={" _STPF_ "," _STPF_ "} {int,ext,rec}Allocs={" _STPF_ "," _STPF_ "," _STPF_ "} {chunk,block}Bytes={" _STPF_ "," _STPF_ "}",
			__func__,
			handle,
			lctype,
			globalIndex,
			poolImpl.numAllocs[PoolImpl::NUM_POOLS],
			poolImpl.allocSums[PoolImpl::NUM_POOLS],
			allocStats[STAT_NIA],
			allocStats[STAT_NEA],
			allocStats[STAT_NRA],
			allocStats[STAT_NCB],
			allocStats[STAT_NBB]
		);
	#endif
}


void LuaMemPool::DeleteBlocks()
{
	#if (LMP_USE_CHUNK_TABLE == 1)
	#if 1
	for (void* p: allocBlocks) {
		::operator delete(p);
	}

	allocBlocks.clear();
	#endif
	#endif
}

void* LuaMemPool::Alloc(size_t size)
{
	if (AllocExternal(size)) {
		allocStats[STAT_NEA] += 1;
		return ::operator new(size);
	}

	allocStats[STAT_NIA] += 1;
	allocStats[STAT_NCB] += (size = std::max(size, size_t(MIN_ALLOC_SIZE)));

	#if (LMP_USE_CHUNK_TABLE == 1)
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
	#else
	return (poolImpl.Alloc(size));
	#endif
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

	#if (LMP_USE_CHUNK_TABLE == 1)
	*(void**) ptr = freeChunksTable[size];
	freeChunksTable[size] = ptr;
	#else
	poolImpl.Free(ptr, size);
	#endif
}




void LuaMemPool::PoolImpl::Init() {
	poolPtrs.fill(nullptr);
	numAllocs.fill(0);
	allocSums.fill(0);

	poolPtrs[ 0] = NewPool< 0>();
	poolPtrs[ 1] = NewPool< 1>();
	poolPtrs[ 2] = NewPool< 2>();
	poolPtrs[ 3] = NewPool< 3>();
	poolPtrs[ 4] = NewPool< 4>();
	poolPtrs[ 5] = NewPool< 5>();
	poolPtrs[ 6] = NewPool< 6>();
	poolPtrs[ 7] = NewPool< 7>();
	poolPtrs[ 8] = NewPool< 8>();
	poolPtrs[ 9] = NewPool< 9>();
	poolPtrs[10] = NewPool<10>();
	poolPtrs[11] = NewPool<11>();
	poolPtrs[12] = NewPool<12>();
	poolPtrs[13] = NewPool<13>();
	poolPtrs[14] = NewPool<14>();
	poolPtrs[15] = NewPool<15>();
	poolPtrs[16] = NewPool<16>();
	poolPtrs[17] = NewPool<17>();
	poolPtrs[18] = NewPool<18>();
	poolPtrs[19] = NewPool<19>();
	poolPtrs[20] = NewPool<20>();
	poolPtrs[21] = NewPool<21>();
	poolPtrs[22] = NewPool<22>();
	poolPtrs[23] = NewPool<23>();
	poolPtrs[24] = NewPool<24>();
	poolPtrs[25] = NewPool<25>();
	poolPtrs[26] = NewPool<26>();
}

void LuaMemPool::PoolImpl::Kill() {
	KillPool< 0>();
	KillPool< 1>();
	KillPool< 2>();
	KillPool< 3>();
	KillPool< 4>();
	KillPool< 5>();
	KillPool< 6>();
	KillPool< 7>();
	KillPool< 8>();
	KillPool< 9>();
	KillPool<10>();
	KillPool<11>();
	KillPool<12>();
	KillPool<13>();
	KillPool<14>();
	KillPool<15>();
	KillPool<16>();
	KillPool<17>();
	KillPool<18>();
	KillPool<19>();
	KillPool<20>();
	KillPool<21>();
	KillPool<22>();
	KillPool<23>();
	KillPool<24>();
	KillPool<25>();
	KillPool<26>();

	poolPtrs.fill(nullptr);
}


void* LuaMemPool::PoolImpl::Alloc(uint32_t size) {
	const uint32_t subPoolIndex = CalcPoolIndex(size);

	numAllocs[subPoolIndex] += 1;
	allocSums[subPoolIndex] += size;
	numAllocs[   NUM_POOLS] += 1;
	allocSums[   NUM_POOLS] += size;

	switch (subPoolIndex) {
		case  0: { return (GetPool< 0>()->allocMem(size)); } break;
		case  1: { return (GetPool< 1>()->allocMem(size)); } break;
		case  2: { return (GetPool< 2>()->allocMem(size)); } break;
		case  3: { return (GetPool< 3>()->allocMem(size)); } break;
		case  4: { return (GetPool< 4>()->allocMem(size)); } break;
		case  5: { return (GetPool< 5>()->allocMem(size)); } break;
		case  6: { return (GetPool< 6>()->allocMem(size)); } break;
		case  7: { return (GetPool< 7>()->allocMem(size)); } break;
		case  8: { return (GetPool< 8>()->allocMem(size)); } break;
		case  9: { return (GetPool< 9>()->allocMem(size)); } break;
		case 10: { return (GetPool<10>()->allocMem(size)); } break;
		case 11: { return (GetPool<11>()->allocMem(size)); } break;
		case 12: { return (GetPool<12>()->allocMem(size)); } break;
		case 13: { return (GetPool<13>()->allocMem(size)); } break;
		case 14: { return (GetPool<14>()->allocMem(size)); } break;
		case 15: { return (GetPool<15>()->allocMem(size)); } break;
		case 16: { return (GetPool<16>()->allocMem(size)); } break;
		case 17: { return (GetPool<17>()->allocMem(size)); } break;
		case 18: { return (GetPool<18>()->allocMem(size)); } break;
		case 19: { return (GetPool<19>()->allocMem(size)); } break;
		case 20: { return (GetPool<20>()->allocMem(size)); } break;
		case 21: { return (GetPool<21>()->allocMem(size)); } break;
		case 22: { return (GetPool<22>()->allocMem(size)); } break;
		case 23: { return (GetPool<23>()->allocMem(size)); } break;
		case 24: { return (GetPool<24>()->allocMem(size)); } break;
		case 25: { return (GetPool<25>()->allocMem(size)); } break;
		case 26: { return (GetPool<26>()->allocMem(size)); } break;
		case 27: {                                         } break;
		case 28: {                                         } break;
		case 29: {                                         } break;
		case 30: {                                         } break;
		case 31: {                                         } break;
		default: {                                         } break;
	}

	// allocation too large, handle externally
	return nullptr;
}

void LuaMemPool::PoolImpl::Free(void* ptr, uint32_t size) {
	const uint32_t subPoolIndex = CalcPoolIndex(size);

	numAllocs[subPoolIndex] -= 1;
	allocSums[subPoolIndex] -= size;
	numAllocs[   NUM_POOLS] -= 1;
	allocSums[   NUM_POOLS] -= size;

	assert(ptr != nullptr);

	switch (subPoolIndex) {
		case  0: { return (GetPool< 0>()->freeMem(ptr)); } break;
		case  1: { return (GetPool< 1>()->freeMem(ptr)); } break;
		case  2: { return (GetPool< 2>()->freeMem(ptr)); } break;
		case  3: { return (GetPool< 3>()->freeMem(ptr)); } break;
		case  4: { return (GetPool< 4>()->freeMem(ptr)); } break;
		case  5: { return (GetPool< 5>()->freeMem(ptr)); } break;
		case  6: { return (GetPool< 6>()->freeMem(ptr)); } break;
		case  7: { return (GetPool< 7>()->freeMem(ptr)); } break;
		case  8: { return (GetPool< 8>()->freeMem(ptr)); } break;
		case  9: { return (GetPool< 9>()->freeMem(ptr)); } break;
		case 10: { return (GetPool<10>()->freeMem(ptr)); } break;
		case 11: { return (GetPool<11>()->freeMem(ptr)); } break;
		case 12: { return (GetPool<12>()->freeMem(ptr)); } break;
		case 13: { return (GetPool<13>()->freeMem(ptr)); } break;
		case 14: { return (GetPool<14>()->freeMem(ptr)); } break;
		case 15: { return (GetPool<15>()->freeMem(ptr)); } break;
		case 16: { return (GetPool<16>()->freeMem(ptr)); } break;
		case 17: { return (GetPool<17>()->freeMem(ptr)); } break;
		case 18: { return (GetPool<18>()->freeMem(ptr)); } break;
		case 19: { return (GetPool<19>()->freeMem(ptr)); } break;
		case 20: { return (GetPool<20>()->freeMem(ptr)); } break;
		case 21: { return (GetPool<21>()->freeMem(ptr)); } break;
		case 22: { return (GetPool<22>()->freeMem(ptr)); } break;
		case 23: { return (GetPool<23>()->freeMem(ptr)); } break;
		case 24: { return (GetPool<24>()->freeMem(ptr)); } break;
		case 25: { return (GetPool<25>()->freeMem(ptr)); } break;
		case 26: { return (GetPool<26>()->freeMem(ptr)); } break;
		case 27: {                                       } break;
		case 28: {                                       } break;
		case 29: {                                       } break;
		case 30: {                                       } break;
		case 31: {                                       } break;
	}
}

