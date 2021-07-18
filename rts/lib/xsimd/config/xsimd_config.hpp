/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_CONFIG_HPP
#define XSIMD_CONFIG_HPP

#include "xsimd_align.hpp"

#define XSIMD_VERSION_MAJOR 7
#define XSIMD_VERSION_MINOR 5
#define XSIMD_VERSION_PATCH 0

#ifndef XSIMD_DEFAULT_ALLOCATOR
    #if XSIMD_X86_INSTR_SET_AVAILABLE
        #define XSIMD_DEFAULT_ALLOCATOR(T) xsimd::aligned_allocator<T, XSIMD_DEFAULT_ALIGNMENT>
    #else
        #define XSIMD_DEFAULT_ALLOCATOR(T) std::allocator<T>
    #endif
#endif

#ifndef XSIMD_STACK_ALLOCATION_LIMIT
    #define XSIMD_STACK_ALLOCATION_LIMIT 20000
#endif

#if defined(__LP64__) || defined(_WIN64)
    #define XSIMD_64_BIT_ABI
#else
    #define XSIMD_32_BIT_ABI
#endif

#endif
