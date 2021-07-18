/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_AVX_INT64_HPP
#define XSIMD_AVX_INT64_HPP

#include <cstdint>

#include "xsimd_base.hpp"
#include "xsimd_avx_int_base.hpp"
#include "xsimd_sse_int64.hpp"

namespace xsimd
{

    /**************************
     * batch_bool<int64_t, 4> *
     **************************/

    template <>
    struct simd_batch_traits<batch_bool<int64_t, 4>>
    {
        using value_type = int64_t;
        static constexpr std::size_t size = 4;
        using batch_type = batch<int64_t, 4>;
        static constexpr std::size_t align = 32;
    };

    template <>
    struct simd_batch_traits<batch_bool<uint64_t, 4>>
    {
        using value_type = uint64_t;
        static constexpr std::size_t size = 4;
        using batch_type = batch<uint64_t, 4>;
        static constexpr std::size_t align = 32;
    };

    template <>
    class batch_bool<int64_t, 4> : public avx_int_batch_bool<int64_t, 4>
    {
    public:
        using avx_int_batch_bool::avx_int_batch_bool;
    };

    template <>
    class batch_bool<uint64_t, 4> : public avx_int_batch_bool<uint64_t, 4>
    {
    public:
        using avx_int_batch_bool::avx_int_batch_bool;
    };

    namespace detail
    {
        template <>
        struct batch_bool_kernel<int64_t, 4> : public avx_int_batch_bool_kernel<int64_t, 4>
        {
        };

        template <>
        struct batch_bool_kernel<uint64_t, 4> : public avx_int_batch_bool_kernel<uint64_t, 4>
        {
        };
    }


    /*********************
     * batch<int64_t, 4> *
     *********************/

    template <>
    struct simd_batch_traits<batch<int64_t, 4>>
    {
        using value_type = int64_t;
        static constexpr std::size_t size = 4;
        using batch_bool_type = batch_bool<int64_t, 4>;
        static constexpr std::size_t align = 32;
        using storage_type = __m256i;
    };

    template <>
    struct simd_batch_traits<batch<uint64_t, 4>>
    {
        using value_type = uint64_t;
        static constexpr std::size_t size = 4;
        using batch_bool_type = batch_bool<uint64_t, 4>;
        static constexpr std::size_t align = 32;
        using storage_type = __m256i;
    };

    template <>
    class batch<int64_t, 4> : public avx_int_batch<int64_t, 4>
    {

    public:

        using base_type = avx_int_batch<int64_t, 4>;
        using base_type::base_type;
        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;

        XSIMD_DECLARE_LOAD_STORE_INT64(int64_t, 4)
        XSIMD_DECLARE_LOAD_STORE_LONG(int64_t, 4)
    };

    template <>
    class batch<uint64_t, 4> : public avx_int_batch<uint64_t, 4>
    {
    public:

        using base_type = avx_int_batch<uint64_t, 4>;
        using base_type::base_type;
        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;

        XSIMD_DECLARE_LOAD_STORE_INT64(uint64_t, 4)
        XSIMD_DECLARE_LOAD_STORE_LONG(uint64_t, 4)
    };

    batch<int64_t, 4> operator<<(const batch<int64_t, 4>& lhs, int32_t rhs);
    batch<int64_t, 4> operator>>(const batch<int64_t, 4>& lhs, int32_t rhs);
    batch<int64_t, 4> operator<<(const batch<int64_t, 4>& lhs, const batch<int64_t, 4>& rhs);
    batch<int64_t, 4> operator>>(const batch<int64_t, 4>& lhs, const batch<int64_t, 4>& rhs);
    batch<uint64_t, 4> operator<<(const batch<uint64_t, 4>& lhs, int32_t rhs);
    batch<uint64_t, 4> operator>>(const batch<uint64_t, 4>& lhs, int32_t rhs);
    batch<uint64_t, 4> operator<<(const batch<uint64_t, 4>& lhs, const batch<int64_t, 4>& rhs);
    batch<uint64_t, 4> operator>>(const batch<uint64_t, 4>& lhs, const batch<int64_t, 4>& rhs);

    /************************************
     * batch<int64_t, 4> implementation *
     ************************************/

    XSIMD_DEFINE_LOAD_STORE_INT64(int64_t, 4, 32)
    XSIMD_DEFINE_LOAD_STORE_LONG(int64_t, 4, 32)

    /*************************************
     * batch<uint64_t, 4> implementation *
     *************************************/

    XSIMD_DEFINE_LOAD_STORE_INT64(uint64_t, 4, 32)
    XSIMD_DEFINE_LOAD_STORE_LONG(uint64_t, 4, 32)

#undef AVX_DEFINE_LOAD_STORE_INT64

    namespace detail
    {
        template <>
        struct batch_kernel<uint64_t, 4>
            : avx_int_kernel_base<batch<uint64_t, 4>>
        {
            using batch_type = batch<uint64_t, 4>;
            using value_type = uint64_t;
            using batch_bool_type = batch_bool<uint64_t, 4>;

            static batch_type neg(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_sub_epi64(_mm256_setzero_si256(), rhs);
#else
                XSIMD_SPLIT_AVX(rhs);
                __m128i res_low = _mm_sub_epi64(_mm_setzero_si128(), rhs_low);
                __m128i res_high = _mm_sub_epi64(_mm_setzero_si128(), rhs_high);
                XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
            }

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_add_epi64(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_add_epi64, lhs, rhs);
#endif
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_sub_epi64(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_sub_epi64, lhs, rhs);
#endif
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

            static batch_type mul(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_MACRO_UNROLL_BINARY(*);
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
#if defined(XSIMD_FAST_INTEGER_DIVISION)
                __m256d dlhs = _mm256_setr_pd(static_cast<double>(lhs[0]), static_cast<double>(lhs[1]),
                                              static_cast<double>(lhs[2]), static_cast<double>(lhs[3]));
                __m256d drhs = _mm256_setr_pd(static_cast<double>(rhs[0]), static_cast<double>(rhs[1]),
                                              static_cast<double>(rhs[2]), static_cast<double>(rhs[3]));
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_cvtepi32_epi64(_mm256_cvttpd_epi32(_mm256_div_pd(dlhs, drhs)));
#else
                using batch_int = batch<int32_t, 4>;
                __m128i tmp = _mm256_cvttpd_epi32(_mm256_div_pd(dlhs, drhs));
                __m128i res_low = _mm_unpacklo_epi32(tmp, batch_int(tmp) < batch_int(0));
                __m128i res_high = _mm_unpackhi_epi32(tmp, batch_int(tmp) < batch_int(0));
                __m256i result = _mm256_castsi128_si256(res_low);
                return _mm256_insertf128_si256(result, res_high, 1);
#endif
#else
                XSIMD_MACRO_UNROLL_BINARY(/)
#endif
            }

            static batch_type mod(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_MACRO_UNROLL_BINARY(%);
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_cmpeq_epi64(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_cmpeq_epi64, lhs, rhs);
#endif
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                auto xor_lhs = _mm256_xor_si256(lhs, batch<int64_t, 4>(std::numeric_limits<int64_t>::lowest()));
                auto xor_rhs = _mm256_xor_si256(rhs, batch<int64_t, 4>(std::numeric_limits<int64_t>::lowest()));
                return _mm256_cmpgt_epi64(xor_rhs, xor_lhs);
#else
                XSIMD_SPLIT_AVX(lhs);
                XSIMD_SPLIT_AVX(rhs);
                __m128i xer = batch<int64_t, 2>(std::numeric_limits<int64_t>::lowest());
                lhs_low  = _mm_xor_si128(lhs_low,  xer);
                lhs_high = _mm_xor_si128(lhs_high, xer);
                rhs_low  = _mm_xor_si128(rhs_low,  xer);
                rhs_high = _mm_xor_si128(rhs_high, xer);
                __m128i res_low =  _mm_cmpgt_epi64(rhs_low,  lhs_low);
                __m128i res_high = _mm_cmpgt_epi64(rhs_high, lhs_high);
                XSIMD_RETURN_MERGED_SSE(res_low, res_high);
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

            static batch_type abs(const batch_type& rhs)
            {
#if defined(XSIMD_AVX512VL_AVAILABLE)
                return _mm256_abs_epi64(rhs);
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                __m256i sign = _mm256_cmpgt_epi64(_mm256_setzero_si256(), rhs);
                __m256i inv = _mm256_xor_si256(rhs, sign);
                return _mm256_sub_epi64(inv, sign);
#else
                XSIMD_SPLIT_AVX(rhs);
                __m128i sign_low = _mm_cmpgt_epi64(_mm_setzero_si128(), rhs_low);
                __m128i sign_high = _mm_cmpgt_epi64(_mm_setzero_si128(), rhs_high);
                __m128i inv_low = _mm_xor_si128(rhs_low, sign_low);
                __m128i inv_high = _mm_xor_si128(rhs_high, sign_high);
                __m128i res_low = _mm_sub_epi64(inv_low, sign_low);
                __m128i res_high = _mm_sub_epi64(inv_high, sign_high);
                XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
            }

            static value_type hadd(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                __m256i tmp1 = _mm256_shuffle_epi32(rhs, 0x0E);
                __m256i tmp2 = _mm256_add_epi64(rhs, tmp1);
                __m128i tmp3 = _mm256_extracti128_si256(tmp2, 1);
                __m128i res = _mm_add_epi64(_mm256_castsi256_si128(tmp2), tmp3);
#else
                XSIMD_SPLIT_AVX(rhs);
                __m128i tmp1 = _mm_shuffle_epi32(rhs_low, 0x0E);
                __m128i tmp2 = _mm_add_epi64(tmp1, rhs_low);
                __m128i tmp3 = _mm_shuffle_epi32(rhs_high, 0x0E);
                __m128i tmp4 = _mm_add_epi64(tmp3, rhs_high);
                __m128i res = _mm_add_epi64(tmp2, tmp4);
#endif
#if defined(__x86_64__)
                return _mm_cvtsi128_si64(res);
#else
                union {
                    int64_t i;
                    __m128i m;
                } u;
                _mm_storel_epi64(&u.m, res);
                return u.i;
#endif
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
                return _mm256_unpacklo_epi64(lhs, rhs);
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_unpackhi_epi64(lhs, rhs);
            }

        };

        template <>
        struct batch_kernel<int64_t, 4>
            : avx_int_kernel_base<batch<int64_t, 4>>
        {
            using batch_type = batch<int64_t, 4>;
            using value_type = int64_t;
            using batch_bool_type = batch_bool<int64_t, 4>;

            static batch_type neg(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_sub_epi64(_mm256_setzero_si256(), rhs);
#else
                XSIMD_SPLIT_AVX(rhs);
                __m128i res_low = _mm_sub_epi64(_mm_setzero_si128(), rhs_low);
                __m128i res_high = _mm_sub_epi64(_mm_setzero_si128(), rhs_high);
                XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
            }

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_add_epi64(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_add_epi64, lhs, rhs);
#endif
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_sub_epi64(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_sub_epi64, lhs, rhs);
#endif
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
                return sadd(lhs, neg(rhs));
            }

            static batch_type mul(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_MACRO_UNROLL_BINARY(*);
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
#if defined(XSIMD_FAST_INTEGER_DIVISION)
                __m256d dlhs = _mm256_setr_pd(static_cast<double>(lhs[0]), static_cast<double>(lhs[1]),
                                              static_cast<double>(lhs[2]), static_cast<double>(lhs[3]));
                __m256d drhs = _mm256_setr_pd(static_cast<double>(rhs[0]), static_cast<double>(rhs[1]),
                                              static_cast<double>(rhs[2]), static_cast<double>(rhs[3]));
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_cvtepi32_epi64(_mm256_cvttpd_epi32(_mm256_div_pd(dlhs, drhs)));
#else
                using batch_int = batch<int32_t, 4>;
                __m128i tmp = _mm256_cvttpd_epi32(_mm256_div_pd(dlhs, drhs));
                __m128i res_low = _mm_unpacklo_epi32(tmp, batch_int(tmp) < batch_int(0));
                __m128i res_high = _mm_unpackhi_epi32(tmp, batch_int(tmp) < batch_int(0));
                __m256i result = _mm256_castsi128_si256(res_low);
                return _mm256_insertf128_si256(result, res_high, 1);
#endif
#else
                XSIMD_MACRO_UNROLL_BINARY(/)
#endif
            }

            static batch_type mod(const batch_type& lhs, const batch_type& rhs)
            {
                XSIMD_MACRO_UNROLL_BINARY(%);
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_cmpeq_epi64(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_cmpeq_epi64, lhs, rhs);
#endif
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_cmpgt_epi64(rhs, lhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_cmpgt_epi64, rhs, lhs);
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

            static batch_type abs(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                __m256i sign = _mm256_cmpgt_epi64(_mm256_setzero_si256(), rhs);
                __m256i inv = _mm256_xor_si256(rhs, sign);
                return _mm256_sub_epi64(inv, sign);
#else
                XSIMD_SPLIT_AVX(rhs);
                __m128i sign_low = _mm_cmpgt_epi64(_mm_setzero_si128(), rhs_low);
                __m128i sign_high = _mm_cmpgt_epi64(_mm_setzero_si128(), rhs_high);
                __m128i inv_low = _mm_xor_si128(rhs_low, sign_low);
                __m128i inv_high = _mm_xor_si128(rhs_high, sign_high);
                __m128i res_low = _mm_sub_epi64(inv_low, sign_low);
                __m128i res_high = _mm_sub_epi64(inv_high, sign_high);
                XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
            }

            static value_type hadd(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                __m256i tmp1 = _mm256_shuffle_epi32(rhs, 0x0E);
                __m256i tmp2 = _mm256_add_epi64(rhs, tmp1);
                __m128i tmp3 = _mm256_extracti128_si256(tmp2, 1);
                __m128i res = _mm_add_epi64(_mm256_castsi256_si128(tmp2), tmp3);
#else
                XSIMD_SPLIT_AVX(rhs);
                __m128i tmp1 = _mm_shuffle_epi32(rhs_low, 0x0E);
                __m128i tmp2 = _mm_add_epi64(tmp1, rhs_low);
                __m128i tmp3 = _mm_shuffle_epi32(rhs_high, 0x0E);
                __m128i tmp4 = _mm_add_epi64(tmp3, rhs_high);
                __m128i res = _mm_add_epi64(tmp2, tmp4);
#endif
#if defined(__x86_64__)
                return _mm_cvtsi128_si64(res);
#else
                union {
                    int64_t i;
                    __m128i m;
                } u;
                _mm_storel_epi64(&u.m, res);
                return u.i;
#endif
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
                return _mm256_unpacklo_epi64(lhs, rhs);
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_unpackhi_epi64(lhs, rhs);
            }
        };
    }

    inline batch<int64_t, 4> operator<<(const batch<int64_t, 4>& lhs, int32_t rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_slli_epi64(lhs, rhs);
#else
        XSIMD_SPLIT_AVX(lhs);
        __m128i res_low = _mm_slli_epi64(lhs_low, rhs);
        __m128i res_high = _mm_slli_epi64(lhs_high, rhs);
        XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
    }

    inline batch<int64_t, 4> operator>>(const batch<int64_t, 4>& lhs, int32_t rhs)
    {
#if defined(XSIMD_AVX512VL_AVAILABLE)
        return _mm256_srai_epi64(lhs, rhs);
#else
        return avx_detail::shift_impl([](int64_t val, int32_t s) { return val >> s; }, lhs, rhs);
#endif
    }

    inline batch<int64_t, 4> operator<<(const batch<int64_t, 4>& lhs, const batch<int64_t, 4>& rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_sllv_epi64(lhs, rhs);
#else
        return avx_detail::shift_impl([](int64_t val, int64_t s) { return val << s; }, lhs, rhs);
#endif
    }

    inline batch<int64_t, 4> operator>>(const batch<int64_t, 4>& lhs, const batch<int64_t, 4>& rhs)
    {
#if defined(XSIMD_AVX512VL_AVAILABLE)
        return _mm256_srav_epi64(lhs, rhs);
#else
        return avx_detail::shift_impl([](int64_t val, int64_t s) { return val >> s; }, lhs, rhs);
#endif
    }

    inline batch<uint64_t, 4> operator<<(const batch<uint64_t, 4>& lhs, int32_t rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_slli_epi64(lhs, rhs);
#else
        XSIMD_SPLIT_AVX(lhs);
        __m128i res_low = _mm_slli_epi64(lhs_low, rhs);
        __m128i res_high = _mm_slli_epi64(lhs_high, rhs);
        XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
    }

    inline batch<uint64_t, 4> operator>>(const batch<uint64_t, 4>& lhs, int32_t rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_srli_epi64(lhs, rhs);
#else
        XSIMD_SPLIT_AVX(lhs);
        __m128i res_low = _mm_srli_epi64(lhs_low, rhs);
        __m128i res_high = _mm_srli_epi64(lhs_high, rhs);
        XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
    }

    inline batch<uint64_t, 4> operator<<(const batch<uint64_t, 4>& lhs, const batch<int64_t, 4>& rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_sllv_epi64(lhs, rhs);
#else
        return avx_detail::shift_impl([](uint64_t val, int64_t s) { return val << s; }, lhs, rhs);
#endif
    }

    inline batch<uint64_t, 4> operator>>(const batch<uint64_t, 4>& lhs, const batch<int64_t, 4>& rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_srlv_epi64(lhs, rhs);
#else
        return avx_detail::shift_impl([](uint64_t val, int64_t s) { return val >> s; }, lhs, rhs);
#endif
    }
}

#endif
