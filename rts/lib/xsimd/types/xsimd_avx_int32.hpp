/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_AVX_INT32_HPP
#define XSIMD_AVX_INT32_HPP

#include <cstdint>

#include "xsimd_base.hpp"
#include "xsimd_avx_int_base.hpp"
#include "xsimd_int_conversion.hpp"
#include "xsimd_sse_int32.hpp"

namespace xsimd
{

    /**************************
     * batch_bool<int32_t, 8> *
     **************************/

    template <>
    struct simd_batch_traits<batch_bool<int32_t, 8>>
    {
        using value_type = int32_t;
        static constexpr std::size_t size = 8;
        using batch_type = batch<int32_t, 8>;
        static constexpr std::size_t align = 32;
    };

    template <>
    struct simd_batch_traits<batch_bool<uint32_t, 8>>
    {
        using value_type = uint32_t;
        static constexpr std::size_t size = 8;
        using batch_type = batch<uint32_t, 8>;
        static constexpr std::size_t align = 32;
    };

    template <>
    class batch_bool<int32_t, 8> : public avx_int_batch_bool<int32_t, 8>
    {
    public:
        using avx_int_batch_bool::avx_int_batch_bool;
    };

    template <>
    class batch_bool<uint32_t, 8> : public avx_int_batch_bool<uint32_t, 8>
    {
    public:
        using avx_int_batch_bool::avx_int_batch_bool;
    };

    namespace detail
    {
        template <>
        struct batch_bool_kernel<int32_t, 8> : public avx_int_batch_bool_kernel<int32_t, 8>
        {
        };

        template <>
        struct batch_bool_kernel<uint32_t, 8> : public avx_int_batch_bool_kernel<uint32_t, 8>
        {
        };
    }

    /*********************
     * batch<int32_t, 8> *
     *********************/

    template <>
    struct simd_batch_traits<batch<int32_t, 8>>
    {
        using value_type = int32_t;
        static constexpr std::size_t size = 8;
        using batch_bool_type = batch_bool<int32_t, 8>;
        static constexpr std::size_t align = 32;
        using storage_type = __m256i;
    };

    template <>
    struct simd_batch_traits<batch<uint32_t, 8>>
    {
        using value_type = uint32_t;
        static constexpr std::size_t size = 8;
        using batch_bool_type = batch_bool<uint32_t, 8>;
        static constexpr std::size_t align = 32;
        using storage_type = __m256i;
    };

    template <>
    class batch<int32_t, 8> : public avx_int_batch<int32_t, 8>
    {
    public:

        using base_type = avx_int_batch<int32_t, 8>;
        using base_type::base_type;
        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;

        XSIMD_DECLARE_LOAD_STORE_INT32(int32_t, 8)
        XSIMD_DECLARE_LOAD_STORE_LONG(int32_t, 8)
    };

    template <>
    class batch<uint32_t, 8> : public avx_int_batch<uint32_t, 8>
    {
    public:

        using base_type = avx_int_batch<uint32_t, 8>;
        using base_type::base_type;
        using base_type::load_aligned;
        using base_type::load_unaligned;
        using base_type::store_aligned;
        using base_type::store_unaligned;

        XSIMD_DECLARE_LOAD_STORE_INT32(uint32_t, 8)
        XSIMD_DECLARE_LOAD_STORE_LONG(uint32_t, 8)
    };

    batch<int32_t, 8> operator<<(const batch<int32_t, 8>& lhs, int32_t rhs);
    batch<int32_t, 8> operator>>(const batch<int32_t, 8>& lhs, int32_t rhs);
    batch<int32_t, 8> operator<<(const batch<int32_t, 8>& lhs, const batch<int32_t, 8>& rhs);
    batch<int32_t, 8> operator>>(const batch<int32_t, 8>& lhs, const batch<int32_t, 8>& rhs);
    batch<uint32_t, 8> operator<<(const batch<uint32_t, 8>& lhs, int32_t rhs);
    batch<uint32_t, 8> operator>>(const batch<uint32_t, 8>& lhs, int32_t rhs);
    batch<uint32_t, 8> operator<<(const batch<uint32_t, 8>& lhs, const batch<int32_t, 8>& rhs);
    batch<uint32_t, 8> operator>>(const batch<uint32_t, 8>& lhs, const batch<int32_t, 8>& rhs);

    /************************************
     * batch<int32_t, 8> implementation *
     ************************************/

    inline batch<int32_t, 8>& batch<int32_t, 8>::load_aligned(const float* src)
    {
        this->m_value = _mm256_cvtps_epi32(_mm256_load_ps(src));
        return *this;
    }

    inline batch<int32_t, 8>& batch<int32_t, 8>::load_unaligned(const float* src)
    {
        this->m_value = _mm256_cvtps_epi32(_mm256_loadu_ps(src));
        return *this;
    }

    inline void batch<int32_t, 8>::store_aligned(float* dst) const
    {
        _mm256_store_ps(dst, _mm256_cvtepi32_ps(this->m_value));
    }

    inline void batch<int32_t, 8>::store_unaligned(float* dst) const
    {
        _mm256_storeu_ps(dst, _mm256_cvtepi32_ps(this->m_value));
    }

    XSIMD_DEFINE_LOAD_STORE(int32_t, 8, bool, 32)
    XSIMD_DEFINE_LOAD_STORE(int32_t, 8, int8_t, 32)
    XSIMD_DEFINE_LOAD_STORE(int32_t, 8, uint8_t, 32)
    XSIMD_DEFINE_LOAD_STORE(int32_t, 8, int16_t, 32)
    XSIMD_DEFINE_LOAD_STORE(int32_t, 8, uint16_t, 32)
    XSIMD_DEFINE_LOAD_STORE(int32_t, 8, int64_t, 32)
    XSIMD_DEFINE_LOAD_STORE(int32_t, 8, uint64_t, 32)
    XSIMD_DEFINE_LOAD_STORE(int32_t, 8, double, 32)
    XSIMD_DEFINE_LOAD_STORE_LONG(int32_t, 8, 32)

    /*************************************
     * batch<uint32_t, 8> implementation *
     *************************************/

    XSIMD_DEFINE_LOAD_STORE_INT32(uint32_t, 8, 32)
    XSIMD_DEFINE_LOAD_STORE_LONG(uint32_t, 8, 32)

#undef AVX_DEFINE_LOAD_STORE_INT32

    namespace detail
    {
        template <>
        struct batch_kernel<int32_t, 8>
            : avx_int_kernel_base<batch<int32_t, 8>>
        {
            using batch_type = batch<int32_t, 8>;
            using value_type = int32_t;
            using batch_bool_type = batch_bool<int32_t, 8>;

            static batch_type neg(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_sub_epi32(_mm256_setzero_si256(), rhs);
#else
                XSIMD_SPLIT_AVX(rhs);
                __m128i res_low = _mm_sub_epi32(_mm_setzero_si128(), rhs_low);
                __m128i res_high = _mm_sub_epi32(_mm_setzero_si128(), rhs_high);
                XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
            }

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_add_epi32(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_add_epi32, lhs, rhs);
#endif
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_sub_epi32(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_sub_epi32, lhs, rhs);
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
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_mullo_epi32(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_mullo_epi32, lhs, rhs);
#endif
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
#if defined(XSIMD_FAST_INTEGER_DIVISION)
                return _mm256_cvttps_epi32(_mm256_div_ps(_mm256_cvtepi32_ps(lhs), _mm256_cvtepi32_ps(rhs)));
#else
                alignas(64) int32_t tmp_lhs[8], tmp_rhs[8], tmp_res[8];
                lhs.store_aligned(tmp_lhs);
                rhs.store_aligned(tmp_rhs);
                unroller<8>([&](std::size_t i) {
                    tmp_res[i] = tmp_lhs[i] / tmp_rhs[i];
                });
                return batch_type(tmp_res, aligned_mode());
#endif
            }

            static batch_type mod(const batch_type& lhs, const batch_type& rhs)
            {
                alignas(64) int32_t tmp_lhs[8], tmp_rhs[8], tmp_res[8];
                lhs.store_aligned(tmp_lhs);
                rhs.store_aligned(tmp_rhs);
                unroller<8>([&](std::size_t i) {
                    tmp_res[i] = tmp_lhs[i] % tmp_rhs[i];
                });
                return batch_type(tmp_res, aligned_mode());
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_cmpeq_epi32(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_cmpeq_epi32, lhs, rhs);
#endif
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_cmpgt_epi32(rhs, lhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_cmpgt_epi32, rhs, lhs);
#endif
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_min_epi32(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_min_epi32, lhs, rhs);
#endif
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_max_epi32(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_max_epi32, lhs, rhs);
#endif
            }

            static batch_type abs(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_abs_epi32(rhs);
#else
                XSIMD_SPLIT_AVX(rhs);
                __m128i res_low = _mm_abs_epi32(rhs_low);
                __m128i res_high = _mm_abs_epi32(rhs_high);
                XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
            }

            static value_type hadd(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                __m256i tmp1 = _mm256_hadd_epi32(rhs, rhs);
                __m256i tmp2 = _mm256_hadd_epi32(tmp1, tmp1);
                __m128i tmp3 = _mm256_extracti128_si256(tmp2, 1);
                __m128i tmp4 = _mm_add_epi32(_mm256_castsi256_si128(tmp2), tmp3);
                return _mm_cvtsi128_si32(tmp4);
#else
                XSIMD_SPLIT_AVX(rhs);
                __m128i tmp1 = _mm_add_epi32(rhs_low, rhs_high);
                __m128i tmp2 = _mm_hadd_epi32(tmp1, tmp1);
                __m128i tmp3 = _mm_hadd_epi32(tmp2, tmp2);
                return _mm_cvtsi128_si32(tmp3);
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
                return _mm256_unpacklo_epi32(lhs, rhs);
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_unpackhi_epi32(lhs, rhs);
            }

        };

        template <>
        struct batch_kernel<uint32_t, 8>
            : avx_int_kernel_base<batch<uint32_t, 8>>
        {
            using batch_type = batch<uint32_t, 8>;
            using value_type = uint32_t;
            using batch_bool_type = batch_bool<uint32_t, 8>;

            static batch_type neg(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_sub_epi32(_mm256_setzero_si256(), rhs);
#else
                XSIMD_SPLIT_AVX(rhs);
                __m128i res_low = _mm_sub_epi32(_mm_setzero_si128(), rhs_low);
                __m128i res_high = _mm_sub_epi32(_mm_setzero_si128(), rhs_high);
                XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
            }

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_add_epi32(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_add_epi32, lhs, rhs);
#endif
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_sub_epi32(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_sub_epi32, lhs, rhs);
#endif
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

            static batch_type mul(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_mullo_epi32(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_mullo_epi32, lhs, rhs);
#endif
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
#if defined(XSIMD_FAST_INTEGER_DIVISION)
                return _mm256_cvttps_epi32(_mm256_div_ps(_mm256_cvtepi32_ps(lhs), _mm256_cvtepi32_ps(rhs)));
#else
                alignas(64) uint32_t tmp_lhs[8], tmp_rhs[8], tmp_res[8];
                lhs.store_aligned(tmp_lhs);
                rhs.store_aligned(tmp_rhs);
                unroller<8>([&](std::size_t i) {
                    tmp_res[i] = tmp_lhs[i] / tmp_rhs[i];
                });
                return batch_type(tmp_res, aligned_mode());
#endif
            }

            static batch_type mod(const batch_type& lhs, const batch_type& rhs)
            {
                alignas(64) uint32_t tmp_lhs[8], tmp_rhs[8], tmp_res[8];
                lhs.store_aligned(tmp_lhs);
                rhs.store_aligned(tmp_rhs);
                unroller<8>([&](std::size_t i) {
                    tmp_res[i] = tmp_lhs[i] % tmp_rhs[i];
                });
                return batch_type(tmp_res, aligned_mode());
            }

            static batch_bool_type eq(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_cmpeq_epi32(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_cmpeq_epi32, lhs, rhs);
#endif
            }

            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                auto xor_lhs = _mm256_xor_si256(lhs, _mm256_set1_epi32(std::numeric_limits<int32_t>::lowest()));
                auto xor_rhs = _mm256_xor_si256(rhs, _mm256_set1_epi32(std::numeric_limits<int32_t>::lowest()));
                return _mm256_cmpgt_epi32(xor_rhs, xor_lhs);
#else
                // Note we could also use _mm256_xor_ps here but it might be slower
                // as it would go to the floating point device
                XSIMD_SPLIT_AVX(lhs);
                XSIMD_SPLIT_AVX(rhs);
                auto xer = _mm_set1_epi32(std::numeric_limits<int32_t>::lowest());
                lhs_low  = _mm_xor_si128(lhs_low,  xer);
                lhs_high = _mm_xor_si128(lhs_high, xer);
                rhs_low  = _mm_xor_si128(rhs_low,  xer);
                rhs_high = _mm_xor_si128(rhs_high, xer);
                __m128i res_low =  _mm_cmpgt_epi32(rhs_low,  lhs_low);
                __m128i res_high = _mm_cmpgt_epi32(rhs_high, lhs_high);
                XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_min_epu32(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_min_epu32, lhs, rhs);
#endif
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_max_epu32(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_max_epu32, lhs, rhs);
#endif
            }

            static batch_type abs(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_sign_epi32(rhs, rhs);
#else
                XSIMD_SPLIT_AVX(rhs);
                __m128i res_low = _mm_sign_epi32(rhs_low, rhs_low);
                __m128i res_high = _mm_sign_epi32(rhs_high, rhs_high);
                XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
            }

            static value_type hadd(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                __m256i tmp1 = _mm256_hadd_epi32(rhs, rhs);
                __m256i tmp2 = _mm256_hadd_epi32(tmp1, tmp1);
                __m128i tmp3 = _mm256_extracti128_si256(tmp2, 1);
                __m128i tmp4 = _mm_add_epi32(_mm256_castsi256_si128(tmp2), tmp3);
                return _mm_cvtsi128_si32(tmp4);
#else
                XSIMD_SPLIT_AVX(rhs);
                __m128i tmp1 = _mm_add_epi32(rhs_low, rhs_high);
                __m128i tmp2 = _mm_hadd_epi32(tmp1, tmp1);
                __m128i tmp3 = _mm_hadd_epi32(tmp2, tmp2);
                return _mm_cvtsi128_si32(tmp3);
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
                return _mm256_unpacklo_epi32(lhs, rhs);
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_unpackhi_epi32(lhs, rhs);
            }

        };
    }

    inline batch<int32_t, 8> operator<<(const batch<int32_t, 8>& lhs, int32_t rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_slli_epi32(lhs, rhs);
#else
        XSIMD_SPLIT_AVX(lhs);
        __m128i res_low = _mm_slli_epi32(lhs_low, rhs);
        __m128i res_high = _mm_slli_epi32(lhs_high, rhs);
        XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
    }

    inline batch<int32_t, 8> operator>>(const batch<int32_t, 8>& lhs, int32_t rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_srai_epi32(lhs, rhs);
#else
        XSIMD_SPLIT_AVX(lhs);
        __m128i res_low = _mm_srai_epi32(lhs_low, rhs);
        __m128i res_high = _mm_srai_epi32(lhs_high, rhs);
        XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
    }

    inline batch<int32_t, 8> operator<<(const batch<int32_t, 8>& lhs, const batch<int32_t, 8>& rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_sllv_epi32(lhs, rhs);
#else
        return avx_detail::shift_impl([](int32_t lhs, int32_t s) { return lhs << s; }, lhs, rhs);
#endif
    }

    inline batch<int32_t, 8> operator>>(const batch<int32_t, 8>& lhs, const batch<int32_t, 8>& rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_srav_epi32(lhs, rhs);
#else
        return avx_detail::shift_impl([](int32_t lhs, int32_t s) { return lhs >> s; }, lhs, rhs);
#endif
    }

    inline batch<uint32_t, 8> operator<<(const batch<uint32_t, 8>& lhs, int32_t rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_slli_epi32(lhs, rhs);
#else
        XSIMD_SPLIT_AVX(lhs);
        __m128i res_low = _mm_slli_epi32(lhs_low, rhs);
        __m128i res_high = _mm_slli_epi32(lhs_high, rhs);
        XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
    }

    inline batch<uint32_t, 8> operator>>(const batch<uint32_t, 8>& lhs, int32_t rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_srli_epi32(lhs, rhs);
#else
        XSIMD_SPLIT_AVX(lhs);
        __m128i res_low = _mm_srli_epi32(lhs_low, rhs);
        __m128i res_high = _mm_srli_epi32(lhs_high, rhs);
        XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
    }

    inline batch<uint32_t, 8> operator<<(const batch<uint32_t, 8>& lhs, const batch<int32_t, 8>& rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_sllv_epi32(lhs, rhs);
#else
        return avx_detail::shift_impl([](uint32_t lhs, int32_t s) { return lhs << s; }, lhs, rhs);
#endif
    }

    inline batch<uint32_t, 8> operator>>(const batch<uint32_t, 8>& lhs, const batch<int32_t, 8>& rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_srlv_epi32(lhs, rhs);
#else
        return avx_detail::shift_impl([](uint32_t lhs, int32_t s) { return lhs >> s; }, lhs, rhs);
#endif
    }
}

#endif
