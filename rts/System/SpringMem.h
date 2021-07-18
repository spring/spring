/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_MEM_H
#define SPRING_MEM_H

#include <vector>
#include <map>

namespace spring {
	void* AllocateAlignedMemory(size_t size, size_t alignment);
	void FreeAlignedMemory(void* ptr);
}

namespace spring {
    template <typename T>
    class StablePosAllocator {
    public:
        StablePosAllocator() = default;
        StablePosAllocator(size_t initialSize) :StablePosAllocator() {
            data.reserve(initialSize);
        }
        void Reset() {
            data.clear();
            sizeToPositions.clear();
            positionToSize.clear();
        }

        size_t Allocate(size_t numElems);
        void Free(size_t firstElem, size_t numElems);
        const size_t GetSize() const { return data.size(); }
        std::vector<T>& GetData() { return data; }
    private:
        std::vector<T> data;
        std::multimap<size_t, size_t> sizeToPositions;
        std::map<size_t, size_t> positionToSize;
    };

}

#endif