/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_INCLUDE_HPP
#define XSIMD_INCLUDE_HPP

#include "xsimd_instruction_set.hpp"

// X86 intruction sets
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_FMA3_VERSION // FMA3 and later
    #ifdef __GNUC__
        #include <x86intrin.h>         // x86intrin.h includes header files for whatever instruction
                                       // sets are specified on the compiler command line, such as:
                                       // xopintrin.h, fma4intrin.h
    #else
        #include <immintrin.h>         // MS version of immintrin.h covers AVX, AVX2 and FMA3
    #endif // __GNUC__
#elif XSIMD_X86_INSTR_SET == XSIMD_X86_AVX_VERSION
    #include <immintrin.h>             // AVX
#elif XSIMD_X86_INSTR_SET == XSIMD_X86_SSE4_2_VERSION
    #include <nmmintrin.h>             // SSE4.2
#elif XSIMD_X86_INSTR_SET == XSIMD_X86_SSE4_1_VERSION
    #include <smmintrin.h>             // SSE4.1
#elif XSIMD_X86_INSTR_SET == XSIMD_X86_SSSE3_VERSION
    #include <tmmintrin.h>             // SSSE3
#elif XSIMD_X86_INSTR_SET == XSIMD_X86_SSE3_VERSION
    #include <pmmintrin.h>             // SSE3
#elif XSIMD_X86_INSTR_SET == XSIMD_X86_SSE2_VERSION
    #include <emmintrin.h>             // SSE2
#elif XSIMD_X86_INSTR_SET == XSIMD_X86_SSE_VERSION
    #include <xmmintrin.h>             // SSE
#endif // XSIMD_X86_INSTR_SET

// AMD instruction sets
#if XSIMD_X86_AMD_INSTR_SET >= XSIMD_X86_AMD_FMA4_VERSION
    #ifdef _MSC_VER
        #include <intrin.h>
    #else
        #include <x86intrin.h>
        #if XSIMD_X86_AMD_INSTR_SET >= XSIMD_X86_AMD_XOP_VERSION
            #include <xopintrin.h>
        #else
            #include <fma4intrin.h>
        #endif
    #endif //  _MSC_VER
#elif XSIMD_X86_AMD_INSTR_SET == XSIMD_X86_AMD_SSE4A_VERSION
    #include <ammintrin.h>
#endif // XSIMD_X86_AMD_INSTR_SET

#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM7_NEON_VERSION
    #include <arm_neon.h>
#endif

// TODO: add ALTIVEC instruction set

#endif
