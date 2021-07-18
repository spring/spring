/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_AVX_CONVERSION_HPP
#define XSIMD_AVX_CONVERSION_HPP

#include "xsimd_avx_double.hpp"
#include "xsimd_avx_float.hpp"
#include "xsimd_avx_int8.hpp"
#include "xsimd_avx_int16.hpp"
#include "xsimd_avx_int32.hpp"
#include "xsimd_avx_int64.hpp"

namespace xsimd
{

    /************************
     * conversion functions *
     ************************/

    batch<int32_t, 8> to_int(const batch<float, 8>& x);
    batch<int64_t, 4> to_int(const batch<double, 4>& x);

    batch<float, 8> to_float(const batch<int32_t, 8>& x);
    batch<double, 4> to_float(const batch<int64_t, 4>& x);

    /**************************
     * boolean cast functions *
     **************************/

    batch_bool<int32_t, 8> bool_cast(const batch_bool<float, 8>& x);
    batch_bool<int64_t, 4> bool_cast(const batch_bool<double, 4>& x);
    batch_bool<float, 8> bool_cast(const batch_bool<int32_t, 8>& x);
    batch_bool<double, 4> bool_cast(const batch_bool<int64_t, 4>& x);
    
    /***************************************
     * conversion functions implementation *
     ***************************************/

    inline batch<int32_t, 8> to_int(const batch<float, 8>& x)
    {
        return _mm256_cvttps_epi32(x);
    }

    inline batch<int64_t, 4> to_int(const batch<double, 4>& x)
    {
#if defined(XSIMD_AVX512VL_AVAILABLE) & defined(XSIMD_AVX512DQ_AVAILABLE)
        return _mm256_cvttpd_epi64(x);
#else
        return batch<int64_t, 4>(static_cast<int64_t>(x[0]),
                                 static_cast<int64_t>(x[1]),
                                 static_cast<int64_t>(x[2]),
                                 static_cast<int64_t>(x[3]));
#endif
    }

    inline batch<float, 8> to_float(const batch<int32_t, 8>& x)
    {
        return _mm256_cvtepi32_ps(x);
    }

    inline batch<double, 4> to_float(const batch<int64_t, 4>& x)
    {
#if defined(XSIMD_AVX512VL_AVAILABLE) & defined(XSIMD_AVX512DQ_AVAILABLE)
        return _mm256_cvtepi64_pd(x);
#else
        return batch<double, 4>(static_cast<double>(x[0]),
                                static_cast<double>(x[1]),
                                static_cast<double>(x[2]),
                                static_cast<double>(x[3]));
#endif
    }

    /*****************************************
     * batch cast functions implementation *
     *****************************************/

    XSIMD_BATCH_CAST_IMPLICIT(int8_t, uint8_t, 32)
    XSIMD_BATCH_CAST_IMPLICIT(uint8_t, int8_t, 32)
    XSIMD_BATCH_CAST_IMPLICIT(int16_t, uint16_t, 16)
    XSIMD_BATCH_CAST_IMPLICIT(uint16_t, int16_t, 16)
    XSIMD_BATCH_CAST_IMPLICIT(int32_t, uint32_t, 8)
    XSIMD_BATCH_CAST_INTRINSIC(int32_t, float, 8, _mm256_cvtepi32_ps)
    XSIMD_BATCH_CAST_INTRINSIC(int32_t, double, 4, _mm256_cvtepi32_pd)
    XSIMD_BATCH_CAST_IMPLICIT(uint32_t, int32_t, 8)
    XSIMD_BATCH_CAST_IMPLICIT(int64_t, uint64_t, 4)
    XSIMD_BATCH_CAST_IMPLICIT(uint64_t, int64_t, 4)
    XSIMD_BATCH_CAST_INTRINSIC(float, int32_t, 8, _mm256_cvttps_epi32)
    XSIMD_BATCH_CAST_INTRINSIC(float, double, 4, _mm256_cvtps_pd)
    XSIMD_BATCH_CAST_INTRINSIC(double, int32_t, 4, _mm256_cvttpd_epi32)
    XSIMD_BATCH_CAST_INTRINSIC(double, float, 4, _mm256_cvtpd_ps)
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
    XSIMD_BATCH_CAST_INTRINSIC(int8_t, int16_t, 16, _mm256_cvtepi8_epi16)
    XSIMD_BATCH_CAST_INTRINSIC(int8_t, uint16_t, 16, _mm256_cvtepi8_epi16)
    XSIMD_BATCH_CAST_INTRINSIC(uint8_t, int16_t, 16, _mm256_cvtepu8_epi16)
    XSIMD_BATCH_CAST_INTRINSIC(uint8_t, uint16_t, 16, _mm256_cvtepu8_epi16)
    XSIMD_BATCH_CAST_INTRINSIC(int16_t, int32_t, 8, _mm256_cvtepi16_epi32)
    XSIMD_BATCH_CAST_INTRINSIC(int16_t, uint32_t, 8, _mm256_cvtepi16_epi32)
    XSIMD_BATCH_CAST_INTRINSIC2(int16_t, float, 8, _mm256_cvtepi16_epi32, _mm256_cvtepi32_ps)
    XSIMD_BATCH_CAST_INTRINSIC(uint16_t, int32_t, 8, _mm256_cvtepu16_epi32)
    XSIMD_BATCH_CAST_INTRINSIC(uint16_t, uint32_t, 8, _mm256_cvtepu16_epi32)
    XSIMD_BATCH_CAST_INTRINSIC2(uint16_t, float, 8, _mm256_cvtepu16_epi32, _mm256_cvtepi32_ps)
    XSIMD_BATCH_CAST_INTRINSIC(int32_t, int64_t, 4, _mm256_cvtepi32_epi64)
    XSIMD_BATCH_CAST_INTRINSIC(int32_t, uint64_t, 4, _mm256_cvtepi32_epi64)
    XSIMD_BATCH_CAST_INTRINSIC(uint32_t, int64_t, 4, _mm256_cvtepu32_epi64)
    XSIMD_BATCH_CAST_INTRINSIC(uint32_t, uint64_t, 4, _mm256_cvtepu32_epi64)
#endif
#if defined(XSIMD_AVX512VL_AVAILABLE)
    #if defined(XSIMD_AVX512BW_AVAILABLE)
    XSIMD_BATCH_CAST_INTRINSIC(int16_t, int8_t, 16, _mm256_cvtepi16_epi8)
    XSIMD_BATCH_CAST_INTRINSIC(int16_t, uint8_t, 16, _mm256_cvtepi16_epi8)
    XSIMD_BATCH_CAST_INTRINSIC(uint16_t, int8_t, 16, _mm256_cvtepi16_epi8)
    XSIMD_BATCH_CAST_INTRINSIC(uint16_t, uint8_t, 16, _mm256_cvtepi16_epi8)
#endif
    XSIMD_BATCH_CAST_INTRINSIC(int32_t, int16_t, 8, _mm256_cvtepi32_epi16)
    XSIMD_BATCH_CAST_INTRINSIC(int32_t, uint16_t, 8, _mm256_cvtepi32_epi16)
    XSIMD_BATCH_CAST_INTRINSIC(uint32_t, int16_t, 8, _mm256_cvtepi32_epi16)
    XSIMD_BATCH_CAST_INTRINSIC(uint32_t, uint16_t, 8, _mm256_cvtepi32_epi16)
#if defined(_MSC_VER)
    namespace detail {
        static inline __m256 xsimd_mm256_cvtepu32_ps(__m256i a)
        {
          return _mm512_castps512_ps256(_mm512_cvtepu32_ps(_mm512_castsi256_si512(a)));
        }
    }
    XSIMD_BATCH_CAST_INTRINSIC(uint32_t, float, 8, detail::xsimd_mm256_cvtepu32_ps)
#else
    XSIMD_BATCH_CAST_INTRINSIC(uint32_t, float, 8, _mm256_cvtepu32_ps)
#endif
    XSIMD_BATCH_CAST_INTRINSIC(uint32_t, double, 4, _mm256_cvtepu32_pd)
    XSIMD_BATCH_CAST_INTRINSIC(int64_t, int32_t, 4, _mm256_cvtepi64_epi32)
    XSIMD_BATCH_CAST_INTRINSIC(int64_t, uint32_t, 4, _mm256_cvtepi64_epi32)
    XSIMD_BATCH_CAST_INTRINSIC(uint64_t, int32_t, 4, _mm256_cvtepi64_epi32)
    XSIMD_BATCH_CAST_INTRINSIC(uint64_t, uint32_t, 4, _mm256_cvtepi64_epi32)
    XSIMD_BATCH_CAST_INTRINSIC2(float, int16_t, 8, _mm256_cvttps_epi32, _mm256_cvtepi32_epi16)
    XSIMD_BATCH_CAST_INTRINSIC2(float, uint16_t, 8, _mm256_cvttps_epi32, _mm256_cvtepi32_epi16)
    XSIMD_BATCH_CAST_INTRINSIC(float, uint32_t, 8, _mm256_cvttps_epu32)
    XSIMD_BATCH_CAST_INTRINSIC(double, uint32_t, 4, _mm256_cvttpd_epu32)
#if defined(XSIMD_AVX512DQ_AVAILABLE)
    XSIMD_BATCH_CAST_INTRINSIC(int64_t, float, 4, _mm256_cvtepi64_ps)
    XSIMD_BATCH_CAST_INTRINSIC(int64_t, double, 4, _mm256_cvtepi64_pd)
    XSIMD_BATCH_CAST_INTRINSIC(uint64_t, float, 4, _mm256_cvtepu64_ps)
    XSIMD_BATCH_CAST_INTRINSIC(uint64_t, double, 4, _mm256_cvtepu64_pd)
    XSIMD_BATCH_CAST_INTRINSIC(float, int64_t, 4, _mm256_cvttps_epi64)
    XSIMD_BATCH_CAST_INTRINSIC(float, uint64_t, 4, _mm256_cvttps_epu64)
    XSIMD_BATCH_CAST_INTRINSIC(double, int64_t, 4, _mm256_cvttpd_epi64)
    XSIMD_BATCH_CAST_INTRINSIC(double, uint64_t, 4, _mm256_cvttpd_epu64)
#endif
#endif

    /**************************
     * boolean cast functions *
     **************************/

    inline batch_bool<int32_t, 8> bool_cast(const batch_bool<float, 8>& x)
    {
        return _mm256_castps_si256(x);
    }

    inline batch_bool<int64_t, 4> bool_cast(const batch_bool<double, 4>& x)
    {
        return _mm256_castpd_si256(x);
    }

    inline batch_bool<float, 8> bool_cast(const batch_bool<int32_t, 8>& x)
    {
        return _mm256_castsi256_ps(x);
    }

    inline batch_bool<double, 4> bool_cast(const batch_bool<int64_t, 4>& x)
    {
        return _mm256_castsi256_pd(x);
    }

    /*****************************************
     * bitwise cast functions implementation *
     *****************************************/

    XSIMD_BITWISE_CAST_INTRINSIC(float, 8,
                                 double, 4,
                                 _mm256_castps_pd)

    XSIMD_BITWISE_CAST_INTRINSIC(float, 8,
                                 int32_t, 8,
                                 _mm256_castps_si256)

    XSIMD_BITWISE_CAST_INTRINSIC(float, 8,
                                 int64_t, 4,
                                 _mm256_castps_si256)

    XSIMD_BITWISE_CAST_INTRINSIC(double, 4,
                                 float, 8,
                                 _mm256_castpd_ps)

    XSIMD_BITWISE_CAST_INTRINSIC(double, 4,
                                 int32_t, 8,
                                 _mm256_castpd_si256)

    XSIMD_BITWISE_CAST_INTRINSIC(double, 4,
                                 int64_t, 4,
                                 _mm256_castpd_si256)

    XSIMD_BITWISE_CAST_INTRINSIC(int32_t, 8,
                                 float, 8,
                                 _mm256_castsi256_ps)

    XSIMD_BITWISE_CAST_INTRINSIC(int32_t, 8,
                                 double, 4,
                                 _mm256_castsi256_pd)

    XSIMD_BITWISE_CAST_INTRINSIC(int64_t, 4,
                                 float, 8,
                                 _mm256_castsi256_ps)

    XSIMD_BITWISE_CAST_INTRINSIC(int64_t, 4,
                                 double, 4,
                                 _mm256_castsi256_pd)
}

#endif
