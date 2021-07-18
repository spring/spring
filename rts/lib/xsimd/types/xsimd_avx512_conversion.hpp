/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_AVX512_CONVERSION_HPP
#define XSIMD_AVX512_CONVERSION_HPP

#include "xsimd_avx512_double.hpp"
#include "xsimd_avx512_float.hpp"
#include "xsimd_avx512_int8.hpp"
#include "xsimd_avx512_int16.hpp"
#include "xsimd_avx512_int32.hpp"
#include "xsimd_avx512_int64.hpp"

namespace xsimd
{

    /************************
     * conversion functions *
     ************************/

    batch<int32_t, 16> to_int(const batch<float, 16>& x);
    batch<int64_t, 8> to_int(const batch<double, 16>& x);

    batch<float, 16> to_float(const batch<int32_t, 16>& x);
    batch<double, 8> to_float(const batch<int64_t, 8>& x);

    /**************************
     * boolean cast functions *
     **************************/

    batch_bool<int32_t, 16> bool_cast(const batch_bool<float, 16>& x);
    batch_bool<int64_t, 8> bool_cast(const batch_bool<double, 8>& x);
    batch_bool<float, 16> bool_cast(const batch_bool<int32_t, 16>& x);
    batch_bool<double, 8> bool_cast(const batch_bool<int64_t, 8>& x);

    /*******************************
     * bitwise_cast implementation *
     *******************************/

    XSIMD_DEFINE_BITWISE_CAST_ALL(8)

    /***************************************
     * conversion functions implementation *
     ***************************************/

    inline batch<int32_t, 16> to_int(const batch<float, 16>& x)
    {
        return _mm512_cvttps_epi32(x);
    }

    inline batch<int64_t, 8> to_int(const batch<double, 8>& x)
    {
#if defined(XSIMD_AVX512DQ_AVAILABLE)
        return _mm512_cvttpd_epi64(x);
#else
        return batch<int64_t, 8>(static_cast<int64_t>(x[0]),
                                 static_cast<int64_t>(x[1]),
                                 static_cast<int64_t>(x[2]),
                                 static_cast<int64_t>(x[3]),
                                 static_cast<int64_t>(x[4]),
                                 static_cast<int64_t>(x[5]),
                                 static_cast<int64_t>(x[6]),
                                 static_cast<int64_t>(x[7]));
#endif
    }

    inline batch<float, 16> to_float(const batch<int32_t, 16>& x)
    {
        return _mm512_cvtepi32_ps(x);
    }

    inline batch<double, 8> to_float(const batch<int64_t, 8>& x)
    {
#if defined(XSIMD_AVX512DQ_AVAILABLE)
        return _mm512_cvtepi64_pd(x);
#else
        return batch<double, 8>(static_cast<double>(x[0]),
                                static_cast<double>(x[1]),
                                static_cast<double>(x[2]),
                                static_cast<double>(x[3]),
                                static_cast<double>(x[4]),
                                static_cast<double>(x[5]),
                                static_cast<double>(x[6]),
                                static_cast<double>(x[7]));
#endif
    }

    /*****************************************
     * batch cast functions implementation *
     *****************************************/

    XSIMD_BATCH_CAST_IMPLICIT(int8_t, uint8_t, 64)
    XSIMD_BATCH_CAST_IMPLICIT(uint8_t, int8_t, 64)
    XSIMD_BATCH_CAST_IMPLICIT(int16_t, uint16_t, 32)
    XSIMD_BATCH_CAST_INTRINSIC(int16_t, int32_t, 16, _mm512_cvtepi16_epi32)
    XSIMD_BATCH_CAST_INTRINSIC(int16_t, uint32_t, 16, _mm512_cvtepi16_epi32)
    XSIMD_BATCH_CAST_INTRINSIC(int16_t, int64_t, 8, _mm512_cvtepi16_epi64)
    XSIMD_BATCH_CAST_INTRINSIC(int16_t, uint64_t, 8, _mm512_cvtepi16_epi64)
    XSIMD_BATCH_CAST_INTRINSIC2(int16_t, float, 16, _mm512_cvtepi16_epi32, _mm512_cvtepi32_ps)
    XSIMD_BATCH_CAST_IMPLICIT(uint16_t, int16_t, 32)
    XSIMD_BATCH_CAST_INTRINSIC(uint16_t, int32_t, 16, _mm512_cvtepu16_epi32)
    XSIMD_BATCH_CAST_INTRINSIC(uint16_t, uint32_t, 16, _mm512_cvtepu16_epi32)
    XSIMD_BATCH_CAST_INTRINSIC(uint16_t, int64_t, 8, _mm512_cvtepu16_epi64)
    XSIMD_BATCH_CAST_INTRINSIC(uint16_t, uint64_t, 8, _mm512_cvtepu16_epi64)
    XSIMD_BATCH_CAST_INTRINSIC2(uint16_t, float, 16, _mm512_cvtepu16_epi32, _mm512_cvtepi32_ps)
    XSIMD_BATCH_CAST_INTRINSIC(int32_t, int8_t, 16, _mm512_cvtepi32_epi8)
    XSIMD_BATCH_CAST_INTRINSIC(int32_t, uint8_t, 16, _mm512_cvtepi32_epi8)
    XSIMD_BATCH_CAST_INTRINSIC(int32_t, int16_t, 16, _mm512_cvtepi32_epi16)
    XSIMD_BATCH_CAST_INTRINSIC(int32_t, uint16_t, 16, _mm512_cvtepi32_epi16)
    XSIMD_BATCH_CAST_IMPLICIT(int32_t, uint32_t, 16)
    XSIMD_BATCH_CAST_INTRINSIC(int32_t, int64_t, 8, _mm512_cvtepi32_epi64)
    XSIMD_BATCH_CAST_INTRINSIC(int32_t, uint64_t, 8, _mm512_cvtepi32_epi64)
    XSIMD_BATCH_CAST_INTRINSIC(int32_t, float, 16, _mm512_cvtepi32_ps)
    XSIMD_BATCH_CAST_INTRINSIC(int32_t, double, 8, _mm512_cvtepi32_pd)
    XSIMD_BATCH_CAST_INTRINSIC(uint32_t, int8_t, 16, _mm512_cvtepi32_epi8)
    XSIMD_BATCH_CAST_INTRINSIC(uint32_t, uint8_t, 16, _mm512_cvtepi32_epi8)
    XSIMD_BATCH_CAST_INTRINSIC(uint32_t, int16_t, 16, _mm512_cvtepi32_epi16)
    XSIMD_BATCH_CAST_INTRINSIC(uint32_t, uint16_t, 16, _mm512_cvtepi32_epi16)
    XSIMD_BATCH_CAST_IMPLICIT(uint32_t, int32_t, 16)
    XSIMD_BATCH_CAST_INTRINSIC(uint32_t, int64_t, 8, _mm512_cvtepu32_epi64)
    XSIMD_BATCH_CAST_INTRINSIC(uint32_t, uint64_t, 8, _mm512_cvtepu32_epi64)
    XSIMD_BATCH_CAST_INTRINSIC(uint32_t, float, 16, _mm512_cvtepu32_ps)
    XSIMD_BATCH_CAST_INTRINSIC(uint32_t, double, 8, _mm512_cvtepu32_pd)
    XSIMD_BATCH_CAST_INTRINSIC(int64_t, int16_t, 8, _mm512_cvtepi64_epi16)
    XSIMD_BATCH_CAST_INTRINSIC(int64_t, uint16_t, 8, _mm512_cvtepi64_epi16)
    XSIMD_BATCH_CAST_INTRINSIC(int64_t, int32_t, 8, _mm512_cvtepi64_epi32)
    XSIMD_BATCH_CAST_INTRINSIC(int64_t, uint32_t, 8, _mm512_cvtepi64_epi32)
    XSIMD_BATCH_CAST_IMPLICIT(int64_t, uint64_t, 8)
    XSIMD_BATCH_CAST_INTRINSIC(uint64_t, int16_t, 8, _mm512_cvtepi64_epi16)
    XSIMD_BATCH_CAST_INTRINSIC(uint64_t, uint16_t, 8, _mm512_cvtepi64_epi16)
    XSIMD_BATCH_CAST_INTRINSIC(uint64_t, int32_t, 8, _mm512_cvtepi64_epi32)
    XSIMD_BATCH_CAST_INTRINSIC(uint64_t, uint32_t, 8, _mm512_cvtepi64_epi32)
    XSIMD_BATCH_CAST_IMPLICIT(uint64_t, int64_t, 8)
    XSIMD_BATCH_CAST_INTRINSIC2(float, int8_t, 16, _mm512_cvttps_epi32, _mm512_cvtepi32_epi8)
    XSIMD_BATCH_CAST_INTRINSIC2(float, uint8_t, 16, _mm512_cvttps_epi32, _mm512_cvtepi32_epi8)
    XSIMD_BATCH_CAST_INTRINSIC2(float, int16_t, 16, _mm512_cvttps_epi32, _mm512_cvtepi32_epi16)
    XSIMD_BATCH_CAST_INTRINSIC2(float, uint16_t, 16, _mm512_cvttps_epi32, _mm512_cvtepi32_epi16)
    XSIMD_BATCH_CAST_INTRINSIC(float, int32_t, 16, _mm512_cvttps_epi32)
    XSIMD_BATCH_CAST_INTRINSIC(float, uint32_t, 16, _mm512_cvttps_epu32)
    XSIMD_BATCH_CAST_INTRINSIC(float, double, 8, _mm512_cvtps_pd)
    XSIMD_BATCH_CAST_INTRINSIC(double, int32_t, 8, _mm512_cvttpd_epi32)
    XSIMD_BATCH_CAST_INTRINSIC(double, uint32_t, 8, _mm512_cvttpd_epu32)
    XSIMD_BATCH_CAST_INTRINSIC(double, float, 8, _mm512_cvtpd_ps)
#if defined(XSIMD_AVX512BW_AVAILABLE)
    XSIMD_BATCH_CAST_INTRINSIC(int8_t, int16_t, 32, _mm512_cvtepi8_epi16)
    XSIMD_BATCH_CAST_INTRINSIC(int8_t, uint16_t, 32, _mm512_cvtepi8_epi16)
    XSIMD_BATCH_CAST_INTRINSIC(int8_t, int32_t, 16, _mm512_cvtepi8_epi32)
    XSIMD_BATCH_CAST_INTRINSIC(int8_t, uint32_t, 16, _mm512_cvtepi8_epi32)
    XSIMD_BATCH_CAST_INTRINSIC2(int8_t, float, 16, _mm512_cvtepi8_epi32, _mm512_cvtepi32_ps)
    XSIMD_BATCH_CAST_INTRINSIC(uint8_t, int16_t, 32, _mm512_cvtepu8_epi16)
    XSIMD_BATCH_CAST_INTRINSIC(uint8_t, uint16_t, 32, _mm512_cvtepu8_epi16)
    XSIMD_BATCH_CAST_INTRINSIC(uint8_t, int32_t, 16, _mm512_cvtepu8_epi32)
    XSIMD_BATCH_CAST_INTRINSIC(uint8_t, uint32_t, 16, _mm512_cvtepu8_epi32)
    XSIMD_BATCH_CAST_INTRINSIC2(uint8_t, float, 16, _mm512_cvtepu8_epi32, _mm512_cvtepi32_ps)
    XSIMD_BATCH_CAST_INTRINSIC(int16_t, int8_t, 32, _mm512_cvtepi16_epi8)
    XSIMD_BATCH_CAST_INTRINSIC(int16_t, uint8_t, 32, _mm512_cvtepi16_epi8)
    XSIMD_BATCH_CAST_INTRINSIC(uint16_t, int8_t, 32, _mm512_cvtepi16_epi8)
    XSIMD_BATCH_CAST_INTRINSIC(uint16_t, uint8_t, 32, _mm512_cvtepi16_epi8)
#endif
#if defined(XSIMD_AVX512DQ_AVAILABLE)
    XSIMD_BATCH_CAST_INTRINSIC2(int16_t, double, 8, _mm512_cvtepi16_epi64, _mm512_cvtepi64_pd)
    XSIMD_BATCH_CAST_INTRINSIC2(uint16_t, double, 8, _mm512_cvtepu16_epi64, _mm512_cvtepi64_pd)
    XSIMD_BATCH_CAST_INTRINSIC(int64_t, float, 8, _mm512_cvtepi64_ps)
    XSIMD_BATCH_CAST_INTRINSIC(int64_t, double, 8, _mm512_cvtepi64_pd)
    XSIMD_BATCH_CAST_INTRINSIC(uint64_t, float, 8, _mm512_cvtepu64_ps)
    XSIMD_BATCH_CAST_INTRINSIC(uint64_t, double, 8, _mm512_cvtepu64_pd)
    XSIMD_BATCH_CAST_INTRINSIC(float, int64_t, 8, _mm512_cvttps_epi64)
    XSIMD_BATCH_CAST_INTRINSIC(float, uint64_t, 8, _mm512_cvttps_epu64)
    XSIMD_BATCH_CAST_INTRINSIC2(double, int16_t, 8, _mm512_cvttpd_epi64, _mm512_cvtepi64_epi16)
    XSIMD_BATCH_CAST_INTRINSIC2(double, uint16_t, 8, _mm512_cvttpd_epi64, _mm512_cvtepi64_epi16)
    XSIMD_BATCH_CAST_INTRINSIC(double, int64_t, 8, _mm512_cvttpd_epi64)
    XSIMD_BATCH_CAST_INTRINSIC(double, uint64_t, 8, _mm512_cvttpd_epu64)
#endif

    /**************************
     * boolean cast functions *
     **************************/

    inline batch_bool<int32_t, 16> bool_cast(const batch_bool<float, 16>& x)
    {
        return __mmask16(x);
    }

    inline batch_bool<int64_t, 8> bool_cast(const batch_bool<double, 8>& x)
    {
        return __mmask8(x);
    }

    inline batch_bool<float, 16> bool_cast(const batch_bool<int32_t, 16>& x)
    {
        return __mmask16(x);
    }

    inline batch_bool<double, 8> bool_cast(const batch_bool<int64_t, 8>& x)
    {
        return __mmask8(x);
    }

    /*****************************************
     * bitwise cast functions implementation *
     *****************************************/

    XSIMD_BITWISE_CAST_INTRINSIC(float, 16,
                                 double, 8,
                                 _mm512_castps_pd)

    XSIMD_BITWISE_CAST_INTRINSIC(float, 16,
                                 int32_t, 16,
                                 _mm512_castps_si512)

    XSIMD_BITWISE_CAST_INTRINSIC(float, 16,
                                 int64_t, 8,
                                 _mm512_castps_si512)

    XSIMD_BITWISE_CAST_INTRINSIC(double, 8,
                                 float, 16,
                                 _mm512_castpd_ps)

    XSIMD_BITWISE_CAST_INTRINSIC(double, 8,
                                 int32_t, 16,
                                 _mm512_castpd_si512)

    XSIMD_BITWISE_CAST_INTRINSIC(double, 8,
                                 int64_t, 8,
                                 _mm512_castpd_si512)

    XSIMD_BITWISE_CAST_INTRINSIC(int32_t, 16,
                                 float, 16,
                                 _mm512_castsi512_ps)

    XSIMD_BITWISE_CAST_INTRINSIC(int32_t, 16,
                                 double, 8,
                                 _mm512_castsi512_pd)

    XSIMD_BITWISE_CAST_INTRINSIC(int64_t, 8,
                                 float, 16,
                                 _mm512_castsi512_ps)

    XSIMD_BITWISE_CAST_INTRINSIC(int64_t, 8,
                                 double, 8,
                                 _mm512_castsi512_pd)
}

#endif
