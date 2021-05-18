/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_SSE_INT16_HPP
#define XSIMD_SSE_INT16_HPP

#include <cstdint>

#include "xsimd_base.hpp"
#include "xsimd_sse_int_base.hpp"

namespace xsimd
{

    /**************************
     * batch_bool<int16_t, 8> *
     **************************/

    template <>
    struct simd_batch_traits<batch_bool<int16_t, 8>>
    {
        using value_type = int16_t;
        static constexpr std::size_t size = 8;
        using batch_type = batch<int16_t, 8>;
        static constexpr std::size_t align = 16;
    };

    template <>
    struct simd_batch_traits<batch_bool<uint16_t, 8>>
    {
        using value_type = uint16_t;
        static constexpr std::size_t size = 8;
        using batch_type = batch<uint16_t, 8>;
        static constexpr std::size_t align = 16;
    };

    template <>
    class batch_bool<int16_t, 8> : public sse_int_batch_bool<int16_t, 8>
    {
    public:
        using sse_int_batch_bool::sse_int_batch_bool;
    };

    template <>
    class batch_bool<uint16_t, 8> : public sse_int_batch_bool<uint16_t, 8>
    {
    public:
        using sse_int_batch_bool::sse_int_batch_bool;
    };

    namespace detail
    {
        template <>
        struct batch_bool_kernel<int16_t, 8>
            : public sse_int_batch_bool_kernel<int16_t>
        {
        };

        template <>
        struct batch_bool_kernel<uint16_t, 8>
            : public sse_int_batch_bool_kernel<uint16_t>
        {
        };
    }

    /*********************
     * batch<int16_t, 8> *
     *********************/

    template <>
    struct simd_batch_traits<batch<int16_t, 8>>
    {
        using value_type = int16_t;
        static constexpr std::size_t size = 8;
        using batch_bool_type = batch_bool<int16_t, 8>;
        static constexpr std::size_t align = 16;
        using storage_type = __m128i;
    };

    template <>
    struct simd_batch_traits<batch<uint16_t, 8>>
    {
        using value_type = uint16_t;
        static constexpr std::size_t size = 8;
        using batch_bool_type = batch_bool<uint16_t, 8>;
        static constexpr std::size_t align = 16;
        using storage_type = __m128i;
    };

    template <>
    class batch<int16_t, 8> : public sse_int_batch<int16_t, 8>
    {
    public:

        using base_class = sse_int_batch<int16_t, 8>;
        using base_class::base_class;
        using base_class::load_aligned;
        using base_class::load_unaligned;
        using base_class::store_aligned;
        using base_class::store_unaligned;

        XSIMD_DECLARE_LOAD_STORE_INT16(int16_t, 8)
        XSIMD_DECLARE_LOAD_STORE_LONG(int16_t, 8)
    };

    template <>
    class batch<uint16_t, 8> : public sse_int_batch<uint16_t, 8>
    {
    public:

        using base_class = sse_int_batch<uint16_t, 8>;
        using base_class::base_class;
        using base_class::load_aligned;
        using base_class::load_unaligned;
        using base_class::store_aligned;
        using base_class::store_unaligned;

        XSIMD_DECLARE_LOAD_STORE_INT16(uint16_t, 8)
        XSIMD_DECLARE_LOAD_STORE_LONG(uint16_t, 8)
    };

    batch<int16_t, 8> operator<<(const batch<int16_t, 8>& lhs, int32_t rhs);
    batch<int16_t, 8> operator>>(const batch<int16_t, 8>& lhs, int32_t rhs);
    batch<int16_t, 8> operator<<(const batch<int16_t, 8>& lhs, const batch<int16_t, 8>& rhs);
    batch<int16_t, 8> operator>>(const batch<int16_t, 8>& lhs, const batch<int16_t, 8>& rhs);
    batch<uint16_t, 8> operator<<(const batch<uint16_t, 8>& lhs, int32_t rhs);
    batch<uint16_t, 8> operator>>(const batch<uint16_t, 8>& lhs, int32_t rhs);
    batch<uint16_t, 8> operator<<(const batch<uint16_t, 8>& lhs, const batch<int16_t, 8>& rhs);
    batch<uint16_t, 8> operator>>(const batch<uint16_t, 8>& lhs, const batch<int16_t, 8>& rhs);

    /************************************
     * batch<int16_t, 8> implementation *
     ************************************/

    namespace detail
    {
        template <class T>
        struct sse_int16_batch_kernel
            : sse_int_kernel_base<batch<T, 8>>
        {
            using batch_type = batch<T, 8>;
            using value_type = T;
            using batch_bool_type = batch_bool<T, 8>;

            static constexpr bool is_signed = std::is_signed<value_type>::value;

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_add_epi16(lhs, rhs);
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_sub_epi16(lhs, rhs);
            }

            static batch_type sadd(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_adds_epi16(lhs, rhs);
            }

            static batch_type ssub(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_subs_epi16(lhs, rhs);
            }

            static batch_type mul(const batch_type& lhs, const batch_type& rhs)
            {
            	return _mm_mullo_epi16(lhs, rhs);
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_cmpeq_epi16(lhs, rhs);
            }

            static value_type hadd(const batch_type& rhs)
            {
                // TODO implement with hadd_epi16
                alignas(16) T tmp[8];
                rhs.store_aligned(tmp);
                T res = 0;
                for (int i = 0; i < 8; ++i)
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
                return _mm_unpacklo_epi16(lhs, rhs);
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_unpackhi_epi16(lhs, rhs);
            }
        };

        template <>
        struct batch_kernel<int16_t, 8>
            : sse_int16_batch_kernel<int16_t>
        {
            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_cmplt_epi16(lhs, rhs);
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_min_epi16(lhs, rhs);
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_max_epi16(lhs, rhs);
            }

            static batch_type abs(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSSE3_VERSION
                return _mm_abs_epi16(rhs);
#else
                __m128i tmp, res;
                tmp = _mm_cmplt_epi16(rhs, _mm_setzero_si128());
                res = _mm_xor_si128(rhs, tmp);
                res = _mm_sub_epi16(res, tmp);
                return res;
#endif
            }
        };

        template <>
        struct batch_kernel<uint16_t, 8>
            : public sse_int16_batch_kernel<uint16_t>
        {
            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_cmplt_epi16(_mm_xor_si128(lhs, _mm_set1_epi16(std::numeric_limits<int16_t>::min())),
                                       _mm_xor_si128(rhs, _mm_set1_epi16(std::numeric_limits<int16_t>::min())));
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
                return _mm_min_epu16(lhs, rhs);
#else
                return select(lhs < rhs, lhs, rhs);
#endif
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE4_1_VERSION
                return _mm_max_epu16(lhs, rhs);
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
                return _mm_adds_epu16(lhs, rhs);
            }

            static batch_type ssub(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm_subs_epu16(lhs, rhs);
            }
        };
    }

    XSIMD_DEFINE_LOAD_STORE_INT16(int16_t, 8, 16)
    XSIMD_DEFINE_LOAD_STORE_LONG(int16_t, 8, 16)

    inline batch<int16_t, 8> operator<<(const batch<int16_t, 8>& lhs, int32_t rhs)
    {
        return _mm_slli_epi16(lhs, rhs);
    }

    inline batch<int16_t, 8> operator>>(const batch<int16_t, 8>& lhs, int32_t rhs)
    {
        return _mm_srai_epi16(lhs, rhs);
    }

    inline batch<int16_t, 8> operator<<(const batch<int16_t, 8>& lhs, const batch<int16_t, 8>& rhs)
    {
#if defined(XSIMD_AVX512VL_AVAILABLE) && defined(XSIMD_AVX512BW_AVAILABLE)
        return _mm_sllv_epi16(lhs, rhs);
#else
        return sse_detail::shift_impl([](int16_t alhs, int16_t s) { return alhs << s; }, lhs, rhs);
#endif
    }

    inline batch<int16_t, 8> operator>>(const batch<int16_t, 8>& lhs, const batch<int16_t, 8>& rhs)
    {
#if defined(XSIMD_AVX512VL_AVAILABLE) && defined(XSIMD_AVX512BW_AVAILABLE)
        return _mm_srav_epi16(lhs, rhs);
#else
        return sse_detail::shift_impl([](int16_t alhs, int16_t s) { return alhs >> s; }, lhs, rhs);
#endif
    }

    XSIMD_DEFINE_LOAD_STORE_INT16(uint16_t, 8, 16)
    XSIMD_DEFINE_LOAD_STORE_LONG(uint16_t, 8, 16)

    inline batch<uint16_t, 8> operator<<(const batch<uint16_t, 8>& lhs, int32_t rhs)
    {
        return _mm_slli_epi16(lhs, rhs);
    }

    inline batch<uint16_t, 8> operator>>(const batch<uint16_t, 8>& lhs, int32_t rhs)
    {
        return _mm_srli_epi16(lhs, rhs);
    }

    inline batch<uint16_t, 8> operator<<(const batch<uint16_t, 8>& lhs, const batch<int16_t, 8>& rhs)
    {
#if defined(XSIMD_AVX512VL_AVAILABLE) && defined(XSIMD_AVX512BW_AVAILABLE)
        return _mm_sllv_epi16(lhs, rhs);
#else
        return sse_detail::shift_impl([](uint16_t alhs, int16_t s) { return alhs << s; }, lhs, rhs);
#endif
    }

    inline batch<uint16_t, 8> operator>>(const batch<uint16_t, 8>& lhs, const batch<int16_t, 8>& rhs)
    {
#if defined(XSIMD_AVX512VL_AVAILABLE) && defined(XSIMD_AVX512BW_AVAILABLE)
        return _mm_srlv_epi16(lhs, rhs);
#else
        return sse_detail::shift_impl([](uint16_t alhs, int16_t s) { return alhs >> s; }, lhs, rhs);
#endif
    }
}

#endif
