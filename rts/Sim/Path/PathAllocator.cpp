/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring>
#include "PathAllocator.h"

void* PathAllocator::Alloc(unsigned int n)
{
	char* ret = new char[n];
	memset(ret, 0, n);
	return ret;
}

void PathAllocator::Free(void* p, unsigned int)
{
	delete[] ((char*) p);
}
