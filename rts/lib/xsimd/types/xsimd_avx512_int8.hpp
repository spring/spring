/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_AVX512_INT8_HPP
#define XSIMD_AVX512_INT8_HPP

#include "xsimd_avx512_bool.hpp"
#include "xsimd_avx512_int_base.hpp"

namespace xsimd
{

#define XSIMD_APPLY_AVX2_FUNCTION_INT8(func, avx_lhs, avx_rhs) \
    XSIMD_APPLY_AVX2_FUNCTION(32, func, avx_lhs, avx_rhs)

    /****************************
     * batch_bool<int8 / int16> *
     ****************************/

    template <>
    struct simd_batch_traits<batch_bool<int8_t, 64>>
    {
        using value_type = int8_t;
        static constexpr std::size_t size = 64;
        using batch_type = batch<int8_t, 64>;
        static constexpr std::size_t align = 64;
    };

    template <>
    struct simd_batch_traits<batch_bool<uint8_t, 64>>
    {
        using value_type = uint8_t;
        static constexpr std::size_t size = 64;
        using batch_type = batch<uint8_t, 64>;
        static constexpr std::size_t align = 64;
    };

#if defined(XSIMD_AVX512BW_AVAILABLE)

    template <>
    class batch_bool<int8_t, 64> :
        public batch_bool_avx512<__mmask64, batch_bool<int8_t, 64>>
    {
    public:

        using base_class = batch_bool_avx512<__mmask64, batch_bool<int8_t, 64>>;
        using base_class::base_class;
    };

    template <>
    class batch_bool<uint8_t, 64> :
        public batch_bool_avx512<__mmask64, batch_bool<uint8_t, 64>>
    {
    public:

        using base_class = batch_bool_avx512<__mmask64, batch_bool<uint8_t, 64>>;
        using base_class::base_class;
    };

    namespace detail
    {
        template <>
        struct batch_bool_kernel<int8_t, 64>
            : batch_bool_kernel_avx512<int8_t, 64>
        {
        };

        template <>
        struct batch_bool_kernel<uint8_t, 64>
            : batch_bool_kernel_avx512<uint8_t, 64>
        {
        };
    }

#else

    template <>
    class batch_bool<int8_t, 64> : public avx512_fallback_batch_bool<int8_t, 64>
    {
    public:

        using base_class = avx512_fallback_batch_bool<int8_t, 64>;
        using base_class::base_class;
    };

    template <>
    class batch_bool<uint8_t, 64> : public avx512_fallback_batch_bool<uint8_t, 64>
    {
    public:

        using base_class = avx512_fallback_batch_bool<uint8_t, 64>;
        using base_class::base_class;
    };


    namespace detail
    {
        template <>
        struct batch_bool_kernel<int8_t, 64>
            : avx512_fallback_batch_bool_kernel<int8_t, 64>
        {
        };

        template <>
        struct batch_bool_kernel<uint8_t, 64>
            : avx512_fallback_batch_bool_kernel<uint8_t, 64>
        {
        };
    }

#endif

    /*********************
     * batch<int32_t, 8> *
     *********************/

    template <>
    struct simd_batch_traits<batch<int8_t, 64>>
    {
        using value_type = int8_t;
        static constexpr std::size_t size = 64;
        using batch_bool_type = batch_bool<int8_t, 64>;
        static constexpr std::size_t align = 64;
        using storage_type = __m512i;
    };

    template <>
    struct simd_batch_traits<batch<uint8_t, 64>>
    {
        using value_type = uint8_t;
        static constexpr std::size_t size = 64;
        using batch_bool_type = batch_bool<uint8_t, 64>;
        static constexpr std::size_t align = 64;
        using storage_type = __m512i;
    };

    template <>
    class batch<int8_t, 64> : public avx512_int_batch<int8_t, 64>
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

        XSIMD_DECLARE_LOAD_STORE_INT8(int8_t, 64)
        XSIMD_DECLARE_LOAD_STORE_LONG(int8_t, 64)
    };

    template <>
    class batch<uint8_t, 64> : public avx512_int_batch<uint8_t, 64>
    {
    public:

        using base_class = avx512_int_batch;
        using base_class::base_class;
        using base_class::load_aligned;
        using base_class::load_unaligned;
        using base_class::store_aligned;
        using base_class::store_unaligned;

        XSIMD_DECLARE_LOAD_STORE_INT8(uint8_t, 64)
        XSIMD_DECLARE_LOAD_STORE_LONG(uint8_t, 64)
    };

    batch<int8_t, 64> operator<<(const batch<int8_t, 64>& lhs, int32_t rhs);
    batch<int8_t, 64> operator>>(const batch<int8_t, 64>& lhs, int32_t rhs);
    batch<int8_t, 64> operator<<(const batch<int8_t, 64>& lhs, const batch<int8_t, 64>& rhs);
    batch<int8_t, 64> operator>>(const batch<int8_t, 64>& lhs, const batch<int8_t, 64>& rhs);
    batch<uint8_t, 64> operator<<(const batch<uint8_t, 64>& lhs, int32_t rhs);
    batch<uint8_t, 64> operator>>(const batch<uint8_t, 64>& lhs, int32_t rhs);
    batch<uint8_t, 64> operator<<(const batch<uint8_t, 64>& lhs, const batch<int8_t, 64>& rhs);
    batch<uint8_t, 64> operator>>(const batch<uint8_t, 64>& lhs, const batch<int8_t, 64>& rhs);

    /************************************
     * batch<int8_t, 64> implementation *
     ************************************/

    namespace detail
    {
        template <class T>
        struct avx512_int8_batch_kernel
            : avx512_int_kernel_base<batch<T, 64>>
        {
            using batch_type = batch<T, 64>;
            using value_type = T;
            using batch_bool_type = batch_bool<T, 64>;

            static batch_type neg(const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_sub_epi8(_mm512_setzero_si512(), rhs);
            #else
                XSIMD_SPLIT_AVX512(rhs);
                __m256i res_low = _mm256_sub_epi8(_mm256_setzero_si256(), rhs_low);
                __m256i res_high = _mm256_sub_epi8(_mm256_setzero_si256(), rhs_high);
                XSIMD_RETURN_MERGED_AVX(res_low, res_high);
            #endif
            }

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_add_epi8(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT8(add, lhs, rhs);
            #endif
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_sub_epi8(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT8(sub, lhs, rhs);
            #endif
            }

            static batch_type sadd(const batch_type &lhs, const batch_type &rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_adds_epi8(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT8(sadd, lhs, rhs);
            #endif
            }

            static batch_type ssub(const batch_type &lhs, const batch_type &rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_subs_epi8(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT8(ssub, lhs, rhs);
            #endif
            }

            static batch_type mul(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                batch_type upper = _mm512_and_si512(_mm512_mullo_epi16(lhs, rhs), _mm512_srli_epi16(_mm512_set1_epi16(-1), 8));
                batch_type lower = _mm512_slli_epi16(_mm512_mullo_epi16(_mm512_srli_epi16(lhs, 8), _mm512_srli_epi16(rhs, 8)), 8);
                return _mm512_or_si512(upper, lower);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT8(mul, lhs, rhs);
            #endif
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_APPLY_AVX2_FUNCTION_INT8(div, lhs, rhs);
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
                return _mm512_xor_si512(rhs, _mm512_set1_epi8(-1));
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
                auto tmp = batch<value_type, 32>(rhs_low) + batch<value_type, 32>(rhs_high);
                return xsimd::hadd(batch<value_type, 32>(tmp));
            }

            static batch_type select(const batch_bool_type& cond, const batch_type& a, const batch_type& b)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                // Some compilers are not happy with passing directly a and b to the intrinsics
                // See https://github.com/xtensor-stack/xsimd/issues/315
                __m512i ma = a;
                __m512i mb = b;
                return _mm512_mask_blend_epi8(cond, mb, ma);
            #else
                XSIMD_SPLIT_AVX512(cond);
                XSIMD_SPLIT_AVX512(a);
                XSIMD_SPLIT_AVX512(b);

                auto res_lo = _mm256_blendv_epi8(b_low, a_low, cond_low);
                auto res_hi = _mm256_blendv_epi8(b_high, a_high, cond_high);

                XSIMD_RETURN_MERGED_AVX(res_lo, res_hi);
            #endif
            }

            static batch_type zip_lo(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_unpacklo_epi8(lhs, rhs);
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_unpackhi_epi8(lhs, rhs);
            }

        };

        template <>
        struct batch_kernel<int8_t, 64>
            : public avx512_int8_batch_kernel<int8_t>
        {
            static batch_type abs(const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_abs_epi8(rhs);
            #else
                XSIMD_SPLIT_AVX512(rhs);
                __m256i res_low = _mm256_abs_epi8(rhs_low);
                __m256i res_high = _mm256_abs_epi8(rhs_high);
                XSIMD_RETURN_MERGED_AVX(res_low, res_high);
            #endif
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_min_epi8(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT8(min, lhs, rhs);
            #endif
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_max_epi8(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT8(max, lhs, rhs);
            #endif
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_cmpeq_epi8_mask(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT8(eq, lhs, rhs);
            #endif
            }

            static batch_bool_type neq(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_cmpneq_epi8_mask(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT8(neq, lhs, rhs);
            #endif
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_cmplt_epi8_mask(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT8(lt, lhs, rhs);
            #endif
            }

            static batch_bool_type lte(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_cmple_epi8_mask(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT8(lte, lhs, rhs);
            #endif
            }
        };

        template <>
        struct batch_kernel<uint8_t, 64>
            : public avx512_int8_batch_kernel<uint8_t>
        {
            static batch_type abs(const batch_type& rhs)
            {
                return rhs;
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_min_epu8(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT8(min, lhs, rhs);
            #endif
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_max_epu8(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT8(max, lhs, rhs);
            #endif
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_cmpeq_epu8_mask(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT8(eq, lhs, rhs);
            #endif
            }

            static batch_bool_type neq(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_cmpneq_epu8_mask(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT8(neq, lhs, rhs);
            #endif
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_cmplt_epu8_mask(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT8(lt, lhs, rhs);
            #endif
            }

            static batch_bool_type lte(const batch_type& lhs, const batch_type& rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_cmple_epu8_mask(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_INT8(lte, lhs, rhs);
            #endif
            }
            
            static batch_type sadd(const batch_type &lhs, const batch_type &rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_adds_epu8(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_UINT8(sadd, lhs, rhs);
            #endif
            }

            static batch_type ssub(const batch_type &lhs, const batch_type &rhs)
            {
            #if defined(XSIMD_AVX512BW_AVAILABLE)
                return _mm512_subs_epu8(lhs, rhs);
            #else
                XSIMD_APPLY_AVX2_FUNCTION_UINT8(ssub, lhs, rhs);
            #endif
            }
        };
    }

    inline batch<int8_t, 64> operator<<(const batch<int8_t, 64>& lhs, int32_t rhs)
    {
#if defined(XSIMD_AVX512_SHIFT_INTRINSICS_IMM_ONLY)
        __m512i tmp = _mm512_sllv_epi32(lhs, _mm512_set1_epi32(rhs));
#else
        __m512i tmp = _mm512_slli_epi32(lhs, rhs);
#endif
        return _mm512_and_si512(_mm512_set1_epi8(0xFF << rhs), tmp);
    }

    inline batch<int8_t, 64> operator>>(const batch<int8_t, 64>& lhs, int32_t rhs)
    {
#if defined(XSIMD_AVX512BW_AVAILABLE)
        __m512i sign_mask = _mm512_set1_epi16((0xFF00 >> rhs) & 0x00FF);
        __m512i zeros = _mm512_setzero_si512();
        __mmask64 cmp_is_negative_mask = _mm512_cmpgt_epi8_mask(zeros, lhs);
        __m512i cmp_sign_mask = _mm512_mask_blend_epi8(cmp_is_negative_mask, zeros, sign_mask);
#if defined(XSIMD_AVX512_SHIFT_INTRINSICS_IMM_ONLY)
        __m512i res = _mm512_srav_epi16(lhs, _mm512_set1_epi16(rhs));
#else
        __m512i res = _mm512_srai_epi16(lhs, rhs);
#endif
        return _mm512_or_si512(cmp_sign_mask, _mm512_andnot_si512(sign_mask, res));
#else
        return avx512_detail::shift_impl([](int8_t val, int32_t s) { return val >> s; }, lhs, rhs);
#endif
    }

    inline batch<int8_t, 64> operator<<(const batch<int8_t, 64>& lhs, const batch<int8_t, 64>& rhs)
    {
        return avx512_detail::shift_impl([](int8_t val, int8_t s) { return val << s; }, lhs, rhs);
    }

    inline batch<int8_t, 64> operator>>(const batch<int8_t, 64>& lhs, const batch<int8_t, 64>& rhs)
    {
        return avx512_detail::shift_impl([](int8_t val, int8_t s) { return val >> s; }, lhs, rhs);
    }

    XSIMD_DEFINE_LOAD_STORE_INT8(int8_t, 64, 64)
    XSIMD_DEFINE_LOAD_STORE_LONG(int8_t, 64, 64)

    inline batch<uint8_t, 64> operator<<(const batch<uint8_t, 64>& lhs, int32_t rhs)
    {
#if defined(XSIMD_AVX512_SHIFT_INTRINSICS_IMM_ONLY)
        __m512i tmp = _mm512_sllv_epi32(lhs, _mm512_set1_epi32(rhs));
#else
        __m512i tmp = _mm512_slli_epi32(lhs, rhs);
#endif
        return _mm512_and_si512(_mm512_set1_epi8(0xFF << rhs), tmp);
    }

    inline batch<uint8_t, 64> operator>>(const batch<uint8_t, 64>& lhs, int32_t rhs)
    {
#if defined(XSIMD_AVX512_SHIFT_INTRINSICS_IMM_ONLY)
        __m512i tmp = _mm512_srlv_epi32(lhs, _mm512_set1_epi32(rhs));
#else
        __m512i tmp = _mm512_srli_epi32(lhs, rhs);
#endif
        return _mm512_and_si512(_mm512_set1_epi8(0xFF >> rhs), tmp);
    }

    inline batch<uint8_t, 64> operator<<(const batch<uint8_t, 64>& lhs, const batch<int8_t, 64>& rhs)
    {
        return avx512_detail::shift_impl([](uint8_t val, int8_t s) { return val << s; }, lhs, rhs);
    }

    inline batch<uint8_t, 64> operator>>(const batch<uint8_t, 64>& lhs, const batch<int8_t, 64>& rhs)
    {
        return avx512_detail::shift_impl([](uint8_t val, int8_t s) { return val >> s; }, lhs, rhs);
    }

    XSIMD_DEFINE_LOAD_STORE_INT8(uint8_t, 64, 64)
    XSIMD_DEFINE_LOAD_STORE_LONG(uint8_t, 64, 64)

#undef XSIMD_APPLY_AVX2_FUNCTION_INT8
}

#endif
