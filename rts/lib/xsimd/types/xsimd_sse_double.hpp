/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_SSE_DOUBLE_HPP
#define XSIMD_SSE_DOUBLE_HPP

#include "xsimd_base.hpp"
#include <array>

namespace xsimd
{

    /*************************
     * batch_bool<double, 2> *
     *************************/

    template <>
    struct simd_batch_traits<batch_bool<double, 2>>
    {
        using value_type = double;
        static constexpr std::size_t size = 2;
        using batch_type = batch<double, 2>;
        static constexpr std::size_t align = 16;
    };

    template <>
    class batch_bool<double, 2> : public simd_batch_bool<batch_bool<double, 2>>
    {
    public:

        batch_bool();
        explicit batch_bool(bool b);
        batch_bool(bool b0, bool b1);
        batch_bool(const __m128d& rhs);
        batch_bool& operator=(const __m128d& rhs);

        operator __m128d() const;

        bool_proxy<double> operator[](std::size_t index);
        bool operator[](std::size_t index) const;

        __m128d get_value() const;

    private:

        batch_bool<double, 2>& load_values(bool b0, bool b1);

        union
        {
            __m128d m_value;
            double m_array[2];
        };

        friend class simd_batch_bool<batch_bool<double, 2>>;
    };

    /********************
     * batch<double, 2> *
     ********************/

    template <>
    struct simd_batch_traits<batch<double, 2>>
    {
        using value_type = double;
        static constexpr std::size_t size = 2;
        using batch_bool_type = batch_bool<double, 2>;
        static constexpr std::size_t align = 16;
        using storage_type = __m128d;
    };

    template <>
    class batch<double, 2> : public simd_batch<batch<double, 2>>
    {
    public:

        using self_type = batch<double, 2>;
        using base_type = simd_batch<self_type>;
        using batch_bool_type = typename base_type::batch_bool_type;

        batch();
        explicit batch(double d);
        batch(double d0, double d1);
        explicit batch(const double* src);

        batch(const double* src, aligned_mode);
        batch(const double* src, unaligned_mode);
        
        batch(const __m128d& rhs);
        batch& operator=(const __m128d& rhs);

        batch(const batch_bool_type& rhs);
        batch& operator=(const batch_bool_type& rhs);
        
        operator __m128d() const;

        XSIMD_DECLARE_LOAD_STORE_ALL(double, 2)
        XSIMD_DECLARE_LOAD_STORE_LONG(double, 2)

        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;
    };

    /****************************************
     * batch_bool<double, 2> implementation *
     ****************************************/

    inline batch_bool<double, 2>::batch_bool()
    {
    }

    inline batch_bool<double, 2>::batch_bool(bool b)
    {
        m_value = _mm_castsi128_pd(_mm_set1_epi32(-(int)b));
    }

    inline batch_bool<double, 2>::batch_bool(bool b0, bool b1)
    {
        m_value = _mm_castsi128_pd(_mm_setr_epi32(-(int)b0, -(int)b0, -(int)b1, -(int)b1));
    }

    inline batch_bool<double, 2>::batch_bool(const __m128d& rhs)
    {
        m_value = rhs;
    }

    inline batch_bool<double, 2>& batch_bool<double, 2>::operator=(const __m128d& rhs)
    {
        m_value = rhs;
        return *this;
    }

    inline batch<double, 2>::batch(const batch_bool_type& rhs)
        : base_type(_mm_and_pd(rhs, batch(1.)))
    {
    }

    inline batch<double, 2>& batch<double, 2>::operator=(const batch_bool_type& rhs)
    {
        this->m_value = _mm_and_pd(rhs, batch(1.));
        return *this;
    }

    inline batch_bool<double, 2>::operator __m128d() const
    {
        return m_value;
    }

    inline bool_proxy<double> batch_bool<double, 2>::operator[](std::size_t index)
    {
        return bool_proxy<double>(m_array[index & 1]);
    }

    inline bool batch_bool<double, 2>::operator[](std::size_t index) const
    {
        return static_cast<bool>(m_array[index & 1]);
    }

    inline __m128d batch_bool<double, 2>::get_value() const
    {
        return m_value;
    }

    inline batch_bool<double, 2>& batch_bool<double, 2>::load_values(bool b0, bool b1)
    {
        m_value = _mm_castsi128_pd(_mm_setr_epi32(-(int)b0, -(int)b0, -(int)b1, -(int)b1));
        return *this;
    }

    namespace detail
    {
        template <>
        struct batch_bool_kernel<double, 2>
        {
            using batch_type = batch_bool<double, 2>;

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_and_pd(lhs, rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_or_pd(lhs, rhs);
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_xor_pd(lhs, rhs);
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return _mm_xor_pd(rhs, _mm_castsi128_pd(_mm_set1_epi32(-1)));
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_andnot_pd(lhs, rhs);
            }

            static batch_type equal(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_castsi128_pd(_mm_cmpeq_epi32(_mm_castpd_si128(lhs), _mm_castpd_si128(rhs)));
            }

            static batch_type not_equal(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_cmpneq_pd(lhs, rhs);
            }

            static bool all(const batch_type& rhs)
            {
                return _mm_movemask_pd(rhs) == 3;
            }

            static bool any(const batch_type& rhs)
            {
                return _mm_movemask_pd(rhs) != 0;
            }
        };
    }

    /***********************************
     * batch<double, 2> implementation *
     ***********************************/

    inline batch<double, 2>::batch()
    {
    }

    inline batch<double, 2>::batch(double d)
        : base_type(_mm_set1_pd(d))
    {
    }

    inline batch<double, 2>::batch(double d0, double d1)
        : base_type(_mm_setr_pd(d0, d1))
    {
    }

    inline batch<double, 2>::batch(const double* src)
        : base_type(_mm_loadu_pd(src))
    {
    }

    inline batch<double, 2>::batch(const double* src, aligned_mode)
        : base_type(_mm_load_pd(src))
    {
    }

    inline batch<double, 2>::batch(const double* src, unaligned_mode)
        : base_type(_mm_loadu_pd(src))
    {
    }

    inline batch<double, 2>::batch(const __m128d& rhs)
        : base_type(rhs)
    {
    }

    inline batch<double, 2>& batch<double, 2>::operator=(const __m128d& rhs)
    {
        this->m_value = rhs;
        return *this;
    }

    inline batch<double, 2>::operator __m128d() const
    {
        return this->m_value;

    }
    inline batch<double, 2>& batch<double, 2>::load_aligned(const int8_t* src)
    {
        __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
        __m128i tmp1 = _mm_cvtepi8_epi32(tmp);
#else
        __m128i mask = _mm_cmplt_epi8(tmp, _mm_set1_epi8(0));
        __m128i tmp2 = _mm_unpacklo_epi8(tmp, mask);
        mask = _mm_cmplt_epi16(tmp2, _mm_set1_epi16(0));
        __m128i tmp1 = _mm_unpacklo_epi16(tmp2, mask);
#endif
        this->m_value = _mm_cvtepi32_pd(tmp1);
        return *this;
    }

    XSIMD_DEFINE_LOAD_STORE(double, 2, bool, 16)

    inline batch<double, 2>& batch<double, 2>::load_unaligned(const int8_t* src)
    {
        return load_aligned(src);
    }

    inline batch<double, 2>& batch<double, 2>::load_aligned(const uint8_t* src)
    {
        __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
        __m128i tmp1 = _mm_cvtepu8_epi32(tmp);
#else
        __m128i tmp2 = _mm_unpacklo_epi8(tmp, _mm_set1_epi8(0));
        __m128i tmp1 = _mm_unpacklo_epi16(tmp2, _mm_set1_epi16(0));
#endif
        this->m_value = _mm_cvtepi32_pd(tmp1);
        return *this;
    }
    
    inline batch<double, 2>& batch<double, 2>::load_unaligned(const uint8_t* src)
    {
        return load_aligned(src);
    }

    inline batch<double, 2>& batch<double, 2>::load_aligned(const int16_t* src)
    {
        __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
        __m128i tmp1 = _mm_cvtepi16_epi32(tmp);
#else
        __m128i mask = _mm_cmplt_epi16(tmp, _mm_set1_epi16(0));
        __m128i tmp1 = _mm_unpacklo_epi16(tmp, mask);
#endif
        this->m_value = _mm_cvtepi32_pd(tmp1);
        return *this;
    }

    inline batch<double, 2>& batch<double, 2>::load_unaligned(const int16_t* src)
    {
        return load_aligned(src);
    }

    inline batch<double, 2>& batch<double, 2>::load_aligned(const uint16_t* src)
    {
        __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
        __m128i tmp1 = _mm_cvtepu16_epi32(tmp);
#else
        __m128i tmp1 = _mm_unpacklo_epi16(tmp, _mm_set1_epi16(0));
#endif
        this->m_value = _mm_cvtepi32_pd(tmp1);
        return *this;
    }
    
    inline batch<double, 2>& batch<double, 2>::load_unaligned(const uint16_t* src)
    {
        return load_aligned(src);
    }

    XSIMD_DEFINE_LOAD_STORE(double, 2, int32_t, 16)
    XSIMD_DEFINE_LOAD_STORE(double, 2, uint32_t, 16)
    XSIMD_DEFINE_LOAD_STORE(double, 2, int64_t, 16)
    XSIMD_DEFINE_LOAD_STORE(double, 2, uint64_t, 16)
    XSIMD_DEFINE_LOAD_STORE_LONG(double, 2, 16)
    XSIMD_DEFINE_LOAD_STORE(double, 2, float, 16)

    inline batch<double, 2>& batch<double, 2>::load_aligned(const double* src)
    {
        this->m_value = _mm_load_pd(src);
        return *this;
    }

    inline batch<double, 2>& batch<double, 2>::load_unaligned(const double* src)
    {
        this->m_value = _mm_loadu_pd(src);
        return *this;
    }

    inline void batch<double, 2>::store_aligned(int8_t* dst) const
    {
        __m128i tmp = _mm_cvtpd_epi32(this->m_value);
        __m128i tmp1 = _mm_packs_epi32(tmp, tmp);
        __m128i tmp2 = _mm_packs_epi16(tmp1, tmp1);
        _mm_storel_epi64((__m128i*)dst, tmp2);
    }

    inline void batch<double, 2>::store_unaligned(int8_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<double, 2>::store_aligned(uint8_t* dst) const
    {
        __m128i tmp = _mm_cvtpd_epi32(this->m_value);
        __m128i tmp1 = _mm_packs_epi32(tmp, tmp);
        __m128i tmp2 = _mm_packus_epi16(tmp1, tmp1);
        _mm_storel_epi64((__m128i*)dst, tmp2);
    }

    inline void batch<double, 2>::store_unaligned(uint8_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<double, 2>::store_aligned(int16_t* dst) const
    {
        __m128i tmp = _mm_cvtpd_epi32(this->m_value);
        __m128i tmp1 = _mm_packs_epi32(tmp, tmp);
        _mm_storel_epi64((__m128i*)dst, tmp1);
    }

    inline void batch<double, 2>::store_unaligned(int16_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<double, 2>::store_aligned(uint16_t* dst) const
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
        __m128i tmp = _mm_cvtpd_epi32(this->m_value);
        __m128i tmp1 = _mm_packus_epi32(tmp, tmp);
        _mm_storel_epi64((__m128i*)dst, tmp1);
#else
        alignas(16) double tmp[2];
        _mm_store_pd(tmp, this->m_value);
        unroller<2>([&](std::size_t i){
            dst[i] = static_cast<uint16_t>(tmp[i]);
        });
#endif
    }

    inline void batch<double, 2>::store_unaligned(uint16_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<double, 2>::store_aligned(double* dst) const
    {
        _mm_store_pd(dst, this->m_value);
    }

    inline void batch<double, 2>::store_unaligned(double* dst) const
    {
        _mm_storeu_pd(dst, this->m_value);
    }

    namespace detail
    {
        template <>
        struct batch_kernel<double, 2>
        {
            using batch_type = batch<double, 2>;
            using value_type = double;
            using batch_bool_type = batch_bool<double, 2>;

            static batch_type neg(const batch_type& rhs)
            {
                return _mm_xor_pd(rhs, _mm_castsi128_pd(_mm_setr_epi32(0, 0x80000000,
                                                                       0, 0x80000000)));
            }

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_add_pd(lhs, rhs);
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_sub_pd(lhs, rhs);
            }

            static batch_type sadd(const batch_type& lhs, const batch_type& rhs)
            {
                return add(lhs, rhs); //do something special for inf?
            }

            static batch_type ssub(const batch_type& lhs, const batch_type& rhs)
            {
                return sub(lhs, rhs); //do something special for inf?
            }

            static batch_type mul(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_mul_pd(lhs, rhs);
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_div_pd(lhs, rhs);
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_cmpeq_pd(lhs, rhs);
            }

            static batch_bool_type neq(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_cmpneq_pd(lhs, rhs);
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_cmplt_pd(lhs, rhs);
            }

            static batch_bool_type lte(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_cmple_pd(lhs, rhs);
            }

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_and_pd(lhs, rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_or_pd(lhs, rhs);
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_xor_pd(lhs, rhs);
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return _mm_xor_pd(rhs, _mm_castsi128_pd(_mm_set1_epi32(-1)));
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_andnot_pd(lhs, rhs);
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_min_pd(lhs, rhs);
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_max_pd(lhs, rhs);
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
                __m128d sign_mask = _mm_set1_pd(-0.);  // -0. = 1 << 63
                return _mm_andnot_pd(sign_mask, rhs);
            }

            static batch_type fabs(const batch_type& rhs)
            {
                return abs(rhs);
            }

            static batch_type sqrt(const batch_type& rhs)
            {
                return _mm_sqrt_pd(rhs);
            }

            static batch_type fma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_FMA3_VERSION
                return _mm_fmadd_pd(x, y, z);
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_AMD_FMA4_VERSION
                return _mm_macc_pd(x, y, z);
#else
                return x * y + z;
#endif
            }

            static batch_type fms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_FMA3_VERSION
                return _mm_fmsub_pd(x, y, z);
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_AMD_FMA4_VERSION
                return _mm_msub_pd(x, y, z);
#else
                return x * y - z;
#endif
            }

            static batch_type fnma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_FMA3_VERSION
                return _mm_fnmadd_pd(x, y, z);
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_AMD_FMA4_VERSION
                return _mm_nmacc_pd(x, y, z);
#else
                return -x * y + z;
#endif
            }

            static batch_type fnms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_FMA3_VERSION
                return _mm_fnmsub_pd(x, y, z);
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_AMD_FMA4_VERSION
                return _mm_nmsub_pd(x, y, z);
#else
                return -x * y - z;
#endif
            }

            static value_type hadd(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE3_VERSION
                __m128d tmp0 = _mm_hadd_pd(rhs, rhs);
#else
                __m128d tmp0 = _mm_add_sd(rhs, _mm_unpackhi_pd(rhs, rhs));
#endif
                return _mm_cvtsd_f64(tmp0);
            }

            static batch_type haddp(const batch_type* row)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE3_VERSION
                return _mm_hadd_pd(row[0], row[1]);
#else
                return _mm_add_pd(_mm_unpacklo_pd(row[0], row[1]),
                    _mm_unpackhi_pd(row[0], row[1]));
#endif
            }

            static batch_type select(const batch_bool_type& cond, const batch_type& a, const batch_type& b)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
                return _mm_blendv_pd(b, a, cond);
#else
                return _mm_or_pd(_mm_and_pd(cond, a), _mm_andnot_pd(cond, b));
#endif
            }

            template<bool... Values>
            static batch_type select(const batch_bool_constant<value_type, Values...>& cond, const batch_type& a, const batch_type& b)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
                (void)cond;
                constexpr int mask = batch_bool_constant<value_type, Values...>::mask();
                return _mm_blend_pd(b, a, mask);
#else
                return select(cond(), a, b);
#endif
            }

            static batch_bool_type isnan(const batch_type& x)
            {
                return _mm_cmpunord_pd(x, x);
            }

            static batch_type zip_lo(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_unpacklo_pd(lhs, rhs);
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_unpackhi_pd(lhs, rhs);
            }
        };
    }
}

#endif
