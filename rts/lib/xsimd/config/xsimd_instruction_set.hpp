/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

// Simplified version of boost.predef

#ifndef XSIMD_INSTRUCTION_SET_HPP
#define XSIMD_INSTRUCTION_SET_HPP

/******************
 * VERSION NUMBER *
 ******************/

// major number can be in [0, 99]
// minor number can be in [0, 99]
// patch number can be in [0, 999999]
#define XSIMD_VERSION_NUMBER(major, minor, patch) \
    ((((major) % 100) * 10000000) + (((minor) % 100) * 100000) + ((patch) % 100000))

#define XSIMD_VERSION_NUMBER_NOT_AVAILABLE \
    XSIMD_VERSION_NUMBER(0, 0, 0)

#define XSIMD_VERSION_NUMBER_AVAILABLE \
    XSIMD_VERSION_NUMBER(0, 0, 1)

/*************************
 * CLEAR INSTRUCTION SET *
 *************************/

#undef XSIMD_X86_INSTR_SET
#undef XSIMD_X86_INSTR_SET_AVAILABLE

#undef XSIMD_X86_AMD_INSTR_SET
#undef XSIMD_X86_AMD_INSTR_SET_AVAILABLE

#undef XSIMD_PPC_INSTR_SET
#undef XSIMD_PPC_INSTR_SET_AVAILABLE

#undef XSIMD_ARM_INSTR_SET
#undef XSIMD_ARM_INSTR_SET_AVAILABLE

/**********************
 * USER CONFIGURATION *
 **********************/

#ifdef XSIMD_FORCE_X86_INSTR_SET
    #define XSIMD_X86_INSTR_SET XSIMD_FORCE_X86_INSTR_SET
    #define XSIMD_X86_INSTR_SET_AVAILABLE XSIMD_VERSION_NUMBER_AVAILABLE
    #ifndef XSIMD_SKIP_ON_WERROR
        #ifdef _MSC_VER
            #pragma message("Warning: Forcing X86 instruction set")
        #else
            #warning "Forcing X86 instruction set"
        #endif
    #endif
#elif defined(XSIMD_FORCE_X86_AMD_INSTR_SET)
    #define XSIMD_X86_AMD_INSTR_SET XSIMD_FORCE_X86_AMD_INSTR_SET
    #define XSIMD_X86_AMD_INSTR_SET_AVAILABLE XSIMD_VERSION_NUMBER_AVAILABLE
    #ifndef XSIMD_SKIP_ON_WERROR
        #ifdef _MSC_VER
            #pragma message("Warning: Forcing X86 AMD instruction set")
        #else
            #warning "Forcing X86 AMD instruction set"
        #endif
    #endif
#elif defined(XSIMD_FORCE_PPC_INSTR_SET)
    #define XSIMD_PPC_INSTR_SET XSIMD_FORCE_PPC_INSTR_SET
    #define XSIMD_PPC_INSTR_SET_AVAILABLE XSIMD_VERSION_NUMBER_AVAILABLE
    #ifndef XSIMD_SKIP_ON_WERROR
        #ifdef _MSC_VER
            #pragma message("Warning: Forcing PPC instruction set")
        #else
            #warning "Forcing PPC instruction set"
        #endif
    #endif
#elif defined(XSIMD_FORCE_ARM_INSTR_SET)
    #define XSIMD_ARM_INSTR_SET XSIMD_FORCE_ARM_INSTR_SET
    #define XSIMD_ARM_INSTR_SET_AVAILABLE XSIMD_VERSION_NUMBER_AVAILABLE
    #ifndef XSIMD_SKIP_ON_WERROR
        #ifdef _MSC_VER
            #pragma message("Warning: Forcing ARM instruction set")
        #else
            #warning "Forcing ARM instruction set"
        #endif
    #endif
#endif

/***********************
 * X86 INSTRUCTION SET *
 ***********************/

#define XSIMD_X86_SSE_VERSION XSIMD_VERSION_NUMBER(1, 0, 0)
#define XSIMD_X86_SSE2_VERSION XSIMD_VERSION_NUMBER(2, 0, 0)
#define XSIMD_X86_SSE3_VERSION XSIMD_VERSION_NUMBER(3, 0, 0)
#define XSIMD_X86_SSSE3_VERSION XSIMD_VERSION_NUMBER(3, 1, 0)
#define XSIMD_X86_SSE4_1_VERSION XSIMD_VERSION_NUMBER(4, 1, 0)
#define XSIMD_X86_SSE4_2_VERSION XSIMD_VERSION_NUMBER(4, 2, 0)
#define XSIMD_X86_AVX_VERSION XSIMD_VERSION_NUMBER(5, 0, 0)
#define XSIMD_X86_FMA3_VERSION XSIMD_VERSION_NUMBER(5, 2, 0)
#define XSIMD_X86_AVX2_VERSION XSIMD_VERSION_NUMBER(5, 3, 0)
#define XSIMD_X86_AVX512_VERSION XSIMD_VERSION_NUMBER(6, 0, 0)
#define XSIMD_X86_MIC_VERSION XSIMD_VERSION_NUMBER(9, 0, 0)

#if !defined(XSIMD_X86_INSTR_SET) && defined(__MIC__)
    #define XSIMD_X86_INSTR_SET XSIMD_X86_MIC_VERSION
#endif

// AVX512 instructions are supported starting with gcc 6
// see https://www.gnu.org/software/gcc/gcc-6/changes.html
#if !defined(XSIMD_X86_INSTR_SET) && (defined(__AVX512__) || defined(__KNCNI__) || defined(__AVX512F__)\
    && (defined(__clang__) || (!defined(__GNUC__) || __GNUC__ >= 6)))
    #define XSIMD_X86_INSTR_SET XSIMD_X86_AVX512_VERSION

    #if defined(__AVX512VL__)
        #define XSIMD_AVX512VL_AVAILABLE 1
    #endif

    #if defined(__AVX512DQ__)
        #define XSIMD_AVX512DQ_AVAILABLE 1
    #endif

    #if defined(__AVX512BW__)
        #define XSIMD_AVX512BW_AVAILABLE 1
    #endif

    #if __GNUC__ == 6
        #define XSIMD_AVX512_SHIFT_INTRINSICS_IMM_ONLY 1
    #endif
#endif

#if !defined(XSIMD_X86_INSTR_SET) && defined(__AVX2__)
    #define XSIMD_X86_INSTR_SET XSIMD_X86_AVX2_VERSION
#endif

#if !defined(XSIMD_X86_INSTR_SET) && defined(__FMA__)
    #define XSIMD_X86_INSTR_SET XSIMD_X86_FMA3_VERSION
#endif

#if !defined(XSIMD_X86_INSTR_SET) && defined(__AVX__)
    #define XSIMD_X86_INSTR_SET XSIMD_X86_AVX_VERSION
#endif

#if !defined(XSIMD_X86_INSTR_SET) && defined(__SSE4_2__)
    #define XSIMD_X86_INSTR_SET XSIMD_X86_SSE4_2_VERSION
#endif

#if !defined(XSIMD_X86_INSTR_SET) && defined(__SSE4_1__)
    #define XSIMD_X86_INSTR_SET XSIMD_X86_SSE4_1_VERSION
#endif

#if !defined(XSIMD_X86_INSTR_SET) && defined(__SSSE3__)
    #define XSIMD_X86_INSTR_SET XSIMD_X86_SSSE3_VERSION
#endif

#if !defined(XSIMD_X86_INSTR_SET) && defined(__SSE3__)
    #define XSIMD_X86_INSTR_SET XSIMD_X86_SSE3_VERSION
#endif

#if !defined(XSIMD_X86_INSTR_SET) && (defined(__SSE2__) || defined(__x86_64__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2))
    #define XSIMD_X86_INSTR_SET XSIMD_X86_SSE2_VERSION
#endif

#if !defined(XSIMD_X86_INSTR_SET) && (defined(__SSE__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1))
    #define XSIMD_X86_INSTR_SET XSIMD_X86_SSE_VERSION
#endif

#if !(defined XSIMD_X86_INSTR_SET)
    #define XSIMD_X86_INSTR_SET XSIMD_VERSION_NUMBER_NOT_AVAILABLE
#else
    #define XSIMD_X86_INSTR_SET_AVAILABLE XSIMD_VERSION_NUMBER_AVAILABLE
#endif

/***************************
 * X86_AMD INSTRUCTION SET *
 ***************************/

#define XSIMD_X86_AMD_SSE4A_VERSION XSIMD_VERSION_NUMBER(4, 0, 0)
#define XSIMD_X86_AMD_FMA4_VERSION XSIMD_VERSION_NUMBER(5, 1, 0)
#define XSIMD_X86_AMD_XOP_VERSION XSIMD_VERSION_NUMBER(5, 1, 1)

#if !defined(XSIMD_X86_AMD_INSTR_SET) && defined(__XOP__)
    #define XSIMD_X86_AMD_INSTR_SET XSIMD_X86_AMD_XOP_VERSION
#endif

#if !defined(XSIMD_X86_AMD_INSTR_SET) && defined(__FMA4__)
    #define XSIMD_X86_AMD_INSTR_SET XSIMD_X86_AMD_FMA4_VERSION
#endif

#if !defined(XSIMD_X86_AMD_INSTR_SET) && defined(__SSE4A__)
    #define XSIMD_X86_AMD_INSTR_SET XSIMD_X86_AMD_SSE4A_VERSION
#endif

#if !defined(XSIMD_X86_AMD_INSTR_SET)
    #define XSIMD_X86_AMD_INSTR_SET XSIMD_VERSION_NUMBER_NOT_AVAILABLE
#else
    // X86_AMD implies X86
    #if XSIMD_X86_INSTR_SET > XSIMD_X86_AMD_INSTR_SET
        #undef XSIMD_X86_AMD_INSTR_SET
        #define XSIMD_X86_AMD_INSTR_SET XSIMD_X86_INSTR_SET
    #endif
    #define XSIMD_X86_AMD_INSTR_SET_AVAILABLE XSIMD_VERSION_NUMBER_AVAILABLE
#endif

/***********************
 * PPC INSTRUCTION SET *
 ***********************/

// We haven't implemented any support for PPC, so we should
// not enable detection for this instructoin set
/*#define XSIMD_PPC_VMX_VERSION XSIMD_VERSION_NUMBER(1, 0, 0)
#define XSIMD_PPC_VSX_VERSION XSIMD_VERSION_NUMBER(1, 1, 0)
#define XSIMD_PPC_QPX_VERSION XSIMD_VERSION_NUMBER(2, 0, 0)

#if !defined(XSIMD_PPC_INSTR_SET) && defined(__VECTOR4DOUBLE__)
    #define XSIMD_PPC_INSTR_SET XSIMD_PPC_QPX_VERSION
#endif

#if !defined(XSIMD_PPC_INSTR_SET) && defined(__VSX__)
    #define XSIMD_PPC_INSTR_SET XSIMD_PPC_VSX_VERSION
#endif

#if !defined(XSIMD_PPC_INSTR_SET) && (defined(__ALTIVEC__) || defined(__VEC__))
    #define XSIMD_PPC_INSTR_SET XSIMD_PPC_VMX_VERSION
#endif

#if !defined(XSIMD_PPC_INSTR_SET)
    #define XSIMD_PPC_INSTR_SET XSIMD_VERSION_NUMBER_NOT_AVAILABLE
#else
    #define XSIMD_PPC_INSTR_SET_AVAILABLE XSIMD_VERSION_NUMBER_AVAILABLE
#endif*/

/***********************
 * ARM INSTRUCTION SET *
 ***********************/

#define XSIMD_ARM7_NEON_VERSION XSIMD_VERSION_NUMBER(7, 0, 0)
#define XSIMD_ARM8_32_NEON_VERSION XSIMD_VERSION_NUMBER(8, 0, 0)
#define XSIMD_ARM8_64_NEON_VERSION XSIMD_VERSION_NUMBER(8, 1, 0)

// TODO __ARM_FEATURE_FMA
#if !defined(XSIMD_ARM_INSTR_SET) && (defined(__ARM_NEON))
    #if __ARM_ARCH >= 8
        #if defined(__aarch64__)
            #define XSIMD_ARM_INSTR_SET XSIMD_ARM8_64_NEON_VERSION
        #else
            #define XSIMD_ARM_INSTR_SET XSIMD_ARM8_32_NEON_VERSION
        #endif
    #elif __ARM_ARCH >= 7
        #define XSIMD_ARM_INSTR_SET XSIMD_ARM7_NEON_VERSION
    #elif defined(XSIMD_ENABLE_FALLBACK)
        #warning "NEON instruction set not supported, using fallback mode."
    #else
        static_assert(false, "NEON instruction set not supported.");
    #endif
#endif

#if !defined(XSIMD_ARM_INSTR_SET)
    #define XSIMD_ARM_INSTR_SET XSIMD_VERSION_NUMBER_NOT_AVAILABLE
#else
    #define XSIMD_ARM_INSTR_SET_AVAILABLE XSIMD_VERSION_NUMBER_AVAILABLE
#endif

/***************************
 * GENERIC INSTRUCTION SET *
 ***************************/

#undef XSIMD_INSTR_SET
#undef XSIMD_INSTR_SET_AVAILABLE

#if defined(XSIMD_X86_AMD_AVAILABLE)
    #if XSIMD_X86_INSTR_SET > XSIMD_X86_AMD_INSTR_SET
        #define XSIMD_INSTR_SET XSIMD_X86_INSTR_SET
    #else
        #define XSIMD_INSTR_SET XSIMD_X86_AMD_INSTR_SET
    #endif
#endif

#if !defined(XSIMD_INSTR_SET) && defined(XSIMD_X86_INSTR_SET_AVAILABLE)
    #define XSIMD_INSTR_SET XSIMD_X86_INSTR_SET
#endif

#if !defined(XSIMD_INSTR_SET) && defined(XSIMD_PPC_INSTR_SET_AVAILABLE)
    #define XSIMD_INSTR_SET XSIMD_PPC_INSTR_SET
#endif

#if !defined(XSIMD_INSTR_SET) && defined(XSIMD_ARM_INSTR_SET_AVAILABLE)
    #define XSIMD_INSTR_SET XSIMD_ARM_INSTR_SET
#endif

#if !defined(XSIMD_INSTR_SET)
    #define XSIMD_INSTR_SET XSIMD_VERSION_NUMBER_NOT_AVAILABLE
#elif XSIMD_INSTR_SET != XSIMD_VERSION_NUMBER_NOT_AVAILABLE
    #define XSIMD_INSTR_SET_AVAILABLE XSIMD_VERSION_NUMBER_AVAILABLE
#endif

#endif
