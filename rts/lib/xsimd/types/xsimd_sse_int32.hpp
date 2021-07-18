/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_SSE_INT32_HPP
#define XSIMD_SSE_INT32_HPP

#include <cstdint>

#include "xsimd_base.hpp"
#include "xsimd_sse_int_base.hpp"

namespace xsimd
{

    /**************************
     * batch_bool<int32_t, 4> *
     **************************/

    template <>
    struct simd_batch_traits<batch_bool<int32_t, 4>>
    {
        using value_type = int32_t;
        static constexpr std::size_t size = 4;
        using batch_type = batch<int32_t, 4>;
        static constexpr std::size_t align = 16;
    };

    template <>
    struct simd_batch_traits<batch_bool<uint32_t, 4>>
    {
        using value_type = uint32_t;
        static constexpr std::size_t size = 4;
        using batch_type = batch<uint32_t, 4>;
        static constexpr std::size_t align = 16;
    };

    template <>
    class batch_bool<int32_t, 4> : public sse_int_batch_bool<int32_t, 4>
    {
    public:
        using sse_int_batch_bool::sse_int_batch_bool;
    };

    template <>
    class batch_bool<uint32_t, 4> : public sse_int_batch_bool<uint32_t, 4>
    {
    public:
        using sse_int_batch_bool::sse_int_batch_bool;
    };

    namespace detail
    {
        template <>
        struct batch_bool_kernel<int32_t, 4> : public sse_int_batch_bool_kernel<int32_t>
        {
        };

        template <>
        struct batch_bool_kernel<uint32_t, 4> : public sse_int_batch_bool_kernel<uint32_t>
        {
        };
    }

    /*********************
     * batch<int32_t, 4> *
     *********************/

    template <>
    struct simd_batch_traits<batch<int32_t, 4>>
    {
        using value_type = int32_t;
        static constexpr std::size_t size = 4;
        using batch_bool_type = batch_bool<int32_t, 4>;
        static constexpr std::size_t align = 16;
        using storage_type = __m128i;
    };

    template <>
    struct simd_batch_traits<batch<uint32_t, 4>>
    {
        using value_type = uint32_t;
        static constexpr std::size_t size = 4;
        using batch_bool_type = batch_bool<uint32_t, 4>;
        static constexpr std::size_t align = 16;
        using storage_type = __m128i;
    };

    template <>
    class batch<int32_t, 4> : public sse_int_batch<int32_t, 4>
    {
    public:

        using base_type = sse_int_batch<int32_t, 4>;
        using base_type::base_type;
        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;

        XSIMD_DECLARE_LOAD_STORE_INT32(int32_t, 4)
        XSIMD_DECLARE_LOAD_STORE_LONG(int32_t, 4)
    };

    template <>
    class batch<uint32_t, 4> : public sse_int_batch<uint32_t, 4>
    {
    public:

        using base_type = sse_int_batch<uint32_t, 4>;
        using base_type::base_type;
        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;

        XSIMD_DECLARE_LOAD_STORE_INT32(uint32_t, 4)
        XSIMD_DECLARE_LOAD_STORE_LONG(uint32_t, 4)
    };

    batch<int32_t, 4> operator<<(const batch<int32_t, 4>& lhs, int32_t rhs);
    batch<int32_t, 4> operator>>(const batch<int32_t, 4>& lhs, int32_t rhs);
    batch<int32_t, 4> operator<<(const batch<int32_t, 4>& lhs, const batch<int32_t, 4>& rhs);
    batch<int32_t, 4> operator>>(const batch<int32_t, 4>& lhs, const batch<int32_t, 4>& rhs);
    batch<uint32_t, 4> operator<<(const batch<uint32_t, 4>& lhs, int32_t rhs);
    batch<uint32_t, 4> operator>>(const batch<uint32_t, 4>& lhs, int32_t rhs);
    batch<uint32_t, 4> operator<<(const batch<uint32_t, 4>& lhs, const batch<int32_t, 4>& rhs);
    batch<uint32_t, 4> operator>>(const batch<uint32_t, 4>& lhs, const batch<int32_t, 4>& rhs);

    /************************************
     * batch<int32_t, 4> implementation *
     ************************************/

    namespace sse_detail
    {
        inline __m128i load_aligned_int32(const int8_t* src)
        {
            __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
            __m128i res = _mm_cvtepi8_epi32(tmp);
#else
            __m128i mask = _mm_cmplt_epi8(tmp, _mm_set1_epi8(0));
            __m128i tmp1 = _mm_unpacklo_epi8(tmp, mask);
            mask = _mm_cmplt_epi16(tmp1, _mm_set1_epi16(0));
            __m128i res = _mm_unpacklo_epi16(tmp1, mask);
#endif
            return res;
        }

        inline __m128i load_aligned_int32(const uint8_t* src)
        {
            __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
            __m128i res = _mm_cvtepu8_epi32(tmp);
#else
            __m128i tmp2 = _mm_unpacklo_epi8(tmp, _mm_set1_epi8(0));
            __m128i res = _mm_unpacklo_epi16(tmp2, _mm_set1_epi16(0));
#endif
            return res;
        }

        inline __m128i load_aligned_int32(const int16_t* src)
        {

            __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
            __m128i res = _mm_cvtepi16_epi32(tmp);
#else
            __m128i mask = _mm_cmplt_epi16(tmp, _mm_set1_epi16(0));
            __m128i res = _mm_unpacklo_epi16(tmp, mask);
#endif
            return res;
        }

        inline __m128i load_aligned_int32(const uint16_t* src)
        {
            __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
            __m128i res = _mm_cvtepu16_epi32(tmp);
#else
            __m128i res = _mm_unpacklo_epi16(tmp, _mm_set1_epi16(0));
#endif
            return res;
        }

        inline void store_aligned_int32(__m128i src, int8_t* dst)
        {
            __m128i tmp1 = _mm_packs_epi32(src, src);
            __m128i tmp2 = _mm_packs_epi16(tmp1, tmp1);
            _mm_storel_epi64((__m128i*)dst, tmp2);
        }

        inline void store_aligned_int32(__m128i src, uint8_t* dst)
        {
            __m128i tmp1 = _mm_packs_epi32(src, src);
            __m128i tmp2 = _mm_packus_epi16(tmp1, tmp1);
            _mm_storel_epi64((__m128i*)dst, tmp2);
        }

        inline void store_aligned_int32(__m128i src, int16_t* dst)
        {
            __m128i tmp1 = _mm_packs_epi32(src, src);
            _mm_storel_epi64((__m128i*)dst, tmp1);
        }

        inline void store_aligned_int32(__m128i src, uint16_t* dst)
        {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
            __m128i tmp = _mm_packus_epi32(src, src);
            _mm_storel_epi64((__m128i*)dst, tmp);
#else
            alignas(16) int32_t tmp[4];
            _mm_store_si128((__m128i*)tmp, src);
            unroller<4>([&](std::size_t i){
                dst[i] = static_cast<uint16_t>(tmp[i]);
            });
#endif
        }
    }

#define SSE_DEFINE_LOAD_STORE_INT32(TYPE, CVT_TYPE)                            \
    inline batch<TYPE, 4>& batch<TYPE, 4>::load_aligned(const CVT_TYPE* src)   \
    {                                                                          \
        this->m_value = sse_detail::load_aligned_int32(src);                   \
        return *this;                                                          \
    }                                                                          \
    inline batch<TYPE, 4>& batch<TYPE, 4>::load_unaligned(const CVT_TYPE* src) \
    {                                                                          \
        return load_aligned(src);                                              \
    }                                                                          \
    inline void batch<TYPE, 4>::store_aligned(CVT_TYPE* dst) const             \
    {                                                                          \
        sse_detail::store_aligned_int32(this->m_value, dst);                   \
    }                                                                          \
    inline void batch<TYPE, 4>::store_unaligned(CVT_TYPE* dst) const           \
    {                                                                          \
        store_aligned(dst);                                                    \
    }

    XSIMD_DEFINE_LOAD_STORE(int32_t, 4, bool, 16)
    SSE_DEFINE_LOAD_STORE_INT32(int32_t, int8_t)
    SSE_DEFINE_LOAD_STORE_INT32(int32_t, uint8_t)
    SSE_DEFINE_LOAD_STORE_INT32(int32_t, int16_t)
    SSE_DEFINE_LOAD_STORE_INT32(int32_t, uint16_t)
    XSIMD_DEFINE_LOAD_STORE_LONG(int32_t, 4, 16)
    XSIMD_DEFINE_LOAD_STORE(int32_t, 4, int64_t, 16)
    XSIMD_DEFINE_LOAD_STORE(int32_t, 4, uint64_t, 16)

    inline batch<int32_t, 4>& batch<int32_t, 4>::load_aligned(const float* src)
    {
        this->m_value = _mm_cvtps_epi32(_mm_load_ps(src));
        return *this;
    }

    inline batch<int32_t, 4>& batch<int32_t, 4>::load_unaligned(const float* src)
    {
        this->m_value = _mm_cvtps_epi32(_mm_loadu_ps(src));
        return *this;
    }

    inline batch<int32_t, 4>& batch<int32_t, 4>::load_aligned(const double* src)
    {
        __m128i tmp1 = _mm_cvtpd_epi32(_mm_load_pd(src));
        __m128i tmp2 = _mm_cvtpd_epi32(_mm_load_pd(src + 2));
        this->m_value = _mm_unpacklo_epi64(tmp1, tmp2);
        return *this;
    }

    inline batch<int32_t, 4>& batch<int32_t, 4>::load_unaligned(const double* src)
    {
        __m128i tmp1 = _mm_cvtpd_epi32(_mm_loadu_pd(src));
        __m128i tmp2 = _mm_cvtpd_epi32(_mm_loadu_pd(src + 2));
        this->m_value = _mm_unpacklo_epi64(tmp1, tmp2);
        return *this;
    }

    inline void batch<int32_t, 4>::store_aligned(float* dst) const
    {
        _mm_store_ps(dst, _mm_cvtepi32_ps(this->m_value));
    }

    inline void batch<int32_t, 4>::store_unaligned(float* dst) const
    {
        _mm_storeu_ps(dst, _mm_cvtepi32_ps(this->m_value));
    }

    inline void batch<int32_t, 4>::store_aligned(double* dst) const
    {
        __m128d tmp1 = _mm_cvtepi32_pd(this->m_value);
        __m128d tmp2 = _mm_cvtepi32_pd(_mm_unpackhi_epi64(this->m_value, this->m_value));
        _mm_store_pd(dst, tmp1);
        _mm_store_pd(dst + 2, tmp2);
    }

    inline void batch<int32_t, 4>::store_unaligned(double* dst) const
    {
        __m128d tmp1 = _mm_cvtepi32_pd(this->m_value);
        __m128d tmp2 = _mm_cvtepi32_pd(_mm_unpackhi_epi64(this->m_value, this->m_value));
        _mm_storeu_pd(dst, tmp1);
        _mm_storeu_pd(dst + 2, tmp2);
    }

    XSIMD_DEFINE_LOAD_STORE(uint32_t, 4, bool, 16)
    SSE_DEFINE_LOAD_STORE_INT32(uint32_t, int8_t)
    SSE_DEFINE_LOAD_STORE_INT32(uint32_t, uint8_t)
    SSE_DEFINE_LOAD_STORE_INT32(uint32_t, int16_t)
    SSE_DEFINE_LOAD_STORE_INT32(uint32_t, uint16_t)
    XSIMD_DEFINE_LOAD_STORE_LONG(uint32_t, 4, 16)
    XSIMD_DEFINE_LOAD_STORE(uint32_t, 4, int64_t, 16)
    XSIMD_DEFINE_LOAD_STORE(uint32_t, 4, uint64_t, 16)
    XSIMD_DEFINE_LOAD_STORE(uint32_t, 4, float, 16)
    XSIMD_DEFINE_LOAD_STORE(uint32_t, 4, double, 16)

#undef SSE_DEFINE_LOAD_STORE_INT32

    namespace detail
    {
        template <class T>
        struct sse_int32_batch_kernel
            : sse_int_kernel_base<batch<T, 4>>
        {
            using batch_type = batch<T, 4>;
            using value_type = T;
            using batch_bool_type = batch_bool<T, 4>;

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_add_epi32(lhs, rhs);
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_sub_epi32(lhs, rhs);
            }

            static batch_type sadd(const batch_type& lhs, const batch_type& rhs)
            {
                batch_type mask = rhs >> (8 * sizeof(value_type) - 1);
                batch_type lhs_pos_branch = min(std::numeric_limits<value_type>::max() - rhs, lhs);
                batch_type lhs_neg_branch = max(std::numeric_limits<value_type>::min() - rhs, lhs);
                return rhs + select((typename batch_type::storage_type)mask, lhs_neg_branch, lhs_pos_branch);
            }

            static batch_type ssub(const batch_type& lhs, const batch_type& rhs)
            {
                return sadd(lhs, sub(_mm_setzero_si128(), rhs));
            }

            static batch_type mul(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
                return _mm_mullo_epi32(lhs, rhs);
#else
                __m128i a13 = _mm_shuffle_epi32(lhs, 0xF5);
                __m128i b13 = _mm_shuffle_epi32(rhs, 0xF5);
                __m128i prod02 = _mm_mul_epu32(lhs, rhs);
                __m128i prod13 = _mm_mul_epu32(a13, b13);
                __m128i prod01 = _mm_unpacklo_epi32(prod02, prod13);
                __m128i prod23 = _mm_unpackhi_epi32(prod02, prod13);
                return _mm_unpacklo_epi64(prod01, prod23);
#endif
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
#if defined(XSIMD_FAST_INTEGER_DIVISION)
                return _mm_cvttps_epi32(_mm_div_ps(_mm_cvtepi32_ps(lhs), _mm_cvtepi32_ps(rhs)));
#else
                XSIMD_MACRO_UNROLL_BINARY(/);
#endif
            }

            static batch_type mod(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_MACRO_UNROLL_BINARY(%);
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_cmpeq_epi32(lhs, rhs);
            }

            static value_type hadd(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSSE3_VERSION
                __m128i tmp1 = _mm_hadd_epi32(rhs, rhs);
                __m128i tmp2 = _mm_hadd_epi32(tmp1, tmp1);
                return _mm_cvtsi128_si32(tmp2);
#else
                __m128i tmp1 = _mm_shuffle_epi32(rhs, 0x0E);
                __m128i tmp2 = _mm_add_epi32(rhs, tmp1);
                __m128i tmp3 = _mm_shuffle_epi32(tmp2, 0x01);
                __m128i tmp4 = _mm_add_epi32(tmp2, tmp3);
                return _mm_cvtsi128_si32(tmp4);
#endif
            }

            static batch_type select(const batch_bool_type& cond, const batch_type& a, const batch_type& b)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
                return _mm_blendv_epi8(b, a, cond);
#else
                return _mm_or_si128(_mm_and_si128(cond, a), _mm_andnot_si128(cond, b));
#endif
            }

            static batch_type zip_lo(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_unpacklo_epi32(lhs, rhs);
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_unpackhi_epi32(lhs, rhs);
            }
        };

        template <>
        struct batch_kernel<int32_t, 4>
            : sse_int32_batch_kernel<int32_t>
        {
            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_cmplt_epi32(lhs, rhs);
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
                return _mm_min_epi32(lhs, rhs);
#else
                __m128i greater = _mm_cmpgt_epi32(lhs, rhs);
                return select(greater, rhs, lhs);
#endif
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
                return _mm_max_epi32(lhs, rhs);
#else
                __m128i greater = _mm_cmpgt_epi32(lhs, rhs);
                return select(greater, lhs, rhs);
#endif
            }

            static batch_type abs(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSSE3_VERSION
                return _mm_abs_epi32(rhs);
#else
                __m128i sign = _mm_srai_epi32(rhs, 31);
                __m128i inv = _mm_xor_si128(rhs, sign);
                return _mm_sub_epi32(inv, sign);
#endif
            }
        };

        template <>
        struct batch_kernel<uint32_t, 4>
            : sse_int32_batch_kernel<uint32_t>
        {
            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
                auto xlhs = _mm_xor_si128(lhs, _mm_set1_epi32(std::numeric_limits<int32_t>::lowest()));
                auto xrhs = _mm_xor_si128(rhs, _mm_set1_epi32(std::numeric_limits<int32_t>::lowest()));
                return _mm_cmplt_epi32(xlhs, xrhs);
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
                return _mm_min_epu32(lhs, rhs);
#else
                auto mask = lhs < rhs;
                return select(mask, lhs, rhs);
#endif
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
                return _mm_max_epu32(lhs, rhs);
#else
                auto mask = lhs < rhs;
                return select(mask, rhs, lhs);
#endif
            }

            static batch_type abs(const batch_type& rhs)
            {
                return rhs;
            }


            static batch_type sadd(const batch_type& lhs, const batch_type& rhs)
            {
                const auto diffmax = std::numeric_limits<value_type>::max() - lhs;
                const auto mindiff = min(diffmax, rhs);
                return lhs + mindiff;
            }

            static batch_type ssub(const batch_type& lhs, const batch_type& rhs)
            {
                const auto diff = min(lhs, rhs);
                return lhs - diff;
            }

        };
    }

    inline batch<int32_t, 4> operator<<(const batch<int32_t, 4>& lhs, int32_t rhs)
    {
        return _mm_slli_epi32(lhs, rhs);
    }

    inline batch<int32_t, 4> operator>>(const batch<int32_t, 4>& lhs, int32_t rhs)
    {
        return _mm_srai_epi32(lhs, rhs);
    }

    inline batch<int32_t, 4> operator<<(const batch<int32_t, 4>& lhs, const batch<int32_t, 4>& rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm_sllv_epi32(lhs, rhs);
#else
        return sse_detail::shift_impl([](int32_t alhs, int32_t s) { return alhs << s; }, lhs, rhs);
#endif
    }

    inline batch<int32_t, 4> operator>>(const batch<int32_t, 4>& lhs, const batch<int32_t, 4>& rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm_srav_epi32(lhs, rhs);
#else
        return sse_detail::shift_impl([](int32_t alhs, int32_t s) { return alhs >> s; }, lhs, rhs);
#endif
    }

    inline batch<uint32_t, 4> operator<<(const batch<uint32_t, 4>& lhs, int32_t rhs)
    {
        return _mm_slli_epi32(lhs, rhs);
    }

    inline batch<uint32_t, 4> operator>>(const batch<uint32_t, 4>& lhs, int32_t rhs)
    {
        return _mm_srli_epi32(lhs, rhs);
    }

    inline batch<uint32_t, 4> operator<<(const batch<uint32_t, 4>& lhs, const batch<int32_t, 4>& rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm_sllv_epi32(lhs, rhs);
#else
        return sse_detail::shift_impl([](uint32_t alhs, int32_t s) { return alhs << s; }, lhs, rhs);
#endif
    }

    inline batch<uint32_t, 4> operator>>(const batch<uint32_t, 4>& lhs, const batch<int32_t, 4>& rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm_srlv_epi32(lhs, rhs);
#else
        return sse_detail::shift_impl([](uint32_t alhs, int32_t s) { return alhs >> s; }, lhs, rhs);
#endif
    }
}

#endif
