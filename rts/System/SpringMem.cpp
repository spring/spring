#ifdef _WIN32
    #include <malloc.h>
#else
    #include <cstdlib>
#endif

#include "SpringMem.h"

#include <cassert>


void* spring::AllocateAlignedMemory(size_t size, size_t alignment)
{
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#else
    void* ptr = nullptr;
    posix_memalign(&ptr, alignment, size);
    return ptr;
#endif
}

void spring::FreeAlignedMemory(void* ptr)
{
    if (ptr)
    {
#ifdef _WIN32
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }
}


template<typename T>
size_t spring::StablePosAllocator<T>::Allocate(size_t numElems)
{
    assert(numElems > 0);

    //no gaps
    if (positionToSize.empty()) {
        size_t returnPos = data.size();
        data.resize(data.size() + numElems);
        return returnPos;
    }

    //try to find gaps >= in size than requested
    for (auto it = sizeToPositions.lower_bound(numElems); it != sizeToPositions.end(); ++it) {
        if (it->first < numElems)
            continue;

        if (it->first > numElems) {
            size_t gapSize = it->first - numElems;
            size_t gapPos = it->second + numElems;
            sizeToPositions.emplace(gapSize, gapPos);
            positionToSize.emplace(gapPos, gapSize);
        }

        size_t returnPos = it->second;
        positionToSize.erase(it->second);
        sizeToPositions.erase(it);
        return returnPos;
    }

    //all gaps are too small
    size_t returnPos = data.size();
    data.resize(data.size() + numElems);
    return returnPos;
}
template<typename T>
void spring::StablePosAllocator<T>::Free(size_t firstElem, size_t numElems)
{
    assert(firstElem + numElems <= data.size());
    assert(numElems > 0);

    //lucky us
    if (firstElem + numElems == data.size()) {
        data.resize(firstElem);
        return;
    }

    const auto eraseSizeToPositionsKV = [this](size_t size, size_t pos) {
        auto [beg, end] = sizeToPositions.equal_range(size);
        for (auto it = beg; it != end; /*noop*/)
            if (it->second == pos)
                it = sizeToPositions.erase(it);
            else
                ++it;
    };

    //check for merge opportunity before firstElem positionToSize
    if (!positionToSize.empty()) {
        auto positionToSizeBeforeIt = positionToSize.lower_bound(firstElem);
        if (positionToSizeBeforeIt == positionToSize.end())
            positionToSizeBeforeIt = positionToSize.begin();

        if (positionToSizeBeforeIt != positionToSize.begin())
            --positionToSizeBeforeIt;

        if (positionToSizeBeforeIt->first + positionToSizeBeforeIt->second == firstElem) {
            // [gapBefore][thisGap] ==> [newBiggerGap]

            //erase old sizeToPositions
            eraseSizeToPositionsKV(positionToSizeBeforeIt->second, positionToSizeBeforeIt->first);
            //patch up positionToSize
            positionToSizeBeforeIt->second += numElems;
            //emplace new sizeToPositions
            sizeToPositions.emplace(positionToSizeBeforeIt->second, positionToSizeBeforeIt->first);

            return;
        }
    }

    //check for merge opportunity after firstElem positionToSize
    if (!positionToSize.empty()) {
        auto positionToSizeAfterIt = positionToSize.upper_bound(firstElem + numElems); //guaranteed to be after firstElem
        if (positionToSizeAfterIt != positionToSize.end()) {
            if (firstElem + numElems == positionToSizeAfterIt->first) {
                // [thisGap][gapAfter] ==> [newBiggerGap]

                //erase old sizeToPositions
                eraseSizeToPositionsKV(positionToSizeAfterIt->second, positionToSizeAfterIt->first);
                //emplace new positionToSize
                positionToSize.emplace(firstElem, numElems + positionToSizeAfterIt->second);
                //emplace new sizeToPositions
                sizeToPositions.emplace(numElems + positionToSizeAfterIt->second, firstElem);
                //erase old positionToSize
                positionToSize.erase(positionToSizeAfterIt);

                return;
            }
        }
    }

    //no adjacent gaps found
    positionToSize.emplace(firstElem, numElems);
    sizeToPositions.emplace(numElems, firstElem);
}