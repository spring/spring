/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_MEM_H
#define SPRING_MEM_H

namespace spring {
	void* AllocateAlignedMemory(size_t size, size_t alignment);
	void FreeAlignedMemory(void* ptr);
}

#endif