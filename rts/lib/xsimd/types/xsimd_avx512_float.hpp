/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_AVX512_FLOAT_HPP
#define XSIMD_AVX512_FLOAT_HPP

#include <cstdint>

#include "xsimd_avx512_bool.hpp"
#include "xsimd_base.hpp"

namespace xsimd
{

    /*************************
     * batch_bool<float, 16> *
     *************************/

    template <>
    struct simd_batch_traits<batch_bool<float, 16>>
    {
        using value_type = float;
        static constexpr std::size_t size = 16;
        using batch_type = batch<float, 16>;
        static constexpr std::size_t align = 0;
    };

    template <>
    class batch_bool<float, 16> : public batch_bool_avx512<__mmask16, batch_bool<float, 16>>
    {
    public:
        using base_class = batch_bool_avx512<__mmask16, batch_bool<float, 16>>;
        using base_class::base_class;
    };

    namespace detail
    {
        template <>
        struct batch_bool_kernel<float, 16>
            : batch_bool_kernel_avx512<float, 16>
        {
        };
    }

    /*********************
     * batch<float, 16> *
     *********************/

    template <>
    struct simd_batch_traits<batch<float, 16>>
    {
        using value_type = float;
        using batch_bool_type = batch_bool<float, 16>;
        static constexpr std::size_t size = 16;
        static constexpr std::size_t align = 64;
        using storage_type = __m512;
    };

    template <>
    class batch<float, 16> : public simd_batch<batch<float, 16>>
    {
    public:

        using self_type = batch<float, 16>;
        using base_type = simd_batch<self_type>;

        batch();
        explicit batch(float i);
        batch(float i0, float i1,  float i2,  float i3,  float i4,  float i5,  float i6,  float i7,
              float i8, float i9, float i10, float i11, float i12, float i13, float i14, float i15);
        explicit batch(const float* src);

        batch(const float* src, aligned_mode);
        batch(const float* src, unaligned_mode);
        
        batch(const __m512& rhs);
        batch& operator=(const __m512& rhs);

        batch(const batch_bool<float, 16>& rhs);
        batch& operator=(const batch_bool<float, 16>& rhs);

        operator __m512() const;

        XSIMD_DECLARE_LOAD_STORE_ALL(float, 16)
        XSIMD_DECLARE_LOAD_STORE_LONG(float, 16)

        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;
    };

    /************************************
     * batch<float, 16> implementation *
     ************************************/

    inline batch<float, 16>::batch()
    {
    }

    inline batch<float, 16>::batch(float i)
        : base_type(_mm512_set1_ps(i))
    {
    }

    inline batch<float, 16>::batch(float i0, float i1,  float i2,  float i3,  float i4,  float i5,  float i6,  float i7,
                                   float i8, float i9, float i10, float i11, float i12, float i13, float i14, float i15)
        : base_type(_mm512_setr_ps(i0, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, i11, i12, i13, i14, i15))
    {
    }

    inline batch<float, 16>::batch(const float* src)
        : base_type(_mm512_loadu_ps(src))
    {
    }

    inline batch<float, 16>::batch(const float* src, aligned_mode)
        : base_type(_mm512_load_ps(src))
    {
    }

    inline batch<float, 16>::batch(const float* src, unaligned_mode)
        : base_type(_mm512_loadu_ps(src))
    {
    }

    inline batch<float, 16>::batch(const __m512& rhs)
        : base_type(rhs)
    {
    }

    inline batch<float, 16>& batch<float, 16>::operator=(const __m512& rhs)
    {
        this->m_value = rhs;
        return *this;
    }

    inline batch<float, 16>::operator __m512() const
    {
        return this->m_value;
    }

    XSIMD_DEFINE_LOAD_STORE(float, 16, bool, 64)

    inline batch<float, 16>& batch<float, 16>::load_aligned(const int8_t* src)
    {
        __m128i tmp = _mm_load_si128((const __m128i*)src);
        __m512i tmp2 = _mm512_cvtepi8_epi32(tmp);
        this->m_value = _mm512_cvtepi32_ps(tmp2);
        return *this;
    }

    inline batch<float, 16>& batch<float, 16>::load_unaligned(const int8_t* src)
    {
        __m128i tmp = _mm_loadu_si128((const __m128i*)src);
        __m512i tmp2 = _mm512_cvtepi8_epi32(tmp);
        this->m_value = _mm512_cvtepi32_ps(tmp2);
        return *this;
    }

    inline batch<float, 16>& batch<float, 16>::load_aligned(const uint8_t* src)
    {
        __m128i tmp = _mm_load_si128((const __m128i*)src);
        __m512i tmp2 = _mm512_cvtepu8_epi32(tmp);
        this->m_value = _mm512_cvtepi32_ps(tmp2);
        return *this;
    }

    inline batch<float, 16>& batch<float, 16>::load_unaligned(const uint8_t* src)
    {
        __m128i tmp = _mm_loadu_si128((const __m128i*)src);
        __m512i tmp2 = _mm512_cvtepu8_epi32(tmp);
        this->m_value = _mm512_cvtepi32_ps(tmp2);
        return *this;
    }

    inline batch<float, 16>& batch<float, 16>::load_aligned(const int16_t* src)
    {
        __m256i tmp = _mm256_load_si256((const __m256i*)src);
        __m512i tmp2 = _mm512_cvtepi16_epi32(tmp);
        this->m_value = _mm512_cvtepi32_ps(tmp2);
        return *this;
    }

    inline batch<float, 16>& batch<float, 16>::load_unaligned(const int16_t* src)
    {
        __m256i tmp = _mm256_loadu_si256((const __m256i*)src);
        __m512i tmp2 = _mm512_cvtepi16_epi32(tmp);
        this->m_value = _mm512_cvtepi32_ps(tmp2);
        return *this;
    }

    inline batch<float, 16>& batch<float, 16>::load_aligned(const uint16_t* src)
    {
        __m256i tmp = _mm256_load_si256((const __m256i*)src);
        __m512i tmp2 = _mm512_cvtepu16_epi32(tmp);
        this->m_value = _mm512_cvtepi32_ps(tmp2);
        return *this;
    }

    inline batch<float, 16>& batch<float, 16>::load_unaligned(const uint16_t* src)
    {
        __m256i tmp = _mm256_loadu_si256((const __m256i*)src);
        __m512i tmp2 = _mm512_cvtepu16_epi32(tmp);
        this->m_value = _mm512_cvtepi32_ps(tmp2);
        return *this;
    }

    inline batch<float, 16>& batch<float, 16>::load_aligned(const int32_t* src)
    {
        // TODO select correct rounding direction?
        this->m_value = _mm512_cvt_roundepi32_ps(_mm512_load_si512(src), _MM_FROUND_CUR_DIRECTION);
        return *this;
    }

    inline batch<float, 16>& batch<float, 16>::load_unaligned(const int32_t* src)
    {
        this->m_value = _mm512_cvt_roundepi32_ps(_mm512_loadu_si512(src), _MM_FROUND_CUR_DIRECTION);
        return *this;
    }

    inline batch<float, 16>& batch<float, 16>::load_aligned(const uint32_t* src)
    {
        // TODO select correct rounding direction?
        this->m_value = _mm512_cvt_roundepu32_ps(_mm512_load_si512(src), _MM_FROUND_CUR_DIRECTION);
        return *this;
    }

    inline batch<float, 16>& batch<float, 16>::load_unaligned(const uint32_t* src)
    {
        this->m_value = _mm512_cvt_roundepu32_ps(_mm512_loadu_si512(src), _MM_FROUND_CUR_DIRECTION);
        return *this;
    }

    XSIMD_DEFINE_LOAD_STORE(float, 16, int64_t, 64)
    XSIMD_DEFINE_LOAD_STORE(float, 16, uint64_t, 64)
    XSIMD_DEFINE_LOAD_STORE_LONG(float, 16, 64)

    inline batch<float, 16>& batch<float, 16>::load_aligned(const float* src)
    {
        this->m_value = _mm512_load_ps(src);
        return *this;
    }

    inline batch<float, 16>& batch<float, 16>::load_unaligned(const float* src)
    {
        this->m_value = _mm512_loadu_ps(src);
        return *this;
    }

    inline batch<float, 16>& batch<float, 16>::load_aligned(const double* src)
    {
        __m256 tmp1 = _mm512_cvtpd_ps(_mm512_load_pd(src));
        __m256 tmp2 = _mm512_cvtpd_ps(_mm512_load_pd(src + 8));
        this->m_value = _mm512_castps256_ps512(tmp1);
        this->m_value = _mm512_insertf32x8(this->m_value, tmp2, 1);
        return *this;
    }

    inline batch<float, 16>& batch<float, 16>::load_unaligned(const double* src)
    {
        __m256 tmp1 = _mm512_cvtpd_ps(_mm512_loadu_pd(src));
        __m256 tmp2 = _mm512_cvtpd_ps(_mm512_loadu_pd(src + 8));
        this->m_value = _mm512_castps256_ps512(tmp1);
        this->m_value = _mm512_insertf32x8(this->m_value, tmp2, 1);
        return *this;
    }

    inline void batch<float, 16>::store_aligned(int8_t* dst) const
    {
        __m512i tmp = _mm512_cvtps_epi32(this->m_value);
        __m128i tmp2 = _mm512_cvtepi32_epi8(tmp);
        _mm_store_si128((__m128i*)dst, tmp2);
    }

    inline void batch<float, 16>::store_unaligned(int8_t* dst) const
    {
        __m512i tmp = _mm512_cvtps_epi32(this->m_value);
        __m128i tmp2 = _mm512_cvtepi32_epi8(tmp);
        _mm_storeu_si128((__m128i*)dst, tmp2);
    }

    inline void batch<float, 16>::store_aligned(uint8_t* dst) const
    {
        __m512i tmp = _mm512_cvtps_epu32(this->m_value);
        __m128i tmp2 = _mm512_cvtusepi32_epi8(tmp);
        _mm_store_si128((__m128i*)dst, tmp2);
    }

    inline void batch<float, 16>::store_unaligned(uint8_t* dst) const
    {
        __m512i tmp = _mm512_cvtps_epu32(this->m_value);
        __m128i tmp2 = _mm512_cvtusepi32_epi8(tmp);
        _mm_storeu_si128((__m128i*)dst, tmp2);
    }

    inline void batch<float, 16>::store_aligned(int16_t* dst) const
    {
        __m512i tmp = _mm512_cvtps_epi32(this->m_value);
        __m256i tmp2 = _mm512_cvtepi32_epi16(tmp);
        _mm256_store_si256((__m256i*)dst, tmp2);
    }

    inline void batch<float, 16>::store_unaligned(int16_t* dst) const
    {
        __m512i tmp = _mm512_cvtps_epi32(this->m_value);
        __m256i tmp2 = _mm512_cvtepi32_epi16(tmp);
        _mm256_storeu_si256((__m256i*)dst, tmp2);
    }

    inline void batch<float, 16>::store_aligned(uint16_t* dst) const
    {
        __m512i tmp = _mm512_cvtps_epu32(this->m_value);
        __m256i tmp2 = _mm512_cvtusepi32_epi16(tmp);
        _mm256_store_si256((__m256i*)dst, tmp2);
    }

    inline void batch<float, 16>::store_unaligned(uint16_t* dst) const
    {
        __m512i tmp = _mm512_cvtps_epu32(this->m_value);
        __m256i tmp2 = _mm512_cvtusepi32_epi16(tmp);
        _mm256_storeu_si256((__m256i*)dst, tmp2);
    }

    inline void batch<float, 16>::store_aligned(int32_t* dst) const
    {
        _mm512_store_si512((__m512i *)dst, _mm512_cvtps_epi32(this->m_value));
    }

    inline void batch<float, 16>::store_unaligned(int32_t* dst) const
    {
        _mm512_storeu_si512((__m512i *)dst, _mm512_cvtps_epi32(this->m_value));
    }

    inline void batch<float, 16>::store_aligned(uint32_t* dst) const
    {
        _mm512_store_si512((__m512i *)dst, _mm512_cvtps_epu32(this->m_value));
    }

    inline void batch<float, 16>::store_unaligned(uint32_t* dst) const
    {
        _mm512_storeu_si512((__m512i *)dst, _mm512_cvtps_epu32(this->m_value));
    }

    inline void batch<float, 16>::store_aligned(float* dst) const
    {
        _mm512_store_ps(dst, this->m_value);
    }

    inline void batch<float, 16>::store_unaligned(float* dst) const
    {
        _mm512_storeu_ps(dst, this->m_value);
    }

    inline void batch<float, 16>::store_aligned(double* dst) const
    {
        _mm512_store_pd(dst, _mm512_cvtps_pd(_mm512_extractf32x8_ps(this->m_value, 0)));
        _mm512_store_pd(dst + 8, _mm512_cvtps_pd(_mm512_extractf32x8_ps(this->m_value, 1)));
    }

    inline void batch<float, 16>::store_unaligned(double* dst) const
    {
        _mm512_storeu_pd(dst, _mm512_cvtps_pd(_mm512_extractf32x8_ps(this->m_value, 0)));
        _mm512_storeu_pd(dst + 8, _mm512_cvtps_pd(_mm512_extractf32x8_ps(this->m_value, 1)));
    }

    namespace detail
    {
        template <>
        struct batch_kernel<float, 16>
        {
            using batch_type = batch<float, 16>;
            using value_type = float;
            using batch_bool_type = batch_bool<float, 16>;

            static batch_type neg(const batch_type& rhs)
            {
                return _mm512_sub_ps(_mm512_setzero_ps(), rhs);
            }

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_add_ps(lhs, rhs);
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_sub_ps(lhs, rhs);
            }

            static batch_type sadd(const batch_type& lhs, const batch_type& rhs)
            {
                return add(lhs, rhs); //do something for inf ?
            }

            static batch_type ssub(const batch_type& lhs, const batch_type& rhs)
            {
                return sub(lhs, rhs); //do something for inf ?
            }

            static batch_type mul(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_mul_ps(lhs, rhs);
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_div_ps(lhs, rhs);
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_cmp_ps_mask(lhs, rhs, _CMP_EQ_OQ);
            }

            static batch_bool_type neq(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_cmp_ps_mask(lhs, rhs, _CMP_NEQ_OQ);
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_cmp_ps_mask(lhs, rhs, _CMP_LT_OQ);
            }

            static batch_bool_type lte(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_cmp_ps_mask(lhs, rhs, _CMP_LE_OQ);
            }

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_and_ps(lhs, rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_or_ps(lhs, rhs);
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_xor_ps(lhs, rhs);
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return _mm512_xor_ps(rhs, _mm512_castsi512_ps(_mm512_set1_epi32(-1)));
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_andnot_ps(lhs, rhs);
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_min_ps(lhs, rhs);
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_max_ps(lhs, rhs);
            }

            static batch_type fmin(const batch_type& lhs, const batch_type& rhs)
            {
                return min(lhs, rhs);
            }

            static batch_type fmax(const batch_type& lhs, const batch_type& rhs)
            {
                return max(lhs, rhs);
            }

            static batch_type abs(const batch_type& rhs)
            {
                __m512 rhs_asf = (__m512)rhs;
                __m512i rhs_asi = *reinterpret_cast<__m512i*>(&rhs_asf);
                __m512i res_asi = _mm512_and_epi32(_mm512_set1_epi32(0x7FFFFFFF),
                                                   rhs_asi);
                return *reinterpret_cast<__m512*>(&res_asi);
            }

            static batch_type fabs(const batch_type& rhs)
            {
                return abs(rhs);
            }

            static batch_type sqrt(const batch_type& rhs)
            {
                return _mm512_sqrt_ps(rhs);
            }

            static batch_type fma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return _mm512_fmadd_ps(x, y, z);
            }

            static batch_type fms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return _mm512_fmsub_ps(x, y, z);
            }

            static batch_type fnma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return _mm512_fnmadd_ps(x, y, z);
            }

            static batch_type fnms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return _mm512_fnmsub_ps(x, y, z);
            }

            static value_type hadd(const batch_type& rhs)
            {
                __m256 tmp1 = _mm512_extractf32x8_ps(rhs, 1);
                __m256 tmp2 = _mm512_extractf32x8_ps(rhs, 0);
                __m256 res1 = _mm256_add_ps(tmp1, tmp2);
                return xsimd::hadd(batch<float, 8>(res1));
            }

            static batch_type haddp(const batch_type* row)
            {
                // The following folds over the vector once:
                // tmp1 = [a0..8, b0..8]
                // tmp2 = [a8..f, b8..f]
#define XSIMD_AVX512_HADDP_STEP1(I, a, b)                                                  \
        batch<float, 16> res ## I;                                                             \
        {                                                                                      \
            auto tmp1 = _mm512_shuffle_f32x4(a, b, _MM_SHUFFLE(1, 0, 1, 0));                   \
            auto tmp2 = _mm512_shuffle_f32x4(a, b, _MM_SHUFFLE(3, 2, 3, 2));                   \
            res ## I = _mm512_add_ps(tmp1, tmp2);                                              \
        }                                                                                      \

                XSIMD_AVX512_HADDP_STEP1(0, row[0], row[2]);
                XSIMD_AVX512_HADDP_STEP1(1, row[4], row[6]);
                XSIMD_AVX512_HADDP_STEP1(2, row[1], row[3]);
                XSIMD_AVX512_HADDP_STEP1(3, row[5], row[7]);
                XSIMD_AVX512_HADDP_STEP1(4, row[8], row[10]);
                XSIMD_AVX512_HADDP_STEP1(5, row[12], row[14]);
                XSIMD_AVX512_HADDP_STEP1(6, row[9], row[11]);
                XSIMD_AVX512_HADDP_STEP1(7, row[13], row[15]);

#undef XSIMD_AVX512_HADDP_STEP1

                // The following flds the code and shuffles so that hadd_ps produces the correct result
                // tmp1 = [a0..4,  a8..12,  b0..4,  b8..12] (same for tmp3)
                // tmp2 = [a5..8, a12..16, b5..8, b12..16]  (same for tmp4)
                // tmp5 = [r1[0], r1[2], r2[0], r2[2], r1[4], r1[6] ...
#define XSIMD_AVX512_HADDP_STEP2(I, a, b, c, d)                                             \
        batch<float, 8> halfx ## I;                                                             \
        {                                                                                       \
            batch<float, 16> tmp1 = _mm512_shuffle_f32x4(a, b, _MM_SHUFFLE(2, 0, 2, 0));        \
            batch<float, 16> tmp2 = _mm512_shuffle_f32x4(a, b, _MM_SHUFFLE(3, 1, 3, 1));        \
                                                                                                \
            batch<float, 16> resx1 = _mm512_add_ps(tmp1, tmp2);                                               \
                                                                                                \
            batch<float, 16> tmp3 = _mm512_shuffle_f32x4(c, d, _MM_SHUFFLE(2, 0, 2, 0));        \
            batch<float, 16> tmp4 = _mm512_shuffle_f32x4(c, d, _MM_SHUFFLE(3, 1, 3, 1));        \
                                                                                                \
            batch<float, 16> resx2 = _mm512_add_ps(tmp3, tmp4);                                               \
                                                                                                \
            batch<float, 16> tmp5 = _mm512_shuffle_ps(resx1, resx2, _MM_SHUFFLE(2, 0, 2, 0));   \
            batch<float, 16> tmp6 = _mm512_shuffle_ps(resx1, resx2, _MM_SHUFFLE(3, 1, 3, 1));   \
                                                                                                \
            batch<float, 16> resx3 = _mm512_add_ps(tmp5, tmp6);                                               \
                                                                                                \
            halfx ## I  = _mm256_hadd_ps(_mm512_extractf32x8_ps(resx3, 0),                      \
                                         _mm512_extractf32x8_ps(resx3, 1));                     \
        }                                                                                       \

                XSIMD_AVX512_HADDP_STEP2(0, res0, res1, res2, res3);
                XSIMD_AVX512_HADDP_STEP2(1, res4, res5, res6, res7);

#undef XSIMD_AVX512_HADDP_STEP2

                auto concat = _mm512_castps256_ps512(halfx0);
                concat = _mm512_insertf32x8(concat, halfx1, 1);
                return concat;
            }

            static batch_type select(const batch_bool_type& cond, const batch_type& a, const batch_type& b)
            {
            #if !defined(_MSC_VER)
                return _mm512_mask_blend_ps(cond, b, a);
            #else
                __m512i mcondi = _mm512_maskz_broadcastd_epi32 ((__mmask16)cond, _mm_set1_epi32(~0));
                __m512 mcond = *reinterpret_cast<__m512*>(&mcondi);
                XSIMD_SPLITPS_AVX512(mcond);
                XSIMD_SPLITPS_AVX512(a);
                XSIMD_SPLITPS_AVX512(b);

                auto res_lo = _mm256_blendv_ps(b_low, a_low, mcond_low);
                auto res_hi = _mm256_blendv_ps(b_high, a_high, mcond_high);

                XSIMD_RETURN_MERGEDPS_AVX(res_lo, res_hi);
            #endif
            }

            static batch_type zip_lo(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_unpacklo_ps(lhs, rhs);
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_unpackhi_ps(lhs, rhs);
            }

            static batch_bool_type isnan(const batch_type& x)
            {
                return _mm512_cmp_ps_mask(x, x, _CMP_UNORD_Q);
            }
        };
    }

    inline batch<float, 16>::batch(const batch_bool<float, 16>& rhs)
        : base_type(detail::batch_kernel<float, 16>::select(rhs, batch(float(1)), batch(float(0))))
    {
    }

    inline batch<float, 16>& batch<float, 16>::operator=(const batch_bool<float, 16>& rhs)
    {
        this->m_value = detail::batch_kernel<float, 16>::select(rhs, batch(float(1)), batch(float(0)));
        return *this;
    }

}

#endif
