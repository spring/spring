/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_ALIGN_HPP
#define XSIMD_ALIGN_HPP

#include "xsimd_instruction_set.hpp"

/************************************************
 * Platform checks for aligned malloc functions *
 ************************************************/

#if ((defined __QNXNTO__) || (defined _GNU_SOURCE) || ((defined _XOPEN_SOURCE) && (_XOPEN_SOURCE >= 600))) \
 && (defined _POSIX_ADVISORY_INFO) && (_POSIX_ADVISORY_INFO > 0)
  #define XSIMD_HAS_POSIX_MEMALIGN 1
#else
  #define XSIMD_HAS_POSIX_MEMALIGN 0
#endif

#if defined(XSIMD_X86_INSTR_SET_AVAILABLE)
    #define XSIMD_HAS_MM_MALLOC 1
#else
    #define XSIMD_HAS_MM_MALLOC 0
#endif

/********************
 * Stack allocation *
 ********************/

#ifndef XSIMD_ALLOCA
    #if defined(__linux__)
        #define XSIMD_ALLOCA alloca
    #elif defined(_MSC_VER)
        #define XSIMD_ALLOCA _alloca
    #endif
#endif

/*********************
 * Default alignment *
 *********************/

#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX512_VERSION
    #define XSIMD_DEFAULT_ALIGNMENT 64
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX_VERSION
    #define XSIMD_DEFAULT_ALIGNMENT 32
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE2_VERSION
    #define XSIMD_DEFAULT_ALIGNMENT 16
#elif XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
    #define XSIMD_DEFAULT_ALIGNMENT 32
#elif XSIMD_ARM_INSTR_SET >= XSIMD_ARM7_NEON_VERSION
    #define XSIMD_DEFAULT_ALIGNMENT 16
#else
    // some versions of gcc do nos handle alignment set ot 0
    // see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=69089
    #if __GNUC__ < 7
        #define XSIMD_DEFAULT_ALIGNMENT 8
    #else
        #define XSIMD_DEFAULT_ALIGNMENT 0
    #endif
#endif

#endif
