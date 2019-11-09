/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_LUA_ALLOC_STATE_H
#define SPRING_LUA_ALLOC_STATE_H

#include <atomic>

struct SLuaAllocState {
	static constexpr uint64_t MAX_ALLOC_BYTES[] = {
		 768u * (1024u * 1024u), // spring32
		1536u * (1024u * 1024u), // spring64
	};

	std::atomic<uint64_t> allocedBytes;
	std::atomic<uint64_t> numLuaAllocs;
	std::atomic<uint64_t> luaAllocTime;
	std::atomic<uint64_t> numLuaStates;
};

#endif

