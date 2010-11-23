/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATH_ALLOCATOR_HDR
#define PATH_ALLOCATOR_HDR

struct PathAllocator {
	static void* Alloc(unsigned int n);
	static void Free(void* p, unsigned int n);
};

#endif
