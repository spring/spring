/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_AVX_DOUBLE_HPP
#define XSIMD_AVX_DOUBLE_HPP

#include "xsimd_base.hpp"
#include <array>

namespace xsimd
{

    /*************************
     * batch_bool<double, 4> *
     *************************/

    template <>
    struct simd_batch_traits<batch_bool<double, 4>>
    {
        using value_type = double;
        static constexpr std::size_t size = 4;
        using batch_type = batch<double, 4>;
        static constexpr std::size_t align = 32;
    };

    template <>
    class batch_bool<double, 4> : public simd_batch_bool<batch_bool<double, 4>>
    {
    public:

        batch_bool();
        explicit batch_bool(bool b);
        batch_bool(bool b0, bool b1, bool b2, bool b3);
        batch_bool(const __m256d& rhs);
        batch_bool& operator=(const __m256d& rhs);

        operator __m256d() const;

        bool_proxy<double> operator[](std::size_t index);
        bool operator[](std::size_t index) const;

        __m256d get_value() const;

    private:

        batch_bool<double, 4>& load_values(bool b0, bool b1, bool b2, bool b3);

        union
        {
            __m256d m_value;
            double m_array[4];
        };

        friend class simd_batch_bool<batch_bool<double, 4>>;
    };

    /********************
     * batch<double, 4> *
     ********************/

    template <>
    struct simd_batch_traits<batch<double, 4>>
    {
        using value_type = double;
        static constexpr std::size_t size = 4;
        using batch_bool_type = batch_bool<double, 4>;
        static constexpr std::size_t align = 32;
        using storage_type = __m256d;
    };

    template <>
    class batch<double, 4> : public simd_batch<batch<double, 4>>
    {
    public:

        using self_type = batch<double, 4>;
        using base_type = simd_batch<self_type>;
        using batch_bool_type = typename base_type::batch_bool_type;

        batch();
        explicit batch(double d);
        batch(double d0, double d1, double d2, double d3);
        explicit batch(const double* src);
        
        batch(const double* src, aligned_mode);
        batch(const double* src, unaligned_mode);
        
        batch(const __m256d& rhs);
        batch& operator=(const __m256d& rhs);

        batch(const batch_bool_type& rhs);
        batch& operator=(const batch_bool_type& rhs);
        
        operator __m256d() const;

        XSIMD_DECLARE_LOAD_STORE_ALL(double, 4)
        XSIMD_DECLARE_LOAD_STORE_LONG(double, 4)

        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;
    };

    /****************************************
     * batch_bool<double, 4> implementation *
     ****************************************/

    inline batch_bool<double, 4>::batch_bool()
    {
    }

    inline batch_bool<double, 4>::batch_bool(bool b)
    {
        m_value = _mm256_castsi256_pd(_mm256_set1_epi32(-(int)b));
    }

    inline batch_bool<double, 4>::batch_bool(bool b0, bool b1, bool b2, bool b3)
    {
        m_value = _mm256_castsi256_pd(
                    _mm256_setr_epi32(-(int)b0, -(int)b0, -(int)b1, -(int)b1,
                                      -(int)b2, -(int)b2, -(int)b3, -(int)b3));
    }

    inline batch_bool<double, 4>::batch_bool(const __m256d& rhs)
    {
        m_value = rhs;
    }

    inline batch_bool<double, 4>& batch_bool<double, 4>::operator=(const __m256d& rhs)
    {
        m_value = rhs;
        return *this;
    }

    inline batch_bool<double, 4>::operator __m256d() const
    {
        return m_value;
    }

    inline bool_proxy<double> batch_bool<double, 4>::operator[](std::size_t index)
    {
        return bool_proxy<double>(m_array[index & 3]);
    }

    inline bool batch_bool<double, 4>::operator[](std::size_t index) const
    {
        return static_cast<bool>(m_array[index & 3]);
    }

    inline __m256d batch_bool<double, 4>::get_value() const
    {
        return m_value;
    }

    inline batch_bool<double, 4>& batch_bool<double, 4>::load_values(bool b0, bool b1, bool b2, bool b3)
    {
        m_value = _mm256_castsi256_pd(
                    _mm256_setr_epi32(-(int)b0, -(int)b0, -(int)b1, -(int)b1,
                                      -(int)b2, -(int)b2, -(int)b3, -(int)b3));
        return *this;
    }

    namespace detail
    {
        template <>
        struct batch_bool_kernel<double, 4>
        {
            using batch_type = batch_bool<double, 4>;

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_and_pd(lhs, rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_or_pd(lhs, rhs);
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_xor_pd(lhs, rhs);
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return _mm256_xor_pd(rhs, _mm256_castsi256_pd(_mm256_set1_epi32(-1)));
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_andnot_pd(lhs, rhs);
            }

            static batch_type equal(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_castsi256_pd(_mm256_cmpeq_epi64(_mm256_castpd_si256(lhs), _mm256_castpd_si256(rhs)));
#else
                __m128i lhs_low = _mm256_castsi256_si128(_mm256_castpd_si256(lhs));
                __m128i lhs_high = _mm256_extractf128_si256(_mm256_castpd_si256(lhs), 1);
                __m128i rhs_low = _mm256_castsi256_si128(_mm256_castpd_si256(rhs));
                __m128i rhs_high = _mm256_extractf128_si256(_mm256_castpd_si256(rhs), 1);
                __m128i res_low = _mm_cmpeq_epi64(lhs_low, rhs_low);
                __m128i res_high = _mm_cmpeq_epi64(lhs_high, rhs_high);
                __m256i result = _mm256_castsi128_si256(res_low);
                return _mm256_castsi256_pd(_mm256_insertf128_si256(result, res_high, 1));
#endif
            }

            static batch_type not_equal(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_xor_pd(lhs, rhs);
            }

            static bool all(const batch_type& rhs)
            {
                return _mm256_testc_pd(rhs, batch_bool<double, 4>(true)) != 0;
            }

            static bool any(const batch_type& rhs)
            {
                return !_mm256_testz_pd(rhs, rhs);
            }
        };
    }

    /***********************************
     * batch<double, 4> implementation *
     ***********************************/

    inline batch<double, 4>::batch()
    {
    }

    inline batch<double, 4>::batch(double d)
        : base_type(_mm256_set1_pd(d))
    {
    }

    inline batch<double, 4>::batch(double d0, double d1, double d2, double d3)
        : base_type(_mm256_setr_pd(d0, d1, d2, d3))
    {
    }

    inline batch<double, 4>::batch(const double* src)
        : base_type(_mm256_loadu_pd(src))
    {
    }

    inline batch<double, 4>::batch(const double* src, aligned_mode)
        : base_type(_mm256_load_pd(src))
    {
    }

    inline batch<double, 4>::batch(const double* src, unaligned_mode)
        : base_type(_mm256_loadu_pd(src))
    {
    }

    inline batch<double, 4>::batch(const __m256d& rhs)
        : base_type(rhs)
    {
    }

    inline batch<double, 4>& batch<double, 4>::operator=(const __m256d& rhs)
    {
        this->m_value = rhs;
        return *this;
    }

    inline batch<double, 4>::batch(const batch_bool_type& rhs)
        : base_type(_mm256_and_pd(rhs, batch(1.)))
    {
    }

    inline batch<double, 4>& batch<double, 4>::operator=(const batch_bool_type& rhs)
    {
        this->m_value = _mm256_and_pd(rhs, batch(1.));
        return *this;
    }

    inline batch<double, 4>::operator __m256d() const
    {
        return this->m_value;
    }

    XSIMD_DEFINE_LOAD_STORE(double, 4, bool, 32)

    inline batch<double, 4>& batch<double, 4>::load_aligned(const int8_t* src)
    {
        __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
        __m128i tmp2 = _mm_cvtepi8_epi32(tmp);
        this->m_value = _mm256_cvtepi32_pd(tmp2);
        return *this;
    }

    inline batch<double, 4>& batch<double, 4>::load_unaligned(const int8_t* src)
    {
        return load_aligned(src);
    }

    inline batch<double, 4>& batch<double, 4>::load_aligned(const uint8_t* src)
    {
        __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
        __m128i tmp2 = _mm_cvtepu8_epi32(tmp);
        this->m_value = _mm256_cvtepi32_pd(tmp2);
        return *this;
    }

    inline batch<double, 4>& batch<double, 4>::load_unaligned(const uint8_t* src)
    {
        return load_aligned(src);
    }

    inline batch<double, 4>& batch<double, 4>::load_aligned(const int16_t* src)
    {
        __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
        __m128i tmp2 = _mm_cvtepi16_epi32(tmp);
        this->m_value = _mm256_cvtepi32_pd(tmp2);
        return *this;
    }

    inline batch<double, 4>& batch<double, 4>::load_unaligned(const int16_t* src)
    {
        return load_aligned(src);
    }

    inline batch<double, 4>& batch<double, 4>::load_aligned(const uint16_t* src)
    {
        __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
        __m128i tmp2 = _mm_cvtepu16_epi32(tmp);
        this->m_value = _mm256_cvtepi32_pd(tmp2);
        return *this;
    }

    inline batch<double, 4>& batch<double, 4>::load_unaligned(const uint16_t* src)
    {
        return load_aligned(src);
    }

    inline batch<double, 4>& batch<double, 4>::load_aligned(const int32_t* src)
    {
        this->m_value = _mm256_cvtepi32_pd(_mm_load_si128((__m128i const*)src));
        return *this;
    }

    inline batch<double, 4>& batch<double, 4>::load_unaligned(const int32_t* src)
    {
        this->m_value = _mm256_cvtepi32_pd(_mm_loadu_si128((__m128i const*)src));
        return *this;
    }

    XSIMD_DEFINE_LOAD_STORE(double, 4, uint32_t, 32)
    XSIMD_DEFINE_LOAD_STORE(double, 4, int64_t, 32)
    XSIMD_DEFINE_LOAD_STORE(double, 4, uint64_t, 32)
    XSIMD_DEFINE_LOAD_STORE_LONG(double, 4, 32)

    inline batch<double, 4>& batch<double, 4>::load_aligned(const float* src)
    {
        this->m_value = _mm256_cvtps_pd(_mm_load_ps(src));
        return *this;
    }

    inline batch<double, 4>& batch<double, 4>::load_unaligned(const float* src)
    {
        this->m_value = _mm256_cvtps_pd(_mm_loadu_ps(src));
        return *this;
    }

    inline batch<double, 4>& batch<double, 4>::load_aligned(const double* src)
    {
        this->m_value = _mm256_load_pd(src);
        return *this;
    }

    inline batch<double, 4>& batch<double, 4>::load_unaligned(const double* src)
    {
        this->m_value = _mm256_loadu_pd(src);
        return *this;
    }

    inline void batch<double, 4>::store_aligned(int8_t* dst) const
    {
        __m128i tmp = _mm256_cvtpd_epi32(this->m_value);
        __m128i tmp1 = _mm_packs_epi32(tmp, _mm_set1_epi32(0));
        __m128i tmp2 = _mm_packs_epi16(tmp1, _mm_set1_epi16(0));
        _mm_storel_epi64((__m128i*)dst, tmp2);
    }

    inline void batch<double, 4>::store_unaligned(int8_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<double, 4>::store_aligned(uint8_t* dst) const
    {
        __m128i tmp = _mm256_cvtpd_epi32(this->m_value);
        __m128i tmp1 = _mm_packs_epi32(tmp, _mm_set1_epi32(0));
        __m128i tmp2 = _mm_packus_epi16(tmp1, _mm_set1_epi16(0));
        _mm_storel_epi64((__m128i*)dst, tmp2);
    }

    inline void batch<double, 4>::store_unaligned(uint8_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<double, 4>::store_aligned(int16_t* dst) const
    {
        __m128i tmp = _mm256_cvtpd_epi32(this->m_value);
        __m128i tmp1 = _mm_packs_epi32(tmp, _mm_set1_epi32(0));
        _mm_storel_epi64((__m128i*)dst, tmp1);
    }

    inline void batch<double, 4>::store_unaligned(int16_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<double, 4>::store_aligned(uint16_t* dst) const
    {
        __m128i tmp = _mm256_cvtpd_epi32(this->m_value);
        __m128i tmp1 = _mm_packs_epi32(tmp, _mm_set1_epi32(0));
        _mm_storel_epi64((__m128i*)dst, tmp1);
    }

    inline void batch<double, 4>::store_unaligned(uint16_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<double, 4>::store_aligned(int32_t* dst) const
    {
        _mm_store_si128((__m128i*)dst, _mm256_cvtpd_epi32(this->m_value));
    }

    inline void batch<double, 4>::store_unaligned(int32_t* dst) const
    {
        _mm_storeu_si128((__m128i*)dst, _mm256_cvtpd_epi32(this->m_value));
    }

    inline void batch<double, 4>::store_aligned(float* dst) const
    {
        _mm_store_ps(dst, _mm256_cvtpd_ps(this->m_value));
    }

    inline void batch<double, 4>::store_unaligned(float* dst) const
    {
        _mm_storeu_ps(dst, _mm256_cvtpd_ps(this->m_value));
    }

    inline void batch<double, 4>::store_aligned(double* dst) const
    {
        _mm256_store_pd(dst, this->m_value);
    }

    inline void batch<double, 4>::store_unaligned(double* dst) const
    {
        _mm256_storeu_pd(dst, this->m_value);
    }

    namespace detail
    {
        template <>
        struct batch_kernel<double, 4>
        {
            using batch_type = batch<double, 4>;
            using value_type = double;
            using batch_bool_type = batch_bool<double, 4>;

            static batch_type neg(const batch_type& rhs)
            {
                return _mm256_xor_pd(rhs, _mm256_castsi256_pd(_mm256_set1_epi64x(0x8000000000000000)));
            }

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_add_pd(lhs, rhs);
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_sub_pd(lhs, rhs);
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
                return _mm256_mul_pd(lhs, rhs);
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_div_pd(lhs, rhs);
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_cmp_pd(lhs, rhs, _CMP_EQ_OQ);
            }

            static batch_bool_type neq(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_cmp_pd(lhs, rhs, _CMP_NEQ_OQ);
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_cmp_pd(lhs, rhs, _CMP_LT_OQ);
            }

            static batch_bool_type lte(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_cmp_pd(lhs, rhs, _CMP_LE_OQ);
            }

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_and_pd(lhs, rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_or_pd(lhs, rhs);
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_xor_pd(lhs, rhs);
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return _mm256_xor_pd(rhs, _mm256_castsi256_pd(_mm256_set1_epi32(-1)));
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_andnot_pd(lhs, rhs);
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_min_pd(lhs, rhs);
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_max_pd(lhs, rhs);
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
                __m256d sign_mask = _mm256_set1_pd(-0.);  // -0. = 1 << 63
                return _mm256_andnot_pd(sign_mask, rhs);
            }

            static batch_type fabs(const batch_type& rhs)
            {
                return abs(rhs);
            }

            static batch_type sqrt(const batch_type& rhs)
            {
                return _mm256_sqrt_pd(rhs);
            }

            static batch_type fma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_FMA3_VERSION
                return _mm256_fmadd_pd(x, y, z);
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_AMD_FMA4_VERSION
                return _mm256_macc_pd(x, y, z);
#else
                return x * y + z;
#endif
            }

            static batch_type fms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_FMA3_VERSION
                return _mm256_fmsub_pd(x, y, z);
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_AMD_FMA4_VERSION
                return _mm256_msub_pd(x, y, z);
#else
                return x * y - z;
#endif
            }

            static batch_type fnma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_FMA3_VERSION
                return _mm256_fnmadd_pd(x, y, z);
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_AMD_FMA4_VERSION
                return _mm256_nmacc_pd(x, y, z);
#else
                return -x * y + z;
#endif
            }

            static batch_type fnms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_FMA3_VERSION
                return _mm256_fnmsub_pd(x, y, z);
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_AMD_FMA4_VERSION
                return _mm256_nmsub_pd(x, y, z);
#else
                return -x * y - z;
#endif
            }

            static value_type hadd(const batch_type& rhs)
            {
                // rhs = (x0, x1, x2, x3)
                // tmp = (x2, x3, x0, x1)
                __m256d tmp = _mm256_permute2f128_pd(rhs, rhs, 1);
                // tmp = (x2+x0, x3+x1, -, -)
                tmp = _mm256_add_pd(rhs, tmp);
                // tmp = (x2+x0+x3+x1, -, -, -)
                tmp = _mm256_hadd_pd(tmp, tmp);
                return _mm_cvtsd_f64(_mm256_extractf128_pd(tmp, 0));
            }

            static batch_type haddp(const batch_type* row)
            {
                // row = (a,b,c,d)
                // tmp0 = (a0+a1, b0+b1, a2+a3, b2+b3)
                __m256d tmp0 = _mm256_hadd_pd(row[0], row[1]);
                // tmp1 = (c0+c1, d0+d1, c2+c3, d2+d3)
                __m256d tmp1 = _mm256_hadd_pd(row[2], row[3]);
                // tmp2 = (a0+a1, b0+b1, c2+c3, d2+d3)
                __m256d tmp2 = _mm256_blend_pd(tmp0, tmp1, 0b1100);
                // tmp1 = (a2+a3, b2+b3, c2+c3, d2+d3)
                tmp1 = _mm256_permute2f128_pd(tmp0, tmp1, 0x21);
                return _mm256_add_pd(tmp1, tmp2);
            }

            static batch_type select(const batch_bool_type& cond, const batch_type& a, const batch_type& b)
            {
                return _mm256_blendv_pd(b, a, cond);
            }

            template<bool... Values>
            static batch_type select(const batch_bool_constant<value_type, Values...>&, const batch_type& a, const batch_type& b)
            {
                constexpr int mask = batch_bool_constant<value_type, Values...>::mask();
                return _mm256_blend_pd(b, a, mask);
            }

            static batch_type zip_lo(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_unpacklo_pd(lhs, rhs);
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_unpackhi_pd(lhs, rhs);
            }


            static batch_bool_type isnan(const batch_type& x)
            {
                return _mm256_cmp_pd(x, x, _CMP_UNORD_Q);
            }
        };
    }
}

#endif
