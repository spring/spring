/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_AVX512_INT16_HPP
#define XSIMD_AVX512_INT16_HPP

#include "xsimd_avx512_bool.hpp"
#include "xsimd_avx512_int_base.hpp"

namespace xsimd
{

#define XSIMD_APPLY_AVX2_FUNCTION_INT16(func, avx_lhs, avx_rhs) \
    XSIMD_APPLY_AVX2_FUNCTION(16, func, avx_lhs, avx_rhs)

    /***************************
     * batch_bool<int16_t, 32> *
     ***************************/

    template <>
    struct simd_batch_traits<batch_bool<int16_t, 32>>
    {
        using value_type = int16_t;
        static constexpr std::size_t size = 32;
        using batch_type = batch<int16_t, 32>;
        static constexpr std::size_t align = 64;
    };

    template <>
    struct simd_batch_traits<batch_bool<uint16_t, 32>>
    {
        using value_type = uint16_t;
        static constexpr std::size_t size = 32;
        using batch_type = batch<uint16_t, 32>;
        static constexpr std::size_t align = 64;
    };

#if defined(XSIMD_AVX512BW_AVAILABLE)

    template <>
    class batch_bool<int16_t, 32> :
        public batch_bool_avx512<__mmask32, batch_bool<int16_t, 32>>
    {
    public:

        using base_class = batch_bool_avx512<__mmask32, batch_bool<int16_t, 32>>;
        using base_class::base_class;
    };

    template <>
    class batch_bool<uint16_t, 32> :
        public batch_bool_avx512<__mmask32, batch_bool<uint16_t, 32>>
    {
    public:

        using base_class = batch_bool_avx512<__mmask32, batch_bool<uint16_t, 32>>;
        using base_class::base_class;
    };

    namespace detail
    {
        template <>
        struct batch_bool_kernel<int16_t, 32>
            : batch_bool_kernel_avx512<int16_t, 32>
        {
        };

        template <>
        struct batch_bool_kernel<uint16_t, 32>
            : batch_bool_kernel_avx512<uint16_t, 32>
        {
        };
    }

#else

    template <>
    class batch_bool<int16_t, 32> : public avx512_fallback_batch_bool<int16_t, 32>
    {
    public:

        using base_class = avx512_fallback_batch_bool<int16_t, 32>;
        using base_class::base_class;
    };

    template <>
    class batch_bool<uint16_t, 32> : public avx512_fallback_batch_bool<uint16_t, 32>
    {
    public:

        using base_class = avx512_fallback_batch_bool<uint16_t, 32>;
        using base_class::base_class;
    };


    namespace detail
    {
        template <>
        struct batch_bool_kernel<int16_t, 32>
            : avx512_fallback_batch_bool_kernel<int16_t, 32>
        {
        };

        template <>
        struct batch_bool_kernel<uint16_t, 32>
            : avx512_fallback_batch_bool_kernel<uint16_t, 32>
        {
        };
    }

#endif

    /**********************
     * batch<int16_t, 32> *
     **********************/

    template <>
    struct simd_batch_traits<batch<int16_t, 32>>
    {
        using value_type = int16_t;
        static constexpr std::size_t size = 32;
        using batch_bool_type = batch_bool<int16_t, 32>;
        static constexpr std::size_t align = 64;
        using storage_type = __m512i;
    };

    template <>
    struct simd_batch_traits<batch<uint16_t, 32>>
    {
        using value_type = uint16_t;
        static constexpr std::size_t size = 32;
        using batch_bool_type = batch_bool<uint16_t, 32>;
        static constexpr std::size_t align = 64;
        using storage_type = __m512i;
    };

    template <>
    class batch<int16_t, 32> : public avx512_int_batch<int16_t, 32>
    {
    public:

        using base_class = avx512_int_batch;
        using base_class::base_class;
        using base_class::load_aligned;
        using base_class::load_unaligned;
        using base_class::store_aligned;
        using base_class::store_unaligned;

        batch() = default;

        explicit batch(const char* src)
            : batch(reinterpret_cast<const int16_t*>(src))
        {
        }

        batch(const char* src, aligned_mode)
            : batch(reinterpret_cast<const int16_t*>(src), aligned_mode{})
        {
        }

        batch(const char* src, unaligned_mode)
            : batch(reinterpret_cast<const int16_t*>(src), unaligned_mode{})
        {
        }

        XSIMD_DECLARE_LOAD_STORE_INT16(int16_t, 32)
        XSIMD_DECLARE_LOAD_STORE_LONG(int16_t, 32)
    };

    template <>
    class batch<uint16_t, 32> : public avx512_int_batch<uint16_t, 32>
    {
    public:

        using base_class = avx512_int_batch;
        using base_class::base_class;
        using base_class::load_aligned;
        using base_class::load_unaligned;
        using base_class::store_aligned;
        using base_class::store_unaligned;

        XSIMD_DECLARE_LOAD_STORE_INT16(uint16_t, 32)
        XSIMD_DECLARE_LOAD_STORE_LONG(uint16_t, 32)
    };

    batch<int16_t, 32> operator<<(const batch<int16_t, 32>& lhs, int32_t rhs);
    batch<int16_t, 32> operator>>(const batch<int16_t, 32>& lhs, int32_t rhs);
    batch<int16_t, 32> operator<<(const batch<int16_t, 32>& lhs, const batch<int16_t, 32>& rhs);
    batch<int16_t, 32> operator>>(const batch<int16_t, 32>& lhs, const batch<int16_t, 32>& rhs);
    batch<uint16_t, 32> operator<<(const batch<uint16_t, 32>& lhs, int32_t rhs);
    batch<uint16_t, 32> operator>>(const batch<uint16_t, 32>& lhs, int32_t rhs);
    batch<uint16_t, 32> operator<<(const batch<uint16_t, 32>& lhs, const batch<int16_t, 32>& rhs);
    batch<uint16_t, 32> operator>>(const batch<uint16_t, 32>& lhs, const batch<int16_t, 32>& rhs);

    /*************************************
     * batch<int16_t, 32> implementation *
     *************************************/

    namespace detail
    {
        template <class T>
        struct avx512_int16_batch_kernel
            : avx512_int_kernel_base<batch<T, 32>>
        {
            using batch_type = batch<T, 32>;
            using value_type = T;
            using batch_bool_type = batch_bool<T, 32>;

            static batch_type neg(const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_sub_epi16(_mm512_setzero_si512(), rhs);
            #else
                XSIMD_SPLIT_AVX512(rhs);
                __m256i res_low = _mm256_sub_epi16(_mm256_setzero_si256(), rhs_low);
                __m256i res_high = _mm256_sub_epi16(_mm256_setzero_si256(), rhs_high);
                XSIMD_RETURN_MERGED_AVX(res_low, res_high);
            #endif
            }

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_add_epi16(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT16(add, lhs, rhs);
            #endif
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_sub_epi16(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT16(sub, lhs, rhs);
            #endif
            }

            static batch_type sadd(const batch_type &lhs, const batch_type &rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_adds_epi16(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT16(sadd, lhs, rhs);
            #endif
            }

            static batch_type ssub(const batch_type &lhs, const batch_type &rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_subs_epi16(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT16(ssub, lhs, rhs);
            #endif
            }
            
            static batch_type mul(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_mullo_epi16(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT16(mul, lhs, rhs);
            #endif
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_APPLY_AVX2_FUNCTION_INT16(div, lhs, rhs);
            }

            static batch_type mod(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_MACRO_UNROLL_BINARY(%);
            }

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_and_si512(lhs, rhs);
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_or_si512(lhs, rhs);
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_xor_si512(lhs, rhs);
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return _mm512_xor_si512(rhs, _mm512_set1_epi16(-1));
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_andnot_si512(lhs, rhs);
            }

            static batch_type fma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return x * y + z;
            }

            static batch_type fms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return x * y - z;
            }

            static batch_type fnma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return -x * y + z;
            }

            static batch_type fnms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return -x * y - z;
            }

            static value_type hadd(const batch_type& rhs)
            {
                XSIMD_SPLIT_AVX512(rhs);
                auto tmp = batch<value_type, 16>(rhs_low) + batch<value_type, 16>(rhs_high);
                return xsimd::hadd(batch<value_type, 16>(tmp));
            }

            static batch_type select(const batch_bool_type& cond, const batch_type& a, const batch_type& b)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE) && !defined(_MSC_VER)
                auto res = _mm512_mask_blend_epi16((__mmask32)cond, (__m512i)b, (__m512i)a);
                return batch_type(res);
            #else
                __m512i mcond = _mm512_maskz_broadcastw_epi16((__mmask32)cond, _mm_set1_epi32(~0));
                XSIMD_SPLIT_AVX512(mcond);
                XSIMD_SPLIT_AVX512(a);
                XSIMD_SPLIT_AVX512(b);

                auto res_lo = _mm256_blendv_epi8(b_low, a_low, mcond_low);
                auto res_hi = _mm256_blendv_epi8(b_high, a_high, mcond_high);

                XSIMD_RETURN_MERGED_AVX(res_lo, res_hi);
            #endif
            }

            static batch_type zip_lo(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_unpacklo_epi16(lhs, rhs);
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_unpackhi_epi16(lhs, rhs);
            }

        };

        template <>
        struct batch_kernel<int16_t, 32>
            : public avx512_int16_batch_kernel<int16_t>
        {
            static batch_type abs(const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_abs_epi16(rhs);
            #else
                XSIMD_SPLIT_AVX512(rhs);
                __m256i res_low = _mm256_abs_epi16(rhs_low);
                __m256i res_high = _mm256_abs_epi16(rhs_high);
                XSIMD_RETURN_MERGED_AVX(res_low, res_high);
            #endif
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_min_epi16(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT16(min, lhs, rhs);
            #endif
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_max_epi16(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT16(max, lhs, rhs);
            #endif
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_cmpeq_epi16_mask(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT16(eq, lhs, rhs);
            #endif
            }

            static batch_bool_type neq(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_cmpneq_epi16_mask(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT16(neq, lhs, rhs);
            #endif
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_cmplt_epi16_mask(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT16(lt, lhs, rhs);
            #endif
            }

            static batch_bool_type lte(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_cmple_epi16_mask(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT16(lte, lhs, rhs);
            #endif
            }
        };

        template <>
        struct batch_kernel<uint16_t, 32>
            : public avx512_int16_batch_kernel<uint16_t>
        {
            static batch_type abs(const batch_type& rhs)
            {
                return rhs;
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_min_epu16(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT16(min, lhs, rhs);
            #endif
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_max_epu16(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT16(max, lhs, rhs);
            #endif
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_cmpeq_epu16_mask(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT16(eq, lhs, rhs);
            #endif
            }

            static batch_bool_type neq(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_cmpneq_epu16_mask(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT16(neq, lhs, rhs);
            #endif
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_cmplt_epu16_mask(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT16(lt, lhs, rhs);
            #endif
            }

            static batch_bool_type lte(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_cmple_epu16_mask(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT16(lte, lhs, rhs);
            #endif
            }
            
            static batch_type sadd(const batch_type &lhs, const batch_type &rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_adds_epu16(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_UINT16(sadd, lhs, rhs);
            #endif
            }

            static batch_type ssub(const batch_type &lhs, const batch_type &rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_subs_epu16(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_UINT16(ssub, lhs, rhs);
            #endif
            }
        };
    }

    inline batch<int16_t, 32> operator<<(const batch<int16_t, 32>& lhs, int32_t rhs)
    {
#if defined(XSIMD_AVX512BW_AVAILABLE)
#if defined(XSIMD_AVX512_SHIFT_INTRINSICS_IMM_ONLY)
        return _mm512_sllv_epi16(lhs, _mm512_set1_epi16(rhs));
#else
        return _mm512_slli_epi16(lhs, rhs);
#endif
#else
#if defined(XSIMD_AVX512_SHIFT_INTRINSICS_IMM_ONLY)
        __m512i tmp = _mm512_sllv_epi32(lhs, _mm512_set1_epi32(rhs));
#else
        __m512i tmp = _mm512_slli_epi32(lhs, rhs);
#endif
        return _mm512_and_si512(_mm512_set1_epi16(0xFFFF << rhs), tmp);
#endif
    }

    inline batch<int16_t, 32> operator>>(const batch<int16_t, 32>& lhs, int32_t rhs)
    {
#if defined(XSIMD_AVX512BW_AVAILABLE)
#if defined(XSIMD_AVX512_SHIFT_INTRINSICS_IMM_ONLY)
        return _mm512_srav_epi16(lhs, _mm512_set1_epi16(rhs));
#else
        return _mm512_srai_epi16(lhs, rhs);
#endif
#else
        return avx512_detail::shift_impl([](int16_t val, int32_t s) { return val >> s; }, lhs, rhs);
#endif
    }

    inline batch<int16_t, 32> operator<<(const batch<int16_t, 32>& lhs, const batch<int16_t, 32>& rhs)
    {
#if defined(XSIMD_AVX512BW_AVAILABLE)
        return _mm512_sllv_epi16(lhs, rhs);
#else
        return avx512_detail::shift_impl([](int16_t val, int16_t s) { return val << s; }, lhs, rhs);
#endif
    }

    inline batch<int16_t, 32> operator>>(const batch<int16_t, 32>& lhs, const batch<int16_t, 32>& rhs)
    {
#if defined(XSIMD_AVX512BW_AVAILABLE)
        return _mm512_srav_epi16(lhs, rhs);
#else
        return avx512_detail::shift_impl([](int16_t val, int16_t s) { return val >> s; }, lhs, rhs);
#endif
    }

    XSIMD_DEFINE_LOAD_STORE_INT16(int16_t, 32, 64)
    XSIMD_DEFINE_LOAD_STORE_LONG(int16_t, 32, 64)

    inline batch<uint16_t, 32> operator<<(const batch<uint16_t, 32>& lhs, int32_t rhs)
    {
#if defined(XSIMD_AVX512BW_AVAILABLE)
#if defined(XSIMD_AVX512_SHIFT_INTRINSICS_IMM_ONLY)
        return _mm512_sllv_epi16(lhs, _mm512_set1_epi16(rhs));
#else
        return _mm512_slli_epi16(lhs, rhs);
#endif
#else
#if defined(XSIMD_AVX512_SHIFT_INTRINSICS_IMM_ONLY)
        __m512i tmp = _mm512_sllv_epi32(lhs, _mm512_set1_epi32(rhs));
#else
        __m512i tmp = _mm512_slli_epi32(lhs, rhs);
#endif
        return _mm512_and_si512(_mm512_set1_epi16(0xFFFF << rhs), tmp);
#endif
    }

    inline batch<uint16_t, 32> operator>>(const batch<uint16_t, 32>& lhs, int32_t rhs)
    {
#if defined(XSIMD_AVX512BW_AVAILABLE)
#if defined(XSIMD_AVX512_SHIFT_INTRINSICS_IMM_ONLY)
        return _mm512_srlv_epi16(lhs, _mm512_set1_epi16(rhs));
#else
        return _mm512_srli_epi16(lhs, rhs);
#endif
#else
#if defined(XSIMD_AVX512_SHIFT_INTRINSICS_IMM_ONLY)
        __m512i tmp = _mm512_srlv_epi32(lhs, _mm512_set1_epi32(rhs));
#else
        __m512i tmp = _mm512_srli_epi32(lhs, rhs);
#endif
        return _mm512_and_si512(_mm512_set1_epi16(0xFFFF >> rhs), tmp);
#endif
    }

    inline batch<uint16_t, 32> operator<<(const batch<uint16_t, 32>& lhs, const batch<int16_t, 32>& rhs)
    {
#if defined(XSIMD_AVX512BW_AVAILABLE)
        return _mm512_sllv_epi16(lhs, rhs);
#else
        return avx512_detail::shift_impl([](uint16_t val, int16_t s) { return val << s; }, lhs, rhs);
#endif
    }

    inline batch<uint16_t, 32> operator>>(const batch<uint16_t, 32>& lhs, const batch<int16_t, 32>& rhs)
    {
#if defined(XSIMD_AVX512BW_AVAILABLE)
        return _mm512_srlv_epi16(lhs, rhs);
#else
        return avx512_detail::shift_impl([](uint16_t val, int16_t s) { return val >> s; }, lhs, rhs);
#endif
    }

    XSIMD_DEFINE_LOAD_STORE_INT16(uint16_t, 32, 64)
    XSIMD_DEFINE_LOAD_STORE_LONG(uint16_t, 32, 64)

#undef XSIMD_APPLY_AVX2_FUNCTION_INT16
}

#endif
