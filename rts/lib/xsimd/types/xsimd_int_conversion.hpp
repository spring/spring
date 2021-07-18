/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_INT_CONVERSION_HPP
#define XSIMD_INT_CONVERSION_HPP

#include "xsimd_base.hpp"

namespace xsimd
{
    namespace detail
    {
        /************************************
         * conversion of 8 int8 <-> 8 int32 *
         ************************************/

        // a contains 8 int8 in its low half
        __m256i xsimd_cvtepi8_epi32(__m128i a);
        __m256i xsimd_cvtepu8_epi32(__m128i a);

        // Returns an vector containing 8 int8 in its low half
        __m128i xsimd_cvtepi32_epi8(__m256i a);
        __m128i xsimd_cvtepi32_epu8(__m256i a);

        // a contains 16 int8
        __m256i xsimd_cvtepi16_epi32(__m128i a);
        __m256i xsimd_cvtepu16_epi32(__m128i a);

        // Returns an vector containing 8 int16
        __m128i xsimd_cvtepi32_epi16(__m256i a);
        __m128i xsimd_cvtepi32_epu16(__m256i a);

        /******************
         * Implementation *
         ******************/

        inline __m256i xsimd_cvtepi8_epi32(__m128i a)
        {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
            __m256i res = _mm256_cvtepi8_epi32(a);
#else
            __m128i mask = _mm_cmplt_epi8(a, _mm_set1_epi8(0));
            __m128i tmp1 = _mm_unpacklo_epi8(a, mask);
            mask = _mm_cmplt_epi16(tmp1, _mm_set1_epi16(0));
            __m128i tmp2 = _mm_unpacklo_epi16(tmp1, mask);
            __m128i tmp3 = _mm_unpackhi_epi16(tmp1, mask);
            __m256i res = _mm256_castsi128_si256(tmp2);
            res = _mm256_insertf128_si256(res, tmp3, 1);
#endif
            return res;
        }
        
        inline __m256i xsimd_cvtepu8_epi32(__m128i a)
        {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
            __m256i res = _mm256_cvtepu8_epi32(a);
#else
            __m128i tmp1 = _mm_unpacklo_epi8(a, _mm_set1_epi8(0));
            __m128i tmp2 = _mm_unpacklo_epi16(tmp1, _mm_set1_epi16(0));
            __m128i tmp3 = _mm_unpackhi_epi16(tmp1, _mm_set1_epi16(0));
            __m256i res = _mm256_castsi128_si256(tmp2);
            res = _mm256_insertf128_si256(res, tmp3, 1);
#endif
            return res;
        }

        inline __m128i xsimd_cvtepi32_epi8(__m256i a)
        {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
            __m256i tmp2 = _mm256_packs_epi32(a, a);
            __m256i tmp3 = _mm256_permute4x64_epi64(tmp2, _MM_SHUFFLE(3, 1, 2, 0));
            __m256i tmp4 = _mm256_packs_epi16(tmp3, _mm256_set1_epi16(0));
            __m128i res = _mm256_castsi256_si128(tmp4);
#else
            __m128i tmp_hi = _mm256_extractf128_si256(a, 1);
            __m128i tmp_lo = _mm256_castsi256_si128(a);
            tmp_hi = _mm_packs_epi32(tmp_hi, tmp_hi);
            tmp_lo = _mm_packs_epi32(tmp_lo, tmp_lo);
            __m128i res = _mm_unpacklo_epi64(tmp_lo, tmp_hi);
            res = _mm_packs_epi16(res, _mm_set1_epi16(0));
#endif
            return res;
        }
        
        inline __m128i xsimd_cvtepi32_epu8(__m256i a)
        {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
            __m256i tmp2 = _mm256_packs_epi32(a, a);
            __m256i tmp3 = _mm256_permute4x64_epi64(tmp2, _MM_SHUFFLE(3, 1, 2, 0));
            __m256i tmp4 = _mm256_packus_epi16(tmp3, _mm256_set1_epi16(0));
            __m128i res = _mm256_castsi256_si128(tmp4);
#else
            __m128i tmp_hi = _mm256_extractf128_si256(a, 1);
            __m128i tmp_lo = _mm256_castsi256_si128(a);
            tmp_hi = _mm_packs_epi32(tmp_hi, tmp_hi);
            tmp_lo = _mm_packs_epi32(tmp_lo, tmp_lo);
            __m128i res = _mm_unpacklo_epi64(tmp_lo, tmp_hi);
            res = _mm_packus_epi16(res, _mm_set1_epi16(0));
#endif
            return res;
        }

        inline __m256i xsimd_cvtepi16_epi32(__m128i a)
        {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
            __m256i res = _mm256_cvtepi16_epi32(a);
#else
            __m128i mask = _mm_cmplt_epi16(a, _mm_set1_epi16(0));
            __m128i tmp1 = _mm_unpacklo_epi16(a, mask);
            __m128i tmp2 = _mm_unpackhi_epi16(a, mask);
            __m256i res = _mm256_castsi128_si256(tmp1);
            res = _mm256_insertf128_si256(res, tmp2, 1);
#endif
            return res;
        }

        inline __m256i xsimd_cvtepu16_epi32(__m128i a)
        {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
            __m256i res = _mm256_cvtepu16_epi32(a);
#else
            __m128i tmp1 = _mm_unpacklo_epi16(a, _mm_set1_epi16(0));
            __m128i tmp2 = _mm_unpackhi_epi16(a, _mm_set1_epi16(0));
            __m256i res = _mm256_castsi128_si256(tmp1);
            res = _mm256_insertf128_si256(res, tmp2, 1);
#endif
            return res;
        }

        inline __m128i xsimd_cvtepi32_epi16(__m256i a)
        {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
            __m256i tmp1 = _mm256_packs_epi32(a, a);
            __m256i tmp2 = _mm256_permute4x64_epi64(tmp1, _MM_SHUFFLE(3, 1, 2, 0));
            __m128i res = _mm256_castsi256_si128(tmp2);
#else
            __m128i tmp_hi = _mm256_extractf128_si256(a, 1);
            __m128i tmp_lo = _mm256_castsi256_si128(a);
            tmp_hi = _mm_packs_epi32(tmp_hi, tmp_hi);
            tmp_lo = _mm_packs_epi32(tmp_lo, tmp_lo);
            __m128i res = _mm_unpacklo_epi64(tmp_lo, tmp_hi);
#endif
            return res;
        }

        inline __m128i xsimd_cvtepi32_epu16(__m256i a)
        {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
            __m256i tmp1 = _mm256_packus_epi32(a, a);
            __m256i tmp2 = _mm256_permute4x64_epi64(tmp1, _MM_SHUFFLE(3, 1, 2, 0));
            __m128i res = _mm256_castsi256_si128(tmp2);
#else
            __m128i tmp_hi = _mm256_extractf128_si256(a, 1);
            __m128i tmp_lo = _mm256_castsi256_si128(a);
            tmp_hi = _mm_packus_epi32(tmp_hi, tmp_hi);
            tmp_lo = _mm_packus_epi32(tmp_lo, tmp_lo);
            __m128i res = _mm_unpacklo_epi64(tmp_lo, tmp_hi);
#endif
            return res;
        }
    }
}

#endif
