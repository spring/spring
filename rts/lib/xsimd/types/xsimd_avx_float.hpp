/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_AVX_FLOAT_HPP
#define XSIMD_AVX_FLOAT_HPP

#include "xsimd_base.hpp"
#include "xsimd_int_conversion.hpp"
#include <array>

namespace xsimd
{

    /************************
     * batch_bool<float, 8> *
     ************************/

    template <>
    struct simd_batch_traits<batch_bool<float, 8>>
    {
        using value_type = float;
        static constexpr std::size_t size = 8;
        using batch_type = batch<float, 8>;
        static constexpr std::size_t align = 32;
    };

    template <>
    class batch_bool<float, 8> : public simd_batch_bool<batch_bool<float, 8>>
    {
    public:

        batch_bool();
        explicit batch_bool(bool b);
        batch_bool(bool b0, bool b1, bool b2, bool b3,
                   bool b4, bool b5, bool b6, bool b7);
        batch_bool(const __m256& rhs);
        batch_bool& operator=(const __m256& rhs);

        operator __m256() const;

        bool_proxy<float> operator[](std::size_t index);
        bool operator[](std::size_t index) const;

        __m256 get_value() const;

    private:

        batch_bool<float, 8>& load_values(bool b0, bool b1, bool b2, bool b3,
                                          bool b4, bool b5, bool b6, bool b7);

        union
        {
            __m256 m_value;
            float m_array[8];
        };

        friend class simd_batch_bool<batch_bool<float, 8>>;
    };

    /*******************
     * batch<float, 8> *
     *******************/

    template <>
    struct simd_batch_traits<batch<float, 8>>
    {
        using value_type = float;
        static constexpr std::size_t size = 8;
        using batch_bool_type = batch_bool<float, 8>;
        static constexpr std::size_t align = 32;
        using storage_type = __m256;
    };

    template <>
    class batch<float, 8> : public simd_batch<batch<float, 8>>
    {
    public:

        using self_type = batch<float, 8>;
        using base_type = simd_batch<self_type>;
        using batch_bool_type = typename base_type::batch_bool_type;

        batch();
        explicit batch(float f);
        batch(float f0, float f1, float f2, float f3,
              float f4, float f5, float f6, float f7);
        explicit batch(const float* src);

        batch(const float* src, aligned_mode);
        batch(const float* src, unaligned_mode);
        
        batch(const __m256& rhs);
        batch& operator=(const __m256& rhs);

        batch(const batch_bool_type& rhs);
        batch& operator=(const batch_bool_type& rhs);
        
        operator __m256() const;

        XSIMD_DECLARE_LOAD_STORE_ALL(float, 8)
        XSIMD_DECLARE_LOAD_STORE_LONG(float, 8)

        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;
    };

    /***************************************
     * batch_bool<float, 8> implementation *
     ***************************************/

    inline batch_bool<float, 8>::batch_bool()
    {
    }

    inline batch_bool<float, 8>::batch_bool(bool b)
    {
        m_value = _mm256_castsi256_ps(_mm256_set1_epi32(-(int)b));
    }

    inline batch_bool<float, 8>::batch_bool(bool b0, bool b1, bool b2, bool b3,
                                            bool b4, bool b5, bool b6, bool b7)
    {
        m_value = _mm256_castsi256_ps(
              _mm256_setr_epi32(-(int)b0, -(int)b1, -(int)b2, -(int)b3,
                                -(int)b4, -(int)b5, -(int)b6, -(int)b7));
    }

    inline batch_bool<float, 8>::batch_bool(const __m256& rhs)
    {
        m_value = rhs;
    }

    inline batch_bool<float, 8>& batch_bool<float, 8>::operator=(const __m256& rhs)
    {
        m_value = rhs;
        return *this;
    }

    inline batch<float, 8>::batch(const batch_bool_type& rhs)
        : base_type(_mm256_and_ps(rhs, batch(1.f)))
    {
    }

    inline batch<float, 8>& batch<float, 8>::operator=(const batch_bool_type& rhs)
    {
        this->m_value = _mm256_and_ps(rhs, batch(1.f));
        return *this;
    }

    inline batch_bool<float, 8>::operator __m256() const
    {
        return m_value;
    }

    inline bool_proxy<float> batch_bool<float, 8>::operator[](std::size_t index)
    {
        return bool_proxy<float>(m_array[index & 7]);
    }

    inline bool batch_bool<float, 8>::operator[](std::size_t index) const
    {
        return static_cast<bool>(m_array[index & 7]);
    }

    inline __m256 batch_bool<float, 8>::get_value() const
    {
        return m_value;
    }

    inline batch_bool<float, 8>& batch_bool<float, 8>::load_values(bool b0, bool b1, bool b2, bool b3,
                                                                   bool b4, bool b5, bool b6, bool b7)
    {
        m_value = _mm256_castsi256_ps(
              _mm256_setr_epi32(-(int)b0, -(int)b1, -(int)b2, -(int)b3,
                                -(int)b4, -(int)b5, -(int)b6, -(int)b7));
        return *this;
    }

    namespace detail
    {
        template <>
        struct batch_bool_kernel<float, 8>
        {
            using batch_type = batch_bool<float, 8>;

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_and_ps(lhs, rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_or_ps(lhs, rhs);
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_xor_ps(lhs, rhs);
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return _mm256_xor_ps(rhs, _mm256_castsi256_ps(_mm256_set1_epi32(-1)));
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_andnot_ps(lhs, rhs);
            }

            static batch_type equal(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_castsi256_ps(_mm256_cmpeq_epi32(_mm256_castps_si256(lhs), _mm256_castps_si256(rhs)));
#else
                __m128i lhs_low = _mm256_castsi256_si128(_mm256_castps_si256(lhs));
                __m128i lhs_high = _mm256_extractf128_si256(_mm256_castps_si256(lhs), 1);
                __m128i rhs_low = _mm256_castsi256_si128(_mm256_castps_si256(rhs));
                __m128i rhs_high = _mm256_extractf128_si256(_mm256_castps_si256(rhs), 1);
                __m128i res_low = _mm_cmpeq_epi32(lhs_low, rhs_low);
                __m128i res_high = _mm_cmpeq_epi32(lhs_high, rhs_high);
                __m256i result = _mm256_castsi128_si256(res_low);
                return _mm256_castsi256_ps(_mm256_insertf128_si256(result, res_high, 1));
#endif
            }

            static batch_type not_equal(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_xor_ps(lhs, rhs);
            }

            static bool all(const batch_type& rhs)
            {
                return _mm256_testc_ps(rhs, batch_bool<float, 8>(true)) != 0;
            }

            static bool any(const batch_type& rhs)
            {
                return !_mm256_testz_ps(rhs, rhs);
            }
        };
    }

    /**********************************
     * batch<float, 8> implementation *
     **********************************/

    inline batch<float, 8>::batch()
    {
    }

    inline batch<float, 8>::batch(float f)
        : base_type(_mm256_set1_ps(f))
    {
    }

    inline batch<float, 8>::batch(float f0, float f1, float f2, float f3,
                                  float f4, float f5, float f6, float f7)
        : base_type(_mm256_setr_ps(f0, f1, f2, f3, f4, f5, f6, f7))
    {
    }

    inline batch<float, 8>::batch(const float* src)
        : base_type(_mm256_loadu_ps(src))
    {
    }

    inline batch<float, 8>::batch(const float* src, aligned_mode)
        : base_type(_mm256_load_ps(src))
    {
    }

    inline batch<float, 8>::batch(const float* src, unaligned_mode)
        : base_type(_mm256_loadu_ps(src))
    {
    }

    inline batch<float, 8>::batch(const __m256& rhs)
        : base_type(rhs)
    {
    }

    inline batch<float, 8>& batch<float, 8>::operator=(const __m256& rhs)
    {
        this->m_value = rhs;
        return *this;
    }

    inline batch<float, 8>::operator __m256() const
    {
        return this->m_value;
    }

    XSIMD_DEFINE_LOAD_STORE(float, 8, bool, 32)

    inline batch<float, 8>& batch<float, 8>::load_aligned(const int8_t* src)
    {
        __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
        __m256i tmp2 = detail::xsimd_cvtepi8_epi32(tmp);
        this->m_value = _mm256_cvtepi32_ps(tmp2);
        return *this;
    }

    inline batch<float, 8>& batch<float, 8>::load_unaligned(const int8_t* src)
    {
        return load_aligned(src);
    }

    inline batch<float, 8>& batch<float, 8>::load_aligned(const uint8_t* src)
    {
        __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
        __m256i tmp2 = detail::xsimd_cvtepu8_epi32(tmp);
        this->m_value = _mm256_cvtepi32_ps(tmp2);
        return *this;
    }

    inline batch<float, 8>& batch<float, 8>::load_unaligned(const uint8_t* src)
    {
        return load_aligned(src);
    }

    inline batch<float, 8>& batch<float, 8>::load_aligned(const int16_t* src)
    {
        __m128i tmp = _mm_load_si128((const __m128i*)src);
        __m256i tmp2 = detail::xsimd_cvtepi16_epi32(tmp);
        this->m_value = _mm256_cvtepi32_ps(tmp2);
        return *this;
    }

    inline batch<float, 8>& batch<float, 8>::load_unaligned(const int16_t* src)
    {
        __m128i tmp = _mm_loadu_si128((const __m128i*)src);
        __m256i tmp2 = detail::xsimd_cvtepi16_epi32(tmp);
        this->m_value = _mm256_cvtepi32_ps(tmp2);
        return *this;
    }

    inline batch<float, 8>& batch<float, 8>::load_aligned(const uint16_t* src)
    {
        __m128i tmp = _mm_load_si128((const __m128i*)src);
        __m256i tmp2 = detail::xsimd_cvtepu16_epi32(tmp);
        this->m_value = _mm256_cvtepi32_ps(tmp2);
        return *this;
    }

    inline batch<float, 8>& batch<float, 8>::load_unaligned(const uint16_t* src)
    {
        __m128i tmp = _mm_loadu_si128((const __m128i*)src);
        __m256i tmp2 = detail::xsimd_cvtepu16_epi32(tmp);
        this->m_value = _mm256_cvtepi32_ps(tmp2);
        return *this;
    }

    inline batch<float, 8>& batch<float, 8>::load_aligned(const int32_t* src)
    {
        this->m_value = _mm256_cvtepi32_ps(_mm256_load_si256((__m256i const*)src));
        return *this;
    }

    inline batch<float, 8>& batch<float, 8>::load_unaligned(const int32_t* src)
    {
        this->m_value = _mm256_cvtepi32_ps(_mm256_loadu_si256((__m256i const*)src));
        return *this;
    }

    XSIMD_DEFINE_LOAD_STORE(float, 8, uint32_t, 32)
    XSIMD_DEFINE_LOAD_STORE(float, 8, int64_t, 32)
    XSIMD_DEFINE_LOAD_STORE(float, 8, uint64_t, 32)
    XSIMD_DEFINE_LOAD_STORE_LONG(float, 8, 32)

    inline batch<float, 8>& batch<float, 8>::load_aligned(const float* src)
    {
        this->m_value = _mm256_load_ps(src);
        return *this;
    }

    inline batch<float, 8>& batch<float, 8>::load_unaligned(const float* src)
    {
        this->m_value = _mm256_loadu_ps(src);
        return *this;
    }

    inline batch<float, 8>& batch<float, 8>::load_aligned(const double* src)
    {
        __m128 tmp1 = _mm256_cvtpd_ps(_mm256_load_pd(src));
        __m128 tmp2 = _mm256_cvtpd_ps(_mm256_load_pd(src + 4));
        this->m_value = _mm256_castps128_ps256(tmp1);
        this->m_value = _mm256_insertf128_ps(this->m_value, tmp2, 1);
        return *this;
    }

    inline batch<float, 8>& batch<float, 8>::load_unaligned(const double* src)
    {
        __m128 tmp1 = _mm256_cvtpd_ps(_mm256_loadu_pd(src));
        __m128 tmp2 = _mm256_cvtpd_ps(_mm256_loadu_pd(src + 4));
        this->m_value = _mm256_castps128_ps256(tmp1);
        this->m_value = _mm256_insertf128_ps(this->m_value, tmp2, 1);
        return *this;
    }

    inline void batch<float, 8>::store_aligned(int8_t* dst) const
    {
        __m256i tmp = _mm256_cvtps_epi32(this->m_value);
        __m128i tmp2 = detail::xsimd_cvtepi32_epi8(tmp);
        _mm_storel_epi64((__m128i*)dst, tmp2);
    }

    inline void batch<float, 8>::store_unaligned(int8_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<float, 8>::store_aligned(uint8_t* dst) const
    {
        __m256i tmp = _mm256_cvtps_epi32(this->m_value);
        __m128i tmp2 = detail::xsimd_cvtepi32_epu8(tmp);
        _mm_storel_epi64((__m128i*)dst, tmp2);
    }

    inline void batch<float, 8>::store_unaligned(uint8_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<float, 8>::store_aligned(int16_t* dst) const
    {
        __m256i tmp = _mm256_cvtps_epi32(this->m_value);
        __m128i tmp2 = detail::xsimd_cvtepi32_epi16(tmp);
        _mm_store_si128((__m128i*)dst, tmp2);
    }

    inline void batch<float, 8>::store_unaligned(int16_t* dst) const
    {
        __m256i tmp = _mm256_cvtps_epi32(this->m_value);
        __m128i tmp2 = detail::xsimd_cvtepi32_epi16(tmp);
        _mm_storeu_si128((__m128i*)dst, tmp2);
    }

    inline void batch<float, 8>::store_aligned(uint16_t* dst) const
    {
        __m256i tmp = _mm256_cvtps_epi32(this->m_value);
        __m128i tmp2 = detail::xsimd_cvtepi32_epu16(tmp);
        _mm_store_si128((__m128i*)dst, tmp2);
    }

    inline void batch<float, 8>::store_unaligned(uint16_t* dst) const
    {
        __m256i tmp = _mm256_cvtps_epi32(this->m_value);
        __m128i tmp2 = detail::xsimd_cvtepi32_epu16(tmp);
        _mm_storeu_si128((__m128i*)dst, tmp2);
    }

    inline void batch<float, 8>::store_aligned(int32_t* dst) const
    {
        _mm256_store_si256((__m256i*)dst, _mm256_cvtps_epi32(this->m_value));
    }

    inline void batch<float, 8>::store_unaligned(int32_t* dst) const
    {
        _mm256_storeu_si256((__m256i*)dst, _mm256_cvtps_epi32(this->m_value));
    }

    inline void batch<float, 8>::store_aligned(float* dst) const
    {
        _mm256_store_ps(dst, this->m_value);
    }

    inline void batch<float, 8>::store_unaligned(float* dst) const
    {
        _mm256_storeu_ps(dst, this->m_value);
    }

    inline void batch<float, 8>::store_aligned(double* dst) const
    {
        alignas(32) float tmp[8];
        _mm256_store_ps(tmp, this->m_value);
        dst[0] = static_cast<double>(tmp[0]);
        dst[1] = static_cast<double>(tmp[1]);
        dst[2] = static_cast<double>(tmp[2]);
        dst[3] = static_cast<double>(tmp[3]);
        dst[4] = static_cast<double>(tmp[4]);
        dst[5] = static_cast<double>(tmp[5]);
        dst[6] = static_cast<double>(tmp[6]);
        dst[7] = static_cast<double>(tmp[7]);
    }

    inline void batch<float, 8>::store_unaligned(double* dst) const
    {
        store_aligned(dst);
    }

    namespace detail
    {
        template <>
        struct batch_kernel<float, 8>
        {
            using batch_type = batch<float, 8>;
            using value_type = float;
            using batch_bool_type = batch_bool<float, 8>;

            static batch_type neg(const batch_type& rhs)
            {
                return _mm256_xor_ps(rhs, _mm256_castsi256_ps(_mm256_set1_epi32(0x80000000)));
            }

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_add_ps(lhs, rhs);
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_sub_ps(lhs, rhs);
            }

            static batch_type sadd(const batch_type& lhs, const batch_type& rhs)
            {
                return add(lhs, rhs); //FIXME something special for inf ?
            }

            static batch_type ssub(const batch_type& lhs, const batch_type& rhs)
            {
                return sub(lhs,rhs); //FIXME something special for inf ?
            }

            static batch_type mul(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_mul_ps(lhs, rhs);
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_div_ps(lhs, rhs);
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_cmp_ps(lhs, rhs, _CMP_EQ_OQ);
            }

            static batch_bool_type neq(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_cmp_ps(lhs, rhs, _CMP_NEQ_OQ);
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_cmp_ps(lhs, rhs, _CMP_LT_OQ);
            }

            static batch_bool_type lte(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_cmp_ps(lhs, rhs, _CMP_LE_OQ);
            }

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_and_ps(lhs, rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_or_ps(lhs, rhs);
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_xor_ps(lhs, rhs);
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return _mm256_xor_ps(rhs, _mm256_castsi256_ps(_mm256_set1_epi32(-1)));
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_andnot_ps(lhs, rhs);
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_min_ps(lhs, rhs);
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_max_ps(lhs, rhs);
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
                __m256 sign_mask = _mm256_set1_ps(-0.f);  // -0.f = 1 << 31
                return _mm256_andnot_ps(sign_mask, rhs);
            }

            static batch_type fabs(const batch_type& rhs)
            {
                return abs(rhs);
            }

            static batch_type sqrt(const batch_type& rhs)
            {
                return _mm256_sqrt_ps(rhs);
            }

            static batch_type fma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_FMA3_VERSION
                return _mm256_fmadd_ps(x, y, z);
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_AMD_FMA4_VERSION
                return _mm256_macc_ps(x, y, z);
#else
                return x * y + z;
#endif
            }

            static batch_type fms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_FMA3_VERSION
                return _mm256_fmsub_ps(x, y, z);
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_AMD_FMA4_VERSION
                return _mm256_msub_ps(x, y, z);
#else
                return x * y - z;
#endif
            }

            static batch_type fnma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_FMA3_VERSION
                return _mm256_fnmadd_ps(x, y, z);
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_AMD_FMA4_VERSION
                return _mm256_nmacc_ps(x, y, z);
#else
                return -x * y + z;
#endif
            }

            static batch_type fnms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_FMA3_VERSION
                return _mm256_fnmsub_ps(x, y, z);
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_AMD_FMA4_VERSION
                return _mm256_nmsub_ps(x, y, z);
#else
                return -x * y - z;
#endif
            }

            static value_type hadd(const batch_type& rhs)
            {
                // Warning about _mm256_hadd_ps:
                // _mm256_hadd_ps(a,b) gives
                // (a0+a1,a2+a3,b0+b1,b2+b3,a4+a5,a6+a7,b4+b5,b6+b7). Hence we can't
                // rely on a naive use of this method
                // rhs = (x0, x1, x2, x3, x4, x5, x6, x7)
                // tmp = (x4, x5, x6, x7, x0, x1, x2, x3)
                __m256 tmp = _mm256_permute2f128_ps(rhs, rhs, 1);
                // tmp = (x4+x0, x5+x1, x6+x2, x7+x3, x0+x4, x1+x5, x2+x6, x3+x7)
                tmp = _mm256_add_ps(rhs, tmp);
                // tmp = (x4+x0+x5+x1, x6+x2+x7+x3, -, -, -, -, -, -)
                tmp = _mm256_hadd_ps(tmp, tmp);
                // tmp = (x4+x0+x5+x1+x6+x2+x7+x3, -, -, -, -, -, -, -)
                tmp = _mm256_hadd_ps(tmp, tmp);
                return _mm_cvtss_f32(_mm256_extractf128_ps(tmp, 0));
            }

            static batch_type haddp(const batch_type* row)
            {
                // row = (a,b,c,d,e,f,g,h)
                // tmp0 = (a0+a1, a2+a3, b0+b1, b2+b3, a4+a5, a6+a7, b4+b5, b6+b7)
                __m256 tmp0 = _mm256_hadd_ps(row[0], row[1]);
                // tmp1 = (c0+c1, c2+c3, d1+d2, d2+d3, c4+c5, c6+c7, d4+d5, d6+d7)
                __m256 tmp1 = _mm256_hadd_ps(row[2], row[3]);
                // tmp1 = (a0+a1+a2+a3, b0+b1+b2+b3, c0+c1+c2+c3, d0+d1+d2+d3,
                // a4+a5+a6+a7, b4+b5+b6+b7, c4+c5+c6+c7, d4+d5+d6+d7)
                tmp1 = _mm256_hadd_ps(tmp0, tmp1);
                // tmp0 = (e0+e1, e2+e3, f0+f1, f2+f3, e4+e5, e6+e7, f4+f5, f6+f7)
                tmp0 = _mm256_hadd_ps(row[4], row[5]);
                // tmp2 = (g0+g1, g2+g3, h0+h1, h2+h3, g4+g5, g6+g7, h4+h5, h6+h7)
                __m256 tmp2 = _mm256_hadd_ps(row[6], row[7]);
                // tmp2 = (e0+e1+e2+e3, f0+f1+f2+f3, g0+g1+g2+g3, h0+h1+h2+h3,
                // e4+e5+e6+e7, f4+f5+f6+f7, g4+g5+g6+g7, h4+h5+h6+h7)
                tmp2 = _mm256_hadd_ps(tmp0, tmp2);
                // tmp0 = (a0+a1+a2+a3, b0+b1+b2+b3, c0+c1+c2+c3, d0+d1+d2+d3,
                // e4+e5+e6+e7, f4+f5+f6+f7, g4+g5+g6+g7, h4+h5+h6+h7)
                tmp0 = _mm256_blend_ps(tmp1, tmp2, 0b11110000);
                // tmp1 = (a4+a5+a6+a7, b4+b5+b6+b7, c4+c5+c6+c7, d4+d5+d6+d7,
                // e0+e1+e2+e3, f0+f1+f2+f3, g0+g1+g2+g3, h0+h1+h2+h3)
                tmp1 = _mm256_permute2f128_ps(tmp1, tmp2, 0x21);
                return _mm256_add_ps(tmp0, tmp1);
            }

            static batch_type select(const batch_bool_type& cond, const batch_type& a, const batch_type& b)
            {
                return _mm256_blendv_ps(b, a, cond);
            }

            template<bool... Values>
            static batch_type select(const batch_bool_constant<value_type, Values...>&, const batch_type& a, const batch_type& b)
            {
                constexpr int mask = batch_bool_constant<value_type, Values...>::mask();
                return _mm256_blend_ps(b, a, mask);
            }

            static batch_type zip_lo(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_unpacklo_ps(lhs, rhs);
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_unpackhi_ps(lhs, rhs);
            }

            static batch_bool_type isnan(const batch_type& x)
            {
                return _mm256_cmp_ps(x, x, _CMP_UNORD_Q);
            }
        };
    }
}

#endif
