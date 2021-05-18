/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_AVX512_INT32_HPP
#define XSIMD_AVX512_INT32_HPP

#include "xsimd_avx512_bool.hpp"
#include "xsimd_avx512_int_base.hpp"

namespace xsimd
{

    /***************************
     * batch_bool<int32_t, 16> *
     ***************************/

    template <>
    struct simd_batch_traits<batch_bool<int32_t, 16>>
    {
        using value_type = int32_t;
        static constexpr std::size_t size = 16;
        using batch_type = batch<int32_t, 16>;
        static constexpr std::size_t align = 0;
    };

    template <>
    struct simd_batch_traits<batch_bool<uint32_t, 16>>
    {
        using value_type = uint32_t;
        static constexpr std::size_t size = 16;
        using batch_type = batch<uint32_t, 16>;
        static constexpr std::size_t align = 0;
    };

    template <>
    class batch_bool<int32_t, 16> :
        public batch_bool_avx512<__mmask16, batch_bool<int32_t, 16>>
    {
    public:
        using base_class = batch_bool_avx512<__mmask16, batch_bool<int32_t, 16>>;
        using base_class::base_class;
    };

    template <>
    class batch_bool<uint32_t, 16> :
        public batch_bool_avx512<__mmask16, batch_bool<uint32_t, 16>>
    {
    public:
        using base_class = batch_bool_avx512<__mmask16, batch_bool<uint32_t, 16>>;
        using base_class::base_class;
    };

    namespace detail
    {
        template <>
        struct batch_bool_kernel<int32_t, 16>
            : batch_bool_kernel_avx512<int32_t, 16>
        {
        };

        template <>
        struct batch_bool_kernel<uint32_t, 16>
            : batch_bool_kernel_avx512<uint32_t, 16>
        {
        };
    }

    /**********************
     * batch<int32_t, 16> *
     **********************/

    template <>
    struct simd_batch_traits<batch<int32_t, 16>>
    {
        using value_type = int32_t;
        static constexpr std::size_t size = 16;
        using batch_bool_type = batch_bool<int32_t, 16>;
        static constexpr std::size_t align = 64;
        using storage_type = __m512i;
    };

    template <>
    struct simd_batch_traits<batch<uint32_t, 16>>
    {
        using value_type = uint32_t;
        static constexpr std::size_t size = 16;
        using batch_bool_type = batch_bool<uint32_t, 16>;
        static constexpr std::size_t align = 64;
        using storage_type = __m512i;
    };

    template <>
    class batch<int32_t, 16> : public avx512_int_batch<int32_t, 16>
    {
    public:

        using base_type = avx512_int_batch<int32_t, 16>;
        using base_type::base_type;
        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;

        XSIMD_DECLARE_LOAD_STORE_INT32(int32_t, 16)
        XSIMD_DECLARE_LOAD_STORE_LONG(int32_t, 16)
    };

    template <>
    class batch<uint32_t, 16> : public avx512_int_batch<uint32_t, 16>
    {
    public:

        using base_type = avx512_int_batch<uint32_t, 16>;
        using base_type::base_type;
        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;

        XSIMD_DECLARE_LOAD_STORE_INT32(uint32_t, 16)
        XSIMD_DECLARE_LOAD_STORE_LONG(uint32_t, 16)
    };

    batch<int32_t, 16> operator<<(const batch<int32_t, 16>& lhs, int32_t rhs);
    batch<int32_t, 16> operator>>(const batch<int32_t, 16>& lhs, int32_t rhs);
    batch<int32_t, 16> operator<<(const batch<int32_t, 16>& lhs, const batch<int32_t, 16>& rhs);
    batch<int32_t, 16> operator>>(const batch<int32_t, 16>& lhs, const batch<int32_t, 16>& rhs);
    batch<uint32_t, 16> operator<<(const batch<uint32_t, 16>& lhs, int32_t rhs);
    batch<uint32_t, 16> operator>>(const batch<uint32_t, 16>& lhs, int32_t rhs);
    batch<uint32_t, 16> operator<<(const batch<uint32_t, 16>& lhs, const batch<int32_t, 16>& rhs);
    batch<uint32_t, 16> operator>>(const batch<uint32_t, 16>& lhs, const batch<int32_t, 16>& rhs);

    /*************************************
     * batch<int32_t, 16> implementation *
     *************************************/

    XSIMD_DEFINE_LOAD_STORE_INT32(int32_t, 16, 64)
    XSIMD_DEFINE_LOAD_STORE_LONG(int32_t, 16, 64)

    /*************************************
     * batch<uint32_t, 16> implementation *
     *************************************/

    XSIMD_DEFINE_LOAD_STORE_INT32(uint32_t, 16, 64)
    XSIMD_DEFINE_LOAD_STORE_LONG(uint32_t, 16, 64)

    namespace detail
    {
        template <class T>
        struct avx512_int32_batch_kernel
            : avx512_int_kernel_base<batch<T, 16>>
        {
            using batch_type = batch<T, 16>;
            using value_type = T;
            using batch_bool_type = batch_bool<T, 16>;

            static batch_type neg(const batch_type& rhs)
            {
                return _mm512_sub_epi32(_mm512_setzero_si512(), rhs);
            }

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_add_epi32(lhs, rhs);
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_sub_epi32(lhs, rhs);
            }

            static batch_type sadd(const batch_type& lhs, const batch_type& rhs)
            {
                batch_bool_type mask = _mm512_movepi32_mask(rhs);
                batch_type lhs_pos_branch = min(std::numeric_limits<value_type>::max() - rhs, lhs);
                batch_type lhs_neg_branch = max(std::numeric_limits<value_type>::min() - rhs, lhs);
                return rhs + select(mask, lhs_neg_branch, lhs_pos_branch);
            }

            static batch_type ssub(const batch_type& lhs, const batch_type& rhs)
            {
                return sadd(lhs, neg(rhs));
            }

            static batch_type mul(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_mullo_epi32(lhs, rhs);
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
                return _mm512_xor_si512(rhs, _mm512_set1_epi32(-1));
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
                // TODO Why not _mm512_reduce_add_...?
                __m256i tmp1 = _mm512_extracti32x8_epi32(rhs, 0);
                __m256i tmp2 = _mm512_extracti32x8_epi32(rhs, 1);
                __m256i res1 = _mm256_add_epi32(tmp1, tmp2);
                return xsimd::hadd(batch<int32_t, 8>(res1));
            }

            static batch_type select(const batch_bool_type& cond, const batch_type& a, const batch_type& b)
            {
                return _mm512_mask_blend_epi32(cond, b, a);
            }

            static batch_type zip_lo(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_unpacklo_epi32(lhs, rhs);
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_unpackhi_epi32(lhs, rhs);
            }

        };

        template <>
        struct batch_kernel<int32_t, 16>
            : public avx512_int32_batch_kernel<int32_t>
        {
            using batch_type = batch<int32_t, 16>;
            using value_type = int32_t;
            using batch_bool_type = batch_bool<int32_t, 16>;

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
#if defined(XSIMD_FAST_INTEGER_DIVISION)
                return _mm512_cvttps_epi32(_mm512_div_ps(_mm512_cvtepi32_ps(lhs), _mm512_cvtepi32_ps(rhs)));
#else
                XSIMD_MACRO_UNROLL_BINARY(/);
#endif
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_cmpeq_epi32_mask(lhs, rhs);
            }

            static batch_bool_type neq(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_cmpneq_epi32_mask(lhs, rhs);
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_cmplt_epi32_mask(lhs, rhs);
            }

            static batch_bool_type lte(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_cmple_epi32_mask(lhs, rhs);
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_min_epi32(lhs, rhs);
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_max_epi32(lhs, rhs);
            }

            static batch_type abs(const batch_type& rhs)
            {
                return _mm512_abs_epi32(rhs);
            }
        };

        template <>
        struct batch_kernel<uint32_t, 16>
            : public avx512_int32_batch_kernel<uint32_t>
        {
            using batch_type = batch<uint32_t, 16>;
            using value_type = uint32_t;
            using batch_bool_type = batch_bool<uint32_t, 16>;

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
#if defined(XSIMD_FAST_INTEGER_DIVISION)
                return _mm512_cvttps_epu32(_mm512_div_ps(_mm512_cvtepu32_ps(lhs), _mm512_cvtepu32_ps(rhs)));
#else
                XSIMD_MACRO_UNROLL_BINARY(/);
#endif
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_cmpeq_epu32_mask(lhs, rhs);
            }

            static batch_bool_type neq(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_cmpneq_epu32_mask(lhs, rhs);
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_cmplt_epu32_mask(lhs, rhs);
            }

            static batch_bool_type lte(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_cmple_epu32_mask(lhs, rhs);
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_min_epu32(lhs, rhs);
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm512_max_epu32(lhs, rhs);
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

    inline batch<int32_t, 16> operator<<(const batch<int32_t, 16>& lhs, int32_t rhs)
    {
#if defined(XSIMD_AVX512_SHIFT_INTRINSICS_IMM_ONLY)
        return _mm512_sllv_epi32(lhs, _mm512_set1_epi32(rhs));
#else
        return _mm512_slli_epi32(lhs, rhs);
#endif
    }

    inline batch<int32_t, 16> operator>>(const batch<int32_t, 16>& lhs, int32_t rhs)
    {
#if defined(XSIMD_AVX512_SHIFT_INTRINSICS_IMM_ONLY)
        return _mm512_srav_epi32(lhs, _mm512_set1_epi32(rhs));
#else
        return _mm512_srai_epi32(lhs, rhs);
#endif
    }

    inline batch<int32_t, 16> operator<<(const batch<int32_t, 16>& lhs, const batch<int32_t, 16>& rhs)
    {
        return _mm512_sllv_epi32(lhs, rhs);
    }

    inline batch<int32_t, 16> operator>>(const batch<int32_t, 16>& lhs, const batch<int32_t, 16>& rhs)
    {
        return _mm512_srav_epi32(lhs, rhs);
    }

    inline batch<uint32_t, 16> operator<<(const batch<uint32_t, 16>& lhs, int32_t rhs)
    {
#if defined(XSIMD_AVX512_SHIFT_INTRINSICS_IMM_ONLY)
        return _mm512_sllv_epi32(lhs, _mm512_set1_epi32(rhs));
#else
        return _mm512_slli_epi32(lhs, rhs);
#endif
    }

    inline batch<uint32_t, 16> operator>>(const batch<uint32_t, 16>& lhs, int32_t rhs)
    {
#if defined(XSIMD_AVX512_SHIFT_INTRINSICS_IMM_ONLY)
        return _mm512_srlv_epi32(lhs, _mm512_set1_epi32(rhs));
#else
        return _mm512_srli_epi32(lhs, rhs);
#endif
    }

    inline batch<uint32_t, 16> operator<<(const batch<uint32_t, 16>& lhs, const batch<int32_t, 16>& rhs)
    {
        return _mm512_sllv_epi32(lhs, rhs);
    }

    inline batch<uint32_t, 16> operator>>(const batch<uint32_t, 16>& lhs, const batch<int32_t, 16>& rhs)
    {
        return _mm512_srlv_epi32(lhs, rhs);
    }
}

#endif
