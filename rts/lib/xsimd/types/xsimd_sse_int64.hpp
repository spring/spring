/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_SSE_INT64_HPP
#define XSIMD_SSE_INT64_HPP

#include <cstdint>

#include "xsimd_base.hpp"
#include "xsimd_sse_int_base.hpp"

namespace xsimd
{

    /**************************
     * batch_bool<int64_t, 2> *
     **************************/

    template <>
    struct simd_batch_traits<batch_bool<int64_t, 2>>
    {
        using value_type = int64_t;
        static constexpr std::size_t size = 2;
        using batch_type = batch<int64_t, 2>;
        static constexpr std::size_t align = 16;
    };

    template <>
    struct simd_batch_traits<batch_bool<uint64_t, 2>>
    {
        using value_type = uint64_t;
        static constexpr std::size_t size = 2;
        using batch_type = batch<uint64_t, 2>;
        static constexpr std::size_t align = 16;
    };

    template <>
    class batch_bool<int64_t, 2> : public sse_int_batch_bool<int64_t, 2>
    {
    public:
        using sse_int_batch_bool::sse_int_batch_bool;
    };

    template <>
    class batch_bool<uint64_t, 2> : public sse_int_batch_bool<uint64_t, 2>
    {
    public:
        using sse_int_batch_bool::sse_int_batch_bool;
    };

    namespace detail
    {
        template <>
        struct batch_bool_kernel<int64_t, 2> : public sse_int_batch_bool_kernel<int64_t>
        {
        };

        template <>
        struct batch_bool_kernel<uint64_t, 2> : public sse_int_batch_bool_kernel<uint64_t>
        {
        };
    }

    /*********************
     * batch<int64_t, 2> *
     *********************/

    template <>
    struct simd_batch_traits<batch<int64_t, 2>>
    {
        using value_type = int64_t;
        static constexpr std::size_t size = 2;
        using batch_bool_type = batch_bool<int64_t, 2>;
        static constexpr std::size_t align = 16;
        using storage_type = __m128i;
    };

    template <>
    struct simd_batch_traits<batch<uint64_t, 2>>
    {
        using value_type = uint64_t;
        static constexpr std::size_t size = 2;
        using batch_bool_type = batch_bool<uint64_t, 2>;
        static constexpr std::size_t align = 16;
        using storage_type = __m128i;
    };

    template <>
    class batch<int64_t, 2> : public sse_int_batch<int64_t, 2>
    {
    public:

        using base_type = sse_int_batch<int64_t, 2>;
        using base_type::base_type;
        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;

        XSIMD_DECLARE_LOAD_STORE_INT64(int64_t, 2)
        XSIMD_DECLARE_LOAD_STORE_LONG(int64_t, 2)
   };

    template <>
    class batch<uint64_t, 2> : public sse_int_batch<uint64_t, 2>
    {
    public:

        using base_type = sse_int_batch<uint64_t, 2>;
        using base_type::base_type;
        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;

        XSIMD_DECLARE_LOAD_STORE_INT64(uint64_t, 2)
        XSIMD_DECLARE_LOAD_STORE_LONG(uint64_t, 2)
    };

    batch<int64_t, 2> operator<<(const batch<int64_t, 2>& lhs, int32_t rhs);
    batch<int64_t, 2> operator>>(const batch<int64_t, 2>& lhs, int32_t rhs);
    batch<int64_t, 2> operator<<(const batch<int64_t, 2>& lhs, const batch<int64_t, 2>& rhs);
    batch<int64_t, 2> operator>>(const batch<int64_t, 2>& lhs, const batch<int64_t, 2>& rhs);
    batch<uint64_t, 2> operator<<(const batch<uint64_t, 2>& lhs, int32_t rhs);
    batch<uint64_t, 2> operator>>(const batch<uint64_t, 2>& lhs, int32_t rhs);
    batch<uint64_t, 2> operator<<(const batch<uint64_t, 2>& lhs, const batch<int64_t, 2>& rhs);
    batch<uint64_t, 2> operator>>(const batch<uint64_t, 2>& lhs, const batch<int64_t, 2>& rhs);

    /************************************
     * batch<int64_t, 2> implementation *
     ************************************/

    namespace sse_detail
    {
        inline __m128i load_aligned_int64(const int8_t* src)
        {
            __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
            __m128i res = _mm_cvtepi8_epi64(tmp);
#else
            __m128i mask = _mm_cmplt_epi8(tmp, _mm_set1_epi8(0));
            __m128i tmp1 = _mm_unpacklo_epi8(tmp, mask);
            mask = _mm_cmplt_epi16(tmp1, _mm_set1_epi16(0));
            __m128i tmp2 = _mm_unpacklo_epi16(tmp1, mask);
            mask = _mm_cmplt_epi32(tmp2, _mm_set1_epi32(0));
            __m128i res = _mm_unpacklo_epi32(tmp2, mask);
#endif
            return res;
        }

        inline __m128i load_aligned_int64(const uint8_t* src)
        {
            __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
            __m128i res = _mm_cvtepu8_epi64(tmp);
#else
            __m128i tmp1 = _mm_unpacklo_epi8(tmp, _mm_set1_epi8(0));
            __m128i tmp2 = _mm_unpacklo_epi16(tmp1, _mm_set1_epi16(0));
            __m128i res = _mm_unpacklo_epi32(tmp2, _mm_set1_epi32(0));
#endif
            return res;
        }

        inline __m128i load_aligned_int64(const int16_t* src)
        {
            __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
            __m128i res = _mm_cvtepi16_epi64(tmp);
#else
            __m128i mask = _mm_cmplt_epi16(tmp, _mm_set1_epi16(0));
            __m128i tmp1 = _mm_unpacklo_epi16(tmp, mask);
            mask = _mm_cmplt_epi32(tmp1, _mm_set1_epi32(0));
            __m128i res = _mm_unpacklo_epi32(tmp1, mask);
#endif
            return res;
        }

        inline __m128i load_aligned_int64(const uint16_t* src)
        {
            __m128i tmp = _mm_loadl_epi64((const __m128i*)src);
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
            __m128i res = _mm_cvtepu16_epi64(tmp);
#else
            __m128i tmp1 = _mm_unpacklo_epi16(tmp, _mm_set1_epi16(0));
            __m128i res = _mm_unpacklo_epi32(tmp1, _mm_set1_epi32(0));
#endif
            return res;
        }

        inline void store_aligned_int64(__m128i src, int8_t* dst)
        {
            alignas(16) int64_t tmp[2];
            _mm_store_si128((__m128i*)tmp, src);
            dst[0] = static_cast<int8_t>(tmp[0]);
            dst[1] = static_cast<int8_t>(tmp[1]);
        }

        inline void store_aligned_int64(__m128i src, uint8_t* dst)
        {
            alignas(16) int64_t tmp[2];
            _mm_store_si128((__m128i*)tmp, src);
            dst[0] = static_cast<uint8_t>(tmp[0]);
            dst[1] = static_cast<uint8_t>(tmp[1]);
        }

        inline void store_aligned_int64(__m128i src, int16_t* dst)
        {
            alignas(16) int64_t tmp[2];
            _mm_store_si128((__m128i*)tmp, src);
            dst[0] = static_cast<int16_t>(tmp[0]);
            dst[1] = static_cast<int16_t>(tmp[1]);
        }

        inline void store_aligned_int64(__m128i src, uint16_t* dst)
        {
            alignas(16) int64_t tmp[2];
            _mm_store_si128((__m128i*)tmp, src);
            dst[0] = static_cast<uint16_t>(tmp[0]);
            dst[1] = static_cast<uint16_t>(tmp[1]);
        }
    }

#define SSE_DEFINE_LOAD_STORE_INT64(TYPE, CVT_TYPE)                            \
    inline batch<TYPE, 2>& batch<TYPE, 2>::load_aligned(const CVT_TYPE* src)   \
    {                                                                          \
        this->m_value = sse_detail::load_aligned_int64(src);                   \
        return *this;                                                          \
    }                                                                          \
    inline batch<TYPE, 2>& batch<TYPE, 2>::load_unaligned(const CVT_TYPE* src) \
    {                                                                          \
        return load_aligned(src);                                              \
    }                                                                          \
    inline void batch<TYPE, 2>::store_aligned(CVT_TYPE* dst) const             \
    {                                                                          \
        sse_detail::store_aligned_int64(this->m_value, dst);                   \
    }                                                                          \
    inline void batch<TYPE, 2>::store_unaligned(CVT_TYPE* dst) const           \
    {                                                                          \
        store_aligned(dst);                                                    \
    }

    XSIMD_DEFINE_LOAD_STORE(int64_t, 2, bool, 16)
    SSE_DEFINE_LOAD_STORE_INT64(int64_t, int8_t)
    SSE_DEFINE_LOAD_STORE_INT64(int64_t, uint8_t)
    SSE_DEFINE_LOAD_STORE_INT64(int64_t, int16_t)
    SSE_DEFINE_LOAD_STORE_INT64(int64_t, uint16_t)
    XSIMD_DEFINE_LOAD_STORE(int64_t, 2, int32_t, 16)
    XSIMD_DEFINE_LOAD_STORE(int64_t, 2, uint32_t, 16)
    XSIMD_DEFINE_LOAD_STORE_LONG(int64_t, 2, 16)
    XSIMD_DEFINE_LOAD_STORE(int64_t, 2, float, 16)
    XSIMD_DEFINE_LOAD_STORE(int64_t, 2, double, 16)

    XSIMD_DEFINE_LOAD_STORE(uint64_t, 2, bool, 16)
    SSE_DEFINE_LOAD_STORE_INT64(uint64_t, int8_t)
    SSE_DEFINE_LOAD_STORE_INT64(uint64_t, uint8_t)
    SSE_DEFINE_LOAD_STORE_INT64(uint64_t, int16_t)
    SSE_DEFINE_LOAD_STORE_INT64(uint64_t, uint16_t)
    XSIMD_DEFINE_LOAD_STORE(uint64_t, 2, int32_t, 16)
    XSIMD_DEFINE_LOAD_STORE(uint64_t, 2, uint32_t, 16)
    XSIMD_DEFINE_LOAD_STORE_LONG(uint64_t, 2, 16)
    XSIMD_DEFINE_LOAD_STORE(uint64_t, 2, float, 16)
    XSIMD_DEFINE_LOAD_STORE(uint64_t, 2, double, 16)

#undef SSE_DEFINE_LOAD_STORE_INT64

    namespace detail
    {
        template <class T>
        struct sse_int64_batch_kernel
            : sse_int_kernel_base<batch<T, 2>>
        {
            using batch_type = batch<T, 2>;
            using value_type = T;
            using batch_bool_type = batch_bool<T, 2>;

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_add_epi64(lhs, rhs);
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_sub_epi64(lhs, rhs);
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
                XSIMD_MACRO_UNROLL_BINARY(*);
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
#if defined(XSIMD_FAST_INTEGER_DIVISION)
                __m128d dlhs = _mm_setr_pd(static_cast<double>(lhs[0]), static_cast<double>(lhs[1]));
                __m128d drhs = _mm_setr_pd(static_cast<double>(rhs[0]), static_cast<double>(rhs[1]));
                __m128i tmp = _mm_cvttpd_epi32(_mm_div_pd(dlhs, drhs));
                using batch_int = batch<int64_t, 2>;
                return _mm_unpacklo_epi32(tmp, batch_int(tmp) < batch_int(int64_t(0)));
#else
                XSIMD_MACRO_UNROLL_BINARY(/);
#endif

            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
                return _mm_cmpeq_epi64(lhs, rhs);
#else
                return sse_detail::cmpeq_epi64_sse2(lhs, rhs);
#endif
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
                return select(lhs < rhs, lhs, rhs);
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
                return select(lhs > rhs, lhs, rhs);
            }

            static value_type hadd(const batch_type& rhs)
            {
                __m128i tmp1 = _mm_shuffle_epi32(rhs, 0x0E);
                __m128i tmp2 = _mm_add_epi64(rhs, tmp1);
#if defined(__x86_64__)
                return _mm_cvtsi128_si64(tmp2);
#else
                union {
                    int64_t i;
                    __m128i m;
                } u;
                _mm_storel_epi64(&u.m, tmp2);
                return u.i;
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
                return _mm_unpacklo_epi64(lhs, rhs);
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_unpackhi_epi64(lhs, rhs);
            }
        };


        template <>
        struct batch_kernel<int64_t, 2>
            : sse_int64_batch_kernel<int64_t>
        {
            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_2_VERSION
                return _mm_cmpgt_epi64(rhs, lhs);
#else
                __m128i tmp1 = _mm_sub_epi64(lhs, rhs);
                __m128i tmp2 = _mm_xor_si128(lhs, rhs);
                __m128i tmp3 = _mm_andnot_si128(rhs, lhs);
                __m128i tmp4 = _mm_andnot_si128(tmp2, tmp1);
                __m128i tmp5 = _mm_or_si128(tmp3, tmp4);
                __m128i tmp6 = _mm_srai_epi32(tmp5, 31);
                return _mm_shuffle_epi32(tmp6, 0xF5);
#endif
            }

            static batch_type abs(const batch_type& rhs)
            {
#if defined(XSIMD_AVX512VL_AVAILABLE)
                return _mm_abs_epi64(rhs);
#else
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_2_VERSION
                __m128i sign = _mm_cmpgt_epi64(_mm_setzero_si128(), rhs);
#else
                __m128i signh = _mm_srai_epi32(rhs, 31);
                __m128i sign = _mm_shuffle_epi32(signh, 0xF5);
#endif
                __m128i inv = _mm_xor_si128(rhs, sign);
                return _mm_sub_epi64(inv, sign);
#endif
            }
        };

        template <>
        struct batch_kernel<uint64_t, 2>
            : sse_int64_batch_kernel<uint64_t>
        {
            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
                auto xlhs = _mm_xor_si128(lhs, _mm_set1_epi64x(std::numeric_limits<int64_t>::lowest()));
                auto xrhs = _mm_xor_si128(rhs, _mm_set1_epi64x(std::numeric_limits<int64_t>::lowest()));

#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_2_VERSION
                return _mm_cmpgt_epi64(xrhs, xlhs);
#else
                __m128i tmp1 = _mm_sub_epi64(xlhs, xrhs);
                __m128i tmp2 = _mm_xor_si128(xlhs, xrhs);
                __m128i tmp3 = _mm_andnot_si128(xrhs, xlhs);
                __m128i tmp4 = _mm_andnot_si128(tmp2, tmp1);
                __m128i tmp5 = _mm_or_si128(tmp3, tmp4);
                __m128i tmp6 = _mm_srai_epi32(tmp5, 31);
                return _mm_shuffle_epi32(tmp6, 0xF5);
#endif
            }

            static batch_type abs(const batch_type& rhs)
            {
                return rhs;
            }

            static batch_type sadd(const batch_type& lhs, const batch_type& rhs)
            {
                const auto diffmax = batch_type(std::numeric_limits<value_type>::max()) - lhs;
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

    inline batch<int64_t, 2> operator<<(const batch<int64_t, 2>& lhs, int32_t rhs)
    {
        return _mm_slli_epi64(lhs, rhs);
    }

    inline batch<int64_t, 2> operator>>(const batch<int64_t, 2>& lhs, int32_t rhs)
    {
#if defined(XSIMD_AVX512VL_AVAILABLE)
        return _mm_srai_epi64(lhs, rhs);
#else
        return sse_detail::shift_impl([](int64_t alhs, int32_t s) { return alhs >> s; }, lhs, rhs);
#endif
    }

    inline batch<int64_t, 2> operator<<(const batch<int64_t, 2>& lhs, const batch<int64_t, 2>& rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm_sllv_epi64(lhs, rhs);
#else
        return sse_detail::shift_impl([](int64_t alhs, int64_t s) { return alhs << s; }, lhs, rhs);
#endif
    }

    inline batch<int64_t, 2> operator>>(const batch<int64_t, 2>& lhs, const batch<int64_t, 2>& rhs)
    {
#if defined(XSIMD_AVX512VL_AVAILABLE)
        return _mm_srav_epi64(lhs, rhs);
#else
        return sse_detail::shift_impl([](int64_t alhs, int64_t s) { return alhs >> s; }, lhs, rhs);
#endif
    }

    inline batch<uint64_t, 2> operator<<(const batch<uint64_t, 2>& lhs, int32_t rhs)
    {
        return _mm_slli_epi64(lhs, rhs);
    }

    inline batch<uint64_t, 2> operator>>(const batch<uint64_t, 2>& lhs, int32_t rhs)
    {
        return _mm_srli_epi64(lhs, rhs);
    }

    inline batch<uint64_t, 2> operator<<(const batch<uint64_t, 2>& lhs, const batch<int64_t, 2>& rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm_sllv_epi64(lhs, rhs);
#else
        return sse_detail::shift_impl([](uint64_t alhs, int64_t s) { return alhs << s; }, lhs, rhs);
#endif
    }

    inline batch<uint64_t, 2> operator>>(const batch<uint64_t, 2>& lhs, const batch<int64_t, 2>& rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm_srlv_epi64(lhs, rhs);
#else
        return sse_detail::shift_impl([](uint64_t alhs, int64_t s) { return alhs >> s; }, lhs, rhs);
#endif
    }
}

#endif
