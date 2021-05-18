/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_SSE_INT8_HPP
#define XSIMD_SSE_INT8_HPP

#include <cstdint>
#include <limits>

#include "xsimd_base.hpp"
#include "xsimd_sse_int_base.hpp"

namespace xsimd
{

    /**************************
     * batch_bool<int8_t, 16> *
     **************************/

    template <>
    struct simd_batch_traits<batch_bool<int8_t, 16>>
    {
        using value_type = int8_t;
        static constexpr std::size_t size = 16;
        using batch_type = batch<int8_t, 16>;
        static constexpr std::size_t align = 16;
    };

    template <>
    struct simd_batch_traits<batch_bool<uint8_t, 16>>
    {
        using value_type = uint8_t;
        static constexpr std::size_t size = 16;
        using batch_type = batch<uint8_t, 16>;
        static constexpr std::size_t align = 16;
    };

    template <>
    class batch_bool<int8_t, 16> : public sse_int_batch_bool<int8_t, 16>
    {
    public:
        using sse_int_batch_bool::sse_int_batch_bool;
    };

    template <>
    class batch_bool<uint8_t, 16> : public sse_int_batch_bool<uint8_t, 16>
    {
    public:
        using sse_int_batch_bool::sse_int_batch_bool;
    };

    namespace detail
    {
        template <>
        struct batch_bool_kernel<int8_t, 16>
            : public sse_int_batch_bool_kernel<int8_t>
        {
        };

        template <>
        struct batch_bool_kernel<uint8_t, 16>
            : public sse_int_batch_bool_kernel<uint8_t>
        {
        };
    }

    /*********************
     * batch<int8_t, 16> *
     *********************/

    template <>
    struct simd_batch_traits<batch<int8_t, 16>>
    {
        using value_type = int8_t;
        static constexpr std::size_t size = 16;
        using batch_bool_type = batch_bool<int8_t, 16>;
        static constexpr std::size_t align = 16;
        using storage_type = __m128i;
    };

    template <>
    struct simd_batch_traits<batch<uint8_t, 16>>
    {
        using value_type = uint8_t;
        static constexpr std::size_t size = 16;
        using batch_bool_type = batch_bool<uint8_t, 16>;
        static constexpr std::size_t align = 16;
        using storage_type = __m128i;
    };

    template <>
    class batch<int8_t, 16> : public sse_int_batch<int8_t, 16>
    {
    public:

        using base_class = sse_int_batch<int8_t, 16>;
        using base_class::base_class;
        using base_class::load_aligned;
        using base_class::load_unaligned;
        using base_class::store_aligned;
        using base_class::store_unaligned;

        batch() = default;

        explicit batch(const char* src)
            : batch(reinterpret_cast<const int8_t*>(src))
        {
        }

        batch(const char* src, aligned_mode)
            : batch(reinterpret_cast<const int8_t*>(src), aligned_mode{})
        {
        }

        batch(const char* src, unaligned_mode)
            : batch(reinterpret_cast<const int8_t*>(src), unaligned_mode{})
        {
        }

        XSIMD_DECLARE_LOAD_STORE_INT8(int8_t, 16)
        XSIMD_DECLARE_LOAD_STORE_LONG(int8_t, 16)
    };

    template <>
    class batch<uint8_t, 16> : public sse_int_batch<uint8_t, 16>
    {
    public:

        using base_class = sse_int_batch<uint8_t, 16>;
        using base_class::base_class;
        using base_class::load_aligned;
        using base_class::load_unaligned;
        using base_class::store_aligned;
        using base_class::store_unaligned;

        XSIMD_DECLARE_LOAD_STORE_INT8(uint8_t, 16)
        XSIMD_DECLARE_LOAD_STORE_LONG(uint8_t, 16)
    };

    batch<int8_t, 16> operator<<(const batch<int8_t, 16>& lhs, int32_t rhs);
    batch<int8_t, 16> operator>>(const batch<int8_t, 16>& lhs, int32_t rhs);
    batch<int8_t, 16> operator<<(const batch<int8_t, 16>& lhs, const batch<int8_t, 16>& rhs);
    batch<int8_t, 16> operator>>(const batch<int8_t, 16>& lhs, const batch<int8_t, 16>& rhs);
    batch<uint8_t, 16> operator<<(const batch<uint8_t, 16>& lhs, int32_t rhs);
    batch<uint8_t, 16> operator>>(const batch<uint8_t, 16>& lhs, int32_t rhs);
    batch<uint8_t, 16> operator<<(const batch<uint8_t, 16>& lhs, const batch<int8_t, 16>& rhs);
    batch<uint8_t, 16> operator>>(const batch<uint8_t, 16>& lhs, const batch<int8_t, 16>& rhs);

    /************************************
     * batch<int8_t, 16> implementation *
     ************************************/

    namespace detail
    {
        template <class T>
        struct sse_int8_batch_kernel
            : sse_int_kernel_base<batch<T, 16>>
        {
            using batch_type = batch<T, 16>;
            using value_type = T;
            using batch_bool_type = batch_bool<T, 16>;

            static constexpr bool is_signed = std::is_signed<value_type>::value;

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_add_epi8(lhs, rhs);
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_sub_epi8(lhs, rhs);
            }

            static batch_type sadd(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_adds_epi8(lhs, rhs);
            }

            static batch_type ssub(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_subs_epi8(lhs,rhs);
            }


            static batch_type mul(const batch_type& lhs, const batch_type& rhs)
            {
                return sse_int8_batch_kernel::bitwise_or(
                    sse_int8_batch_kernel::bitwise_and(_mm_mullo_epi16(lhs, rhs), _mm_srli_epi16(_mm_set1_epi32(-1), 8)),
                    _mm_slli_epi16(_mm_mullo_epi16(_mm_srli_si128(lhs, 1), _mm_srli_si128(rhs, 1)), 8)
                );
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_cmpeq_epi8(lhs, rhs);
            }

            static value_type hadd(const batch_type& rhs)
            {
                // TODO implement with hadd_epi16
                alignas(16) T tmp[16];
                rhs.store_aligned(tmp);
                T res = 0;
                for (int i = 0; i < 16; ++i)
                {
                    res += tmp[i];
                }
                return res;
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
                return _mm_unpacklo_epi8(lhs, rhs);
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_unpackhi_epi8(lhs, rhs);
            }
        };

        template <>
        struct batch_kernel<int8_t, 16>
            : sse_int8_batch_kernel<int8_t>
        {
            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_cmplt_epi8(lhs, rhs);
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
                return _mm_min_epi8(lhs, rhs);
#else
                __m128i greater = _mm_cmpgt_epi8(lhs, rhs);
                return select(greater, rhs, lhs);
#endif
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
                return _mm_max_epi8(lhs, rhs);
#else
                __m128i greater = _mm_cmpgt_epi8(lhs, rhs);
                return select(greater, lhs, rhs);
#endif
            }


            static batch_type abs(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSSE3_VERSION
                return _mm_abs_epi8(rhs);
#else
                __m128i neg = _mm_sub_epi8(_mm_setzero_si128(), rhs);
                return _mm_min_epu8(rhs, neg);
#endif
            }
        };

        template <>
        struct batch_kernel<uint8_t, 16>
            : public sse_int8_batch_kernel<uint8_t>
        {
            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_cmplt_epi8(_mm_xor_si128(lhs, _mm_set1_epi8(std::numeric_limits<int8_t>::min())),
                                      _mm_xor_si128(rhs, _mm_set1_epi8(std::numeric_limits<int8_t>::min())));
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE2_VERSION
                    return _mm_min_epu8(lhs, rhs);
#else
                    return select(lhs < rhs, lhs, rhs);
#endif
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE2_VERSION
                    return _mm_max_epu8(lhs, rhs);
#else
                    return select(lhs < rhs, rhs, lhs);
#endif
            }

            static batch_type abs(const batch_type& rhs)
            {
                return rhs;
            }

            static batch_type sadd(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_adds_epu8(lhs, rhs);
            }

            static batch_type ssub(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_subs_epu8(lhs,rhs);
            }


        };
    }

    XSIMD_DEFINE_LOAD_STORE_INT8(int8_t, 16, 16)
    XSIMD_DEFINE_LOAD_STORE_LONG(int8_t, 16, 16)

    inline batch<int8_t, 16> operator<<(const batch<int8_t, 16>& lhs, int32_t rhs)
    {
        return _mm_and_si128(_mm_set1_epi8(0xFF << rhs), _mm_slli_epi32(lhs, rhs));
    }

    inline batch<int8_t, 16> operator>>(const batch<int8_t, 16>& lhs, int32_t rhs)
    {
        __m128i sign_mask = _mm_set1_epi16((0xFF00 >> rhs) & 0x00FF);
        __m128i cmp_is_negative = _mm_cmpgt_epi8(_mm_setzero_si128(), lhs);
        __m128i res = _mm_srai_epi16(lhs, rhs);
        return _mm_or_si128(_mm_and_si128(sign_mask, cmp_is_negative), _mm_andnot_si128(sign_mask, res));
    }

    inline batch<int8_t, 16> operator<<(const batch<int8_t, 16>& lhs, const batch<int8_t, 16>& rhs)
    {
        return sse_detail::shift_impl([](int8_t alhs, int8_t s) { return alhs << s; }, lhs, rhs);
    }

    inline batch<int8_t, 16> operator>>(const batch<int8_t, 16>& lhs, const batch<int8_t, 16>& rhs)
    {
        return sse_detail::shift_impl([](int8_t alhs, int8_t s) { return alhs >> s; }, lhs, rhs);
    }

    XSIMD_DEFINE_LOAD_STORE_INT8(uint8_t, 16, 16)
    XSIMD_DEFINE_LOAD_STORE_LONG(uint8_t, 16, 16)

    inline batch<uint8_t, 16> operator<<(const batch<uint8_t, 16>& lhs, int32_t rhs)
    {
        return _mm_and_si128(_mm_set1_epi8(0xFF << rhs), _mm_slli_epi32(lhs, rhs));
    }

    inline batch<uint8_t, 16> operator>>(const batch<uint8_t, 16>& lhs, int32_t rhs)
    {
        return _mm_and_si128(_mm_set1_epi8(0xFF >> rhs), _mm_srli_epi32(lhs, rhs));
    }

    inline batch<uint8_t, 16> operator<<(const batch<uint8_t, 16>& lhs, const batch<int8_t, 16>& rhs)
    {
        return sse_detail::shift_impl([](uint8_t alhs, int8_t s) { return alhs << s; }, lhs, rhs);
    }

    inline batch<uint8_t, 16> operator>>(const batch<uint8_t, 16>& lhs, const batch<int8_t, 16>& rhs)
    {
        return sse_detail::shift_impl([](uint8_t alhs, int8_t s) { return alhs >> s; }, lhs, rhs);
    }
}

#endif
