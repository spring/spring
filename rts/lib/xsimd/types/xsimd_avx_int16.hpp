/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_AVX_INT16_HPP
#define XSIMD_AVX_INT16_HPP

#include <cstdint>

#include "xsimd_base.hpp"
#include "xsimd_avx_int_base.hpp"

namespace xsimd
{

    /***************************
     * batch_bool<int16_t, 16> *
     ***************************/

    template <>
    struct simd_batch_traits<batch_bool<int16_t, 16>>
    {
        using value_type = int16_t;
        static constexpr std::size_t size = 16;
        using batch_type = batch<int16_t, 16>;
        static constexpr std::size_t align = 32;
    };

    template <>
    struct simd_batch_traits<batch_bool<uint16_t, 16>>
    {
        using value_type = uint16_t;
        static constexpr std::size_t size = 16;
        using batch_type = batch<uint16_t, 16>;
        static constexpr std::size_t align = 32;
    };

    template <>
    class batch_bool<int16_t, 16> : public avx_int_batch_bool<int16_t, 16>
    {
    public:
        using avx_int_batch_bool::avx_int_batch_bool;
    };

    template <>
    class batch_bool<uint16_t, 16> : public avx_int_batch_bool<uint16_t, 16>
    {
    public:
        using avx_int_batch_bool::avx_int_batch_bool;
    };

    namespace detail
    {
        template <>
        struct batch_bool_kernel<int16_t, 16> : public avx_int_batch_bool_kernel<int16_t, 16>
        {
        };

        template <>
        struct batch_bool_kernel<uint16_t, 16> : public avx_int_batch_bool_kernel<uint16_t, 16>
        {
        };
    }

    /**********************
     * batch<int16_t, 16> *
     **********************/

    template <>
    struct simd_batch_traits<batch<int16_t, 16>>
    {
        using value_type = int16_t;
        static constexpr std::size_t size = 16;
        using batch_bool_type = batch_bool<int16_t, 16>;
        static constexpr std::size_t align = 32;
        using storage_type = __m256i;
    };

    template <>
    struct simd_batch_traits<batch<uint16_t, 16>>
    {
        using value_type = uint16_t;
        static constexpr std::size_t size = 16;
        using batch_bool_type = batch_bool<uint16_t, 16>;
        static constexpr std::size_t align = 32;
        using storage_type = __m256i;
    };

    template <>
    class batch<int16_t, 16> : public avx_int_batch<int16_t, 16>
    {
    public:

        using base_class = avx_int_batch<int16_t, 16>;
        using base_class::base_class;
        using base_class::load_aligned;
        using base_class::load_unaligned;
        using base_class::store_aligned;
        using base_class::store_unaligned;

        XSIMD_DECLARE_LOAD_STORE_INT16(int16_t, 16)
        XSIMD_DECLARE_LOAD_STORE_LONG(int16_t, 16)
    };

    template <>
    class batch<uint16_t, 16> : public avx_int_batch<uint16_t, 16>
    {
    public:

        using base_class = avx_int_batch<uint16_t, 16>;
        using base_class::base_class;
        using base_class::load_aligned;
        using base_class::load_unaligned;
        using base_class::store_aligned;
        using base_class::store_unaligned;

        XSIMD_DECLARE_LOAD_STORE_INT16(uint16_t, 16)
        XSIMD_DECLARE_LOAD_STORE_LONG(uint16_t, 16)
    };

    batch<int16_t, 16> operator<<(const batch<int16_t, 16>& lhs, int32_t rhs);
    batch<int16_t, 16> operator>>(const batch<int16_t, 16>& lhs, int32_t rhs);
    batch<int16_t, 16> operator<<(const batch<int16_t, 16>& lhs, const batch<int16_t, 16>& rhs);
    batch<int16_t, 16> operator>>(const batch<int16_t, 16>& lhs, const batch<int16_t, 16>& rhs);
    batch<uint16_t, 16> operator<<(const batch<uint16_t, 16>& lhs, int32_t rhs);
    batch<uint16_t, 16> operator>>(const batch<uint16_t, 16>& lhs, int32_t rhs);
    batch<uint16_t, 16> operator<<(const batch<uint16_t, 16>& lhs, const batch<int16_t, 16>& rhs);
    batch<uint16_t, 16> operator>>(const batch<uint16_t, 16>& lhs, const batch<int16_t, 16>& rhs);

    /*************************************
     * batch<int16_t, 16> implementation *
     *************************************/

    namespace detail
    {
        template <class T>
        struct int16_batch_kernel
            : avx_int_kernel_base<batch<T, 16>>
        {
            using batch_type = batch<T, 16>;
            using value_type = T;
            using batch_bool_type = batch_bool<T, 16>;

            constexpr static bool is_signed = std::is_signed<T>::value;

            static batch_type neg(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_sub_epi16(_mm256_setzero_si256(), rhs);
#else
                XSIMD_SPLIT_AVX(rhs);
                __m128i res_low = _mm_sub_epi16(_mm_setzero_si128(), rhs_low);
                __m128i res_high = _mm_sub_epi16(_mm_setzero_si128(), rhs_high);
                XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
            }

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_add_epi16(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_add_epi16, lhs, rhs);
#endif
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_sub_epi16(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_sub_epi16, lhs, rhs);
#endif
            }
            
            static batch_type sadd(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_adds_epi16(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_adds_epi16, lhs, rhs);
#endif
            }

            static batch_type ssub(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_subs_epi16(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_subs_epi16, lhs, rhs);
#endif
            }

            static batch_type mul(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_mullo_epi16(lhs, rhs);
#else
                // Note implement with conversion to epi16
                XSIMD_MACRO_UNROLL_BINARY(*);
#endif
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
                // TODO check if instruction workaround exists
// #if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
#if 0
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
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_cmpeq_epi16(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_cmpeq_epi16, lhs, rhs);
#endif
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_cmpgt_epi16(rhs, lhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_cmpgt_epi16, rhs, lhs);
#endif
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_min_epi16(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_min_epi16, lhs, rhs);
#endif
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_max_epi16(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_max_epi16, lhs, rhs);
#endif
            }

            static batch_type abs(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_abs_epi16(rhs);
#else
                XSIMD_SPLIT_AVX(rhs);
                __m128i res_low = _mm_abs_epi16(rhs_low);
                __m128i res_high = _mm_abs_epi16(rhs_high);
                XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
            }

            // TODO use conversion to int16_t
            static value_type hadd(const batch_type& lhs)
            {
                alignas(32) value_type tmp_lhs[16];
                lhs.store_aligned(&tmp_lhs[0]);
                value_type res = 0;
                unroller<16>([&](std::size_t i) {
                    res += tmp_lhs[i];
                });
                return res;
            }

            static batch_type select(const batch_bool_type& cond, const batch_type& a, const batch_type& b)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_blendv_epi8(b, a, cond);
#else
                XSIMD_SPLIT_AVX(cond);
                XSIMD_SPLIT_AVX(a);
                XSIMD_SPLIT_AVX(b);
                __m128i res_low = _mm_blendv_epi8(b_low, a_low, cond_low);
                __m128i res_high = _mm_blendv_epi8(b_high, a_high, cond_high);
                XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
            }

            static batch_type zip_lo(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_unpacklo_epi16(lhs, rhs);
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_unpackhi_epi16(lhs, rhs);
            }

        };

        template <>
        struct batch_kernel<int16_t, 16> : int16_batch_kernel<int16_t>
        {
        };

        template <>
        struct batch_kernel<uint16_t, 16> : int16_batch_kernel<uint16_t>
        {
            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                auto xor_lhs = _mm256_xor_si256(lhs, _mm256_set1_epi16(std::numeric_limits<int16_t>::lowest()));
                auto xor_rhs = _mm256_xor_si256(rhs, _mm256_set1_epi16(std::numeric_limits<int16_t>::lowest()));
                return _mm256_cmpgt_epi16(xor_rhs, xor_lhs);
#else
                // Note we could also use _mm256_xor_ps here but it might be slower
                // as it would go to the floating point device
                XSIMD_SPLIT_AVX(lhs);
                XSIMD_SPLIT_AVX(rhs);
                auto xer = _mm_set1_epi16(std::numeric_limits<int16_t>::lowest());
                lhs_low  = _mm_xor_si128(lhs_low,  xer);
                lhs_high = _mm_xor_si128(lhs_high, xer);
                rhs_low  = _mm_xor_si128(rhs_low,  xer);
                rhs_high = _mm_xor_si128(rhs_high, xer);
                __m128i res_low =  _mm_cmpgt_epi16(rhs_low,  lhs_low);
                __m128i res_high = _mm_cmpgt_epi16(rhs_high, lhs_high);
                XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_min_epu16(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_min_epu16, lhs, rhs);
#endif
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_max_epu16(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_max_epu16, lhs, rhs);
#endif
            }

            static batch_type abs(const batch_type& rhs)
            {
                return rhs;
            }

            static batch_type sadd(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_adds_epu16(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_adds_epu16, lhs, rhs);
#endif
            }

            static batch_type ssub(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_subs_epu16(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_subs_epu16, lhs, rhs);
#endif
            }
        };
    }

    XSIMD_DEFINE_LOAD_STORE_INT16(int16_t, 16, 32)
    XSIMD_DEFINE_LOAD_STORE_LONG(int16_t, 16, 32)

    inline batch<int16_t, 16> operator<<(const batch<int16_t, 16>& lhs, int32_t rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_slli_epi16(lhs, rhs);
#else
        XSIMD_SPLIT_AVX(lhs);
        __m128i res_low = _mm_slli_epi16(lhs_low, rhs);
        __m128i res_high = _mm_slli_epi16(lhs_high, rhs);
        XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
    }

    inline batch<int16_t, 16> operator>>(const batch<int16_t, 16>& lhs, int32_t rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_srai_epi16(lhs, rhs);
#else
        XSIMD_SPLIT_AVX(lhs);
        __m128i res_low = _mm_srai_epi16(lhs_low, rhs);
        __m128i res_high = _mm_srai_epi16(lhs_high, rhs);
        XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
    }

    inline batch<int16_t, 16> operator<<(const batch<int16_t, 16>& lhs, const batch<int16_t, 16>& rhs)
    {
#if defined(XSIMD_AVX512VL_AVAILABLE) && defined(XSIMD_AVX512BW_AVAILABLE)
        return _mm256_sllv_epi16(lhs, rhs);
#else
        return avx_detail::shift_impl([](int16_t lhs, int16_t s) { return lhs << s; }, lhs, rhs);
#endif
    }

    inline batch<int16_t, 16> operator>>(const batch<int16_t, 16>& lhs, const batch<int16_t, 16>& rhs)
    {
#if defined(XSIMD_AVX512VL_AVAILABLE) && defined(XSIMD_AVX512BW_AVAILABLE)
        return _mm256_srav_epi16(lhs, rhs);
#else
        return avx_detail::shift_impl([](int16_t lhs, int16_t s) { return lhs >> s; }, lhs, rhs);
#endif
    }

    XSIMD_DEFINE_LOAD_STORE_INT16(uint16_t, 16, 32)
    XSIMD_DEFINE_LOAD_STORE_LONG(uint16_t, 16, 32)

    inline batch<uint16_t, 16> operator<<(const batch<uint16_t, 16>& lhs, int32_t rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_slli_epi16(lhs, rhs);
#else
        XSIMD_SPLIT_AVX(lhs);
        __m128i res_low = _mm_slli_epi16(lhs_low, rhs);
        __m128i res_high = _mm_slli_epi16(lhs_high, rhs);
        XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
    }

    inline batch<uint16_t, 16> operator>>(const batch<uint16_t, 16>& lhs, int32_t rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_srli_epi16(lhs, rhs);
#else
        XSIMD_SPLIT_AVX(lhs);
        __m128i res_low = _mm_srli_epi16(lhs_low, rhs);
        __m128i res_high = _mm_srli_epi16(lhs_high, rhs);
        XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
    }

    inline batch<uint16_t, 16> operator<<(const batch<uint16_t, 16>& lhs, const batch<int16_t, 16>& rhs)
    {
#if defined(XSIMD_AVX512VL_AVAILABLE) && defined(XSIMD_AVX512BW_AVAILABLE)
        return _mm256_sllv_epi16(lhs, rhs);
#else
        return avx_detail::shift_impl([](uint16_t lhs, int16_t s) { return lhs << s; }, lhs, rhs);
#endif
    }

    inline batch<uint16_t, 16> operator>>(const batch<uint16_t, 16>& lhs, const batch<int16_t, 16>& rhs)
    {
#if defined(XSIMD_AVX512VL_AVAILABLE) && defined(XSIMD_AVX512BW_AVAILABLE)
        return _mm256_srlv_epi16(lhs, rhs);
#else
        return avx_detail::shift_impl([](uint16_t lhs, int16_t s) { return lhs >> s; }, lhs, rhs);
#endif
    }
}

#endif
