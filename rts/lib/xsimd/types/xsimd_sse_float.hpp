/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_SSE_FLOAT_HPP
#define XSIMD_SSE_FLOAT_HPP

#include "xsimd_base.hpp"
#include <array>

namespace xsimd
{

    /************************
     * batch_bool<float, 4> *
     ************************/

    template <>
    struct simd_batch_traits<batch_bool<float, 4>>
    {
        using value_type = float;
        static constexpr std::size_t size = 4;
        using batch_type = batch<float, 4>;
        static constexpr std::size_t align = 16;
    };

    template <>
    class batch_bool<float, 4> : public simd_batch_bool<batch_bool<float, 4>>
    {
    public:

        batch_bool();
        explicit batch_bool(bool b);
        batch_bool(bool b0, bool b1, bool b2, bool b3);
        batch_bool(const __m128& rhs);
        batch_bool& operator=(const __m128& rhs);

        operator __m128() const;

        bool_proxy<float> operator[](std::size_t index);
        bool operator[](std::size_t index) const;

        __m128 get_value() const;

    private:

        batch_bool<float, 4>& load_values(bool b0, bool b1, bool b2, bool b3);

        union
        {
            __m128 m_value;
            float m_array[4];
        };

        friend class simd_batch_bool<batch_bool<float, 4>>;
    };

    /*******************
     * batch<float, 4> *
     *******************/

    template <>
    struct simd_batch_traits<batch<float, 4>>
    {
        using value_type = float;
        static constexpr std::size_t size = 4;
        using batch_bool_type = batch_bool<float, 4>;
        static constexpr std::size_t align = 16;
        using storage_type = __m128;
    };

    template <>
    class batch<float, 4> : public simd_batch<batch<float, 4>>
    {
    public:

        using self_type = batch<float, 4>;
        using base_type = simd_batch<self_type>;
        using batch_bool_type = typename base_type::batch_bool_type;

        batch();
        explicit batch(float f);
        batch(float f0, float f1, float f2, float f3);
        explicit batch(const float* src);

        batch(const float* src, aligned_mode);
        batch(const float* src, unaligned_mode);
        
        batch(const __m128& rhs);
        batch& operator=(const __m128& rhs);
        
        batch(const batch_bool_type& rhs);
        batch& operator=(const batch_bool_type& rhs);
        
        operator __m128() const;

        XSIMD_DECLARE_LOAD_STORE_ALL(float, 4)
        XSIMD_DECLARE_LOAD_STORE_LONG(float, 4)

        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;
    };

    /***************************************
     * batch_bool<float, 4> implementation *
     ***************************************/

    inline batch_bool<float, 4>::batch_bool()
    {
    }

    inline batch_bool<float, 4>::batch_bool(bool b)
    {
        m_value = _mm_castsi128_ps(_mm_set1_epi32(-(int)b));
    }

    inline batch_bool<float, 4>::batch_bool(bool b0, bool b1, bool b2, bool b3)
    {
        m_value = _mm_castsi128_ps(_mm_setr_epi32(-(int)b0, -(int)b1, -(int)b2, -(int)b3));
    }

    inline batch_bool<float, 4>::batch_bool(const __m128& rhs)
    {
        m_value = rhs;
    }

    inline batch_bool<float, 4>& batch_bool<float, 4>::operator=(const __m128& rhs)
    {
        m_value = rhs;
        return *this;
    }

    inline batch<float, 4>::batch(const batch_bool_type& rhs)
        : base_type(_mm_and_ps(rhs, batch(1.f)))
    {
    }

    inline batch<float, 4>& batch<float, 4>::operator=(const batch_bool_type& rhs)
    {
        this->m_value = _mm_and_ps(rhs, batch(1.f));
        return *this;
    }

    inline batch_bool<float, 4>::operator __m128() const
    {
        return m_value;
    }

    inline bool_proxy<float> batch_bool<float, 4>::operator[](std::size_t index)
    {
        return bool_proxy<float>(m_array[index & 3]);
    }

    inline bool batch_bool<float, 4>::operator[](std::size_t index) const
    {
        return static_cast<bool>(m_array[index & 3]);
    }

    inline __m128 batch_bool<float, 4>::get_value() const
    {
        return m_value;
    }

    inline batch_bool<float, 4>& batch_bool<float, 4>::load_values(bool b0, bool b1, bool b2, bool b3)
    {
        m_value = _mm_castsi128_ps(_mm_setr_epi32(-(int)b0, -(int)b1, -(int)b2, -(int)b3));
        return *this;
    }
    
    namespace detail
    {
        template <>
        struct batch_bool_kernel<float, 4>
        {
            using batch_type = batch_bool<float, 4>;

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_and_ps(lhs, rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_or_ps(lhs, rhs);
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_xor_ps(lhs, rhs);
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return _mm_xor_ps(rhs, _mm_castsi128_ps(_mm_set1_epi32(-1)));
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_andnot_ps(lhs, rhs);
            }

            static batch_type equal(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(lhs), _mm_castps_si128(rhs)));
            }

            static batch_type not_equal(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_cmpneq_ps(lhs, rhs);
            }

            static bool all(const batch_type& rhs)
            {
                return _mm_movemask_ps(rhs) == 0x0F;
            }

            static bool any(const batch_type& rhs)
            {
                return _mm_movemask_ps(rhs) != 0;
            }
        };
    }

    /**********************************
     * batch<float, 4> implementation *
     **********************************/

    inline batch<float, 4>::batch()
    {
    }

    inline batch<float, 4>::batch(float f)
        : base_type(_mm_set1_ps(f))
    {
    }

    inline batch<float, 4>::batch(float f0, float f1, float f2, float f3)
        : base_type(_mm_setr_ps(f0, f1, f2, f3))
    {
    }

    inline batch<float, 4>::batch(const float* src)
        : base_type(_mm_loadu_ps(src))
    {
    }

    inline batch<float, 4>::batch(const float* src, aligned_mode)
        : base_type(_mm_load_ps(src))
    {
    }

    inline batch<float, 4>::batch(const float* src, unaligned_mode)
        : base_type(_mm_loadu_ps(src))
    {
    }

    inline batch<float, 4>::batch(const __m128& rhs)
        : base_type(rhs)
    {
    }

    inline batch<float, 4>& batch<float, 4>::operator=(const __m128& rhs)
    {
        this->m_value = rhs;
        return *this;
    }

    inline batch<float, 4>::operator __m128() const
    {
        return this->m_value;
    }

    XSIMD_DEFINE_LOAD_STORE(float, 4, bool, 16)

    inline batch<float, 4>& batch<float, 4>::load_aligned(const int8_t* src)
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
        this->m_value = _mm_cvtepi32_ps(tmp1);
        return *this;
    }

    inline batch<float, 4>& batch<float, 4>::load_unaligned(const int8_t* src)
    {
        return load_aligned(src);
    }

    inline batch<float, 4>& batch<float, 4>::load_aligned(const uint8_t* src)
    {
        __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
        __m128i tmp1 = _mm_cvtepu8_epi32(tmp);
#else
        __m128i tmp2 = _mm_unpacklo_epi8(tmp, _mm_set1_epi8(0));
        __m128i tmp1 = _mm_unpacklo_epi16(tmp2, _mm_set1_epi16(0));
#endif
        this->m_value = _mm_cvtepi32_ps(tmp1);
        return *this;
    }

    inline batch<float, 4>& batch<float, 4>::load_unaligned(const uint8_t* src)
    {
        return load_aligned(src);
    }

    inline batch<float, 4>& batch<float, 4>::load_aligned(const int16_t* src)
    {
        __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
        __m128i tmp1 = _mm_cvtepi16_epi32(tmp);
#else
        __m128i mask = _mm_cmplt_epi16(tmp, _mm_set1_epi16(0));
        __m128i tmp1 = _mm_unpacklo_epi16(tmp, mask);
#endif
        this->m_value = _mm_cvtepi32_ps(tmp1);
        return *this;
    }

    inline batch<float, 4>& batch<float, 4>::load_unaligned(const int16_t* src)
    {
        return load_aligned(src);
    }

    inline batch<float, 4>& batch<float, 4>::load_aligned(const uint16_t* src)
    {
        __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
        __m128i tmp1 = _mm_cvtepu16_epi32(tmp);
#else
        __m128i tmp1 = _mm_unpacklo_epi16(tmp, _mm_set1_epi16(0));
#endif
        this->m_value = _mm_cvtepi32_ps(tmp1);
        return *this;
    }

    inline batch<float, 4>& batch<float, 4>::load_unaligned(const uint16_t* src)
    {
        return load_aligned(src);
    }

    inline batch<float, 4>& batch<float, 4>::load_aligned(const int32_t* src)
    {
        this->m_value = _mm_cvtepi32_ps(_mm_load_si128((__m128i const*)src));
        return *this;
    }

    inline batch<float, 4>& batch<float, 4>::load_unaligned(const int32_t* src)
    {
        this->m_value = _mm_cvtepi32_ps(_mm_loadu_si128((__m128i const*)src));
        return *this;
    }

    XSIMD_DEFINE_LOAD_STORE(float, 4, uint32_t, 16)
    XSIMD_DEFINE_LOAD_STORE(float, 4, int64_t, 16)
    XSIMD_DEFINE_LOAD_STORE(float, 4, uint64_t, 16)
    XSIMD_DEFINE_LOAD_STORE_LONG(float, 4, 16)

    inline batch<float, 4>& batch<float, 4>::load_aligned(const float* src)
    {
        this->m_value = _mm_load_ps(src);
        return *this;
    }

    inline batch<float, 4>& batch<float, 4>::load_unaligned(const float* src)
    {
        this->m_value = _mm_loadu_ps(src);
        return *this;
    }

    inline batch<float, 4>& batch<float, 4>::load_aligned(const double* src)
    {
        __m128 tmp1 = _mm_cvtpd_ps(_mm_load_pd(src));
        __m128 tmp2 = _mm_cvtpd_ps(_mm_load_pd(src+2));
        this->m_value = _mm_shuffle_ps(tmp1, tmp2, _MM_SHUFFLE(1, 0, 1, 0));
        return *this;
    }

    inline batch<float, 4>& batch<float, 4>::load_unaligned(const double* src)
    {
        __m128 tmp1 = _mm_cvtpd_ps(_mm_loadu_pd(src));
        __m128 tmp2 = _mm_cvtpd_ps(_mm_loadu_pd(src + 2));
        this->m_value = _mm_shuffle_ps(tmp1, tmp2, _MM_SHUFFLE(1, 0, 1, 0));
        return *this;
    }

    inline void batch<float, 4>::store_aligned(int8_t* dst) const
    {
        __m128i tmp = _mm_cvtps_epi32(this->m_value);
        __m128i tmp1 = _mm_packs_epi32(tmp, _mm_set1_epi32(0));
        __m128i tmp2 = _mm_packs_epi16(tmp1, _mm_set1_epi16(0));
        _mm_storel_epi64((__m128i*)dst, tmp2);
    }

    inline void batch<float, 4>::store_unaligned(int8_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<float, 4>::store_aligned(uint8_t* dst) const
    {
        __m128i tmp = _mm_cvtps_epi32(this->m_value);
        __m128i tmp1 = _mm_packs_epi32(tmp, _mm_set1_epi32(0));
        __m128i tmp2 = _mm_packus_epi16(tmp1, _mm_set1_epi16(0));
        _mm_storel_epi64((__m128i*)dst, tmp2);
    }

    inline void batch<float, 4>::store_unaligned(uint8_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<float, 4>::store_aligned(int16_t* dst) const
    {
        __m128i tmp = _mm_cvtps_epi32(this->m_value);
        __m128i tmp1 = _mm_packs_epi32(tmp, _mm_set1_epi32(0));
        _mm_storel_epi64((__m128i*)dst, tmp1);
    }

    inline void batch<float, 4>::store_unaligned(int16_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<float, 4>::store_aligned(uint16_t* dst) const
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
        __m128i tmp = _mm_cvtps_epi32(this->m_value);
        __m128i tmp1 = _mm_packus_epi32(tmp, _mm_set1_epi32(0));
        _mm_storel_epi64((__m128i*)dst, tmp1);
#else
        alignas(16) float tmp[4];
        _mm_store_ps(tmp, this->m_value);
        unroller<4>([&](std::size_t i){
            dst[i] = static_cast<uint16_t>(tmp[i]);
        });
#endif
    }

    inline void batch<float, 4>::store_unaligned(uint16_t* dst) const
    {
        store_aligned(dst);
    }

    inline void batch<float, 4>::store_aligned(int32_t* dst) const
    {
        _mm_store_si128((__m128i*)dst, _mm_cvtps_epi32(this->m_value));
    }

    inline void batch<float, 4>::store_unaligned(int32_t* dst) const
    {
        _mm_storeu_si128((__m128i*)dst, _mm_cvtps_epi32(this->m_value));
    }

    inline void batch<float, 4>::store_aligned(float* dst) const
    {
        _mm_store_ps(dst, this->m_value);
    }

    inline void batch<float, 4>::store_unaligned(float* dst) const
    {
        _mm_storeu_ps(dst, this->m_value);
    }

    inline void batch<float, 4>::store_aligned(double* dst) const
    {
        __m128d tmp1 = _mm_cvtps_pd(this->m_value);
        __m128 ftmp = _mm_shuffle_ps(this->m_value, this->m_value, _MM_SHUFFLE(3, 2, 3, 2));
        __m128d tmp2 = _mm_cvtps_pd(ftmp);
        _mm_store_pd(dst, tmp1);
        _mm_store_pd(dst + 2, tmp2);
    }

    inline void batch<float, 4>::store_unaligned(double* dst) const
    {
        __m128d tmp1 = _mm_cvtps_pd(this->m_value);
        __m128 ftmp = _mm_shuffle_ps(this->m_value, this->m_value, _MM_SHUFFLE(3, 2, 3, 2));
        __m128d tmp2 = _mm_cvtps_pd(ftmp);
        _mm_storeu_pd(dst, tmp1);
        _mm_storeu_pd(dst + 2, tmp2);
    }

    namespace detail
    {
        template <>
        struct batch_kernel<float, 4>
        {
            using batch_type = batch<float, 4>;
            using value_type = float;
            using batch_bool_type = batch_bool<float, 4>;

            static batch_type neg(const batch_type& rhs)
            {
                return _mm_xor_ps(rhs, _mm_castsi128_ps(_mm_set1_epi32(0x80000000)));
            }

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_add_ps(lhs, rhs);
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_sub_ps(lhs, rhs);
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
                return _mm_mul_ps(lhs, rhs);
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_div_ps(lhs, rhs);
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_cmpeq_ps(lhs, rhs);
            }

            static batch_bool_type neq(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_cmpneq_ps(lhs, rhs);
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_cmplt_ps(lhs, rhs);
            }

            static batch_bool_type lte(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_cmple_ps(lhs, rhs);
            }

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_and_ps(lhs, rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_or_ps(lhs, rhs);
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_xor_ps(lhs, rhs);
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return _mm_xor_ps(rhs, _mm_castsi128_ps(_mm_set1_epi32(-1)));
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_andnot_ps(lhs, rhs);
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_min_ps(lhs, rhs);
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_max_ps(lhs, rhs);
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
                __m128 sign_mask = _mm_set1_ps(-0.f);  // -0.f = 1 << 31
                return _mm_andnot_ps(sign_mask, rhs);
            }

            static batch_type fabs(const batch_type& rhs)
            {
                return abs(rhs);
            }

            static batch_type sqrt(const batch_type& rhs)
            {
                return _mm_sqrt_ps(rhs);
            }

            static batch_type fma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_FMA3_VERSION
                return _mm_fmadd_ps(x, y, z);
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_AMD_FMA4_VERSION
                return _mm_macc_ps(x, y, z);
#else
                return x * y + z;
#endif
            }

            static batch_type fms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_FMA3_VERSION
                return _mm_fmsub_ps(x, y, z);
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_AMD_FMA4_VERSION
                return _mm_msub_ps(x, y, z);
#else
                return x * y - z;
#endif
            }

            static batch_type fnma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_FMA3_VERSION
                return _mm_fnmadd_ps(x, y, z);
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_AMD_FMA4_VERSION
                return _mm_nmacc_ps(x, y, z);
#else
                return -x * y + z;
#endif
            }

            static batch_type fnms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_FMA3_VERSION
                return _mm_fnmsub_ps(x, y, z);
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_AMD_FMA4_VERSION
                return _mm_nmsub_ps(x, y, z);
#else
                return -x * y - z;
#endif
            }

            static value_type hadd(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE3_VERSION
                __m128 tmp0 = _mm_hadd_ps(rhs, rhs);
                __m128 tmp1 = _mm_hadd_ps(tmp0, tmp0);
#else
                __m128 tmp0 = _mm_add_ps(rhs, _mm_movehl_ps(rhs, rhs));
                __m128 tmp1 = _mm_add_ss(tmp0, _mm_shuffle_ps(tmp0, tmp0, 1));
#endif
                return _mm_cvtss_f32(tmp1);
            }

            static batch_type haddp(const batch_type* row)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE3_VERSION
                return _mm_hadd_ps(_mm_hadd_ps(row[0], row[1]),
                    _mm_hadd_ps(row[2], row[3]));
#else
                __m128 tmp0 = _mm_unpacklo_ps(row[0], row[1]);
                __m128 tmp1 = _mm_unpackhi_ps(row[0], row[1]);
                __m128 tmp2 = _mm_unpackhi_ps(row[2], row[3]);
                tmp0 = _mm_add_ps(tmp0, tmp1);
                tmp1 = _mm_unpacklo_ps(row[2], row[3]);
                tmp1 = _mm_add_ps(tmp1, tmp2);
                tmp2 = _mm_movehl_ps(tmp1, tmp0);
                tmp0 = _mm_movelh_ps(tmp0, tmp1);
                return _mm_add_ps(tmp0, tmp2);
#endif
            }

            static batch_type select(const batch_bool_type& cond, const batch_type& a, const batch_type& b)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
                return _mm_blendv_ps(b, a, cond);
#else
                return _mm_or_ps(_mm_and_ps(cond, a), _mm_andnot_ps(cond, b));
#endif
            }

            template<bool... Values>
            static batch_type select(const batch_bool_constant<value_type, Values...>& cond, const batch_type& a, const batch_type& b)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
                (void)cond;
                constexpr int mask = batch_bool_constant<value_type, Values...>::mask();
                return _mm_blend_ps(b, a, mask);
#else
                return select((batch_bool_type)cond, a, b);
#endif
            }

            static batch_bool_type isnan(const batch_type& x)
            {
                return _mm_cmpunord_ps(x, x);
            }

            static batch_type zip_lo(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_unpacklo_ps(lhs, rhs);
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_unpackhi_ps(lhs, rhs);
            }
        };
    }
}

#endif
