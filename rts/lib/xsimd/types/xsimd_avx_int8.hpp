/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_AVX_INT8_HPP
#define XSIMD_AVX_INT8_HPP

#include <cstdint>

#include "xsimd_base.hpp"
#include "xsimd_avx_int_base.hpp"

namespace xsimd
{

    /**************************
     * batch_bool<int8_t, 32> *
     **************************/

    template <>
    struct simd_batch_traits<batch_bool<int8_t, 32>>
    {
        using value_type = int8_t;
        static constexpr std::size_t size = 32;
        using batch_type = batch<int8_t, 32>;
        static constexpr std::size_t align = 32;
    };

    template <>
    struct simd_batch_traits<batch_bool<uint8_t, 32>>
    {
        using value_type = uint8_t;
        static constexpr std::size_t size = 32;
        using batch_type = batch<uint8_t, 32>;
        static constexpr std::size_t align = 32;
    };

    template <>
    class batch_bool<int8_t, 32> : public avx_int_batch_bool<int8_t, 32>
    {
    public:
        using avx_int_batch_bool::avx_int_batch_bool;
    };

    template <>
    class batch_bool<uint8_t, 32> : public avx_int_batch_bool<uint8_t, 32>
    {
    public:
        using avx_int_batch_bool::avx_int_batch_bool;
    };

    namespace detail
    {
        template <>
        struct batch_bool_kernel<int8_t, 32> : public avx_int_batch_bool_kernel<int8_t, 32>
        {
        };

        template <>
        struct batch_bool_kernel<uint8_t, 32> : public avx_int_batch_bool_kernel<uint8_t, 32>
        {
        };
    }

    /*********************
     * batch<int8_t, 32> *
     *********************/

    template <>
    struct simd_batch_traits<batch<int8_t, 32>>
    {
        using value_type = int8_t;
        static constexpr std::size_t size = 32;
        using batch_bool_type = batch_bool<int8_t, 32>;
        static constexpr std::size_t align = 32;
        using storage_type = __m256i;
    };

    template <>
    struct simd_batch_traits<batch<uint8_t, 32>>
    {
        using value_type = uint8_t;
        static constexpr std::size_t size = 32;
        using batch_bool_type = batch_bool<uint8_t, 32>;
        static constexpr std::size_t align = 32;
        using storage_type = __m256i;
    };

    template <>
    class batch<int8_t, 32> : public avx_int_batch<int8_t, 32>
    {
    public:

        using base_class = avx_int_batch<int8_t, 32>;
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

        XSIMD_DECLARE_LOAD_STORE_INT8(int8_t, 32)
        XSIMD_DECLARE_LOAD_STORE_LONG(int8_t, 32)
    };

    template <>
    class batch<uint8_t, 32> : public avx_int_batch<uint8_t, 32>
    {
    public:
        
        using base_class = avx_int_batch<uint8_t, 32>;
        using base_class::base_class;
        using base_class::load_aligned;
        using base_class::load_unaligned;
        using base_class::store_aligned;
        using base_class::store_unaligned;

        XSIMD_DECLARE_LOAD_STORE_INT8(uint8_t, 32)
        XSIMD_DECLARE_LOAD_STORE_LONG(uint8_t, 32)
    };

    batch<int8_t, 32> operator<<(const batch<int8_t, 32>& lhs, int32_t rhs);
    batch<int8_t, 32> operator>>(const batch<int8_t, 32>& lhs, int32_t rhs);
    batch<int8_t, 32> operator<<(const batch<int8_t, 32>& lhs, const batch<int8_t, 32>& rhs);
    batch<int8_t, 32> operator>>(const batch<int8_t, 32>& lhs, const batch<int8_t, 32>& rhs);
    batch<uint8_t, 32> operator<<(const batch<uint8_t, 32>& lhs, int32_t rhs);
    batch<uint8_t, 32> operator>>(const batch<uint8_t, 32>& lhs, int32_t rhs);
    batch<uint8_t, 32> operator<<(const batch<uint8_t, 32>& lhs, const batch<int8_t, 32>& rhs);
    batch<uint8_t, 32> operator>>(const batch<uint8_t, 32>& lhs, const batch<int8_t, 32>& rhs);

    /************************************
     * batch<int8_t, 32> implementation *
     ************************************/

    namespace detail
    {
        template <class T>
        struct int8_batch_kernel
            : avx_int_kernel_base<batch<T, 32>>
        {
            using batch_type = batch<T, 32>;
            using value_type = T;
            using batch_bool_type = batch_bool<T, 32>;

            constexpr static bool is_signed = std::is_signed<T>::value;

            static batch_type neg(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_sub_epi8(_mm256_setzero_si256(), rhs);
#else
                XSIMD_SPLIT_AVX(rhs);
                __m128i res_low = _mm_sub_epi8(_mm_setzero_si128(), rhs_low);
                __m128i res_high = _mm_sub_epi8(_mm_setzero_si128(), rhs_high);
                XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
            }

            static batch_type add(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_add_epi8(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_add_epi8, lhs, rhs);
#endif
            }

            static batch_type sub(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_sub_epi8(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_sub_epi8, lhs, rhs);
#endif
            }

            static batch_type sadd(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_adds_epi8(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_adds_epi8, lhs, rhs);
#endif
            }

            static batch_type ssub(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_subs_epi8(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_subs_epi8, lhs, rhs);
#endif
            }

            static batch_type mul(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                batch_type upper = _mm256_and_si256(_mm256_mullo_epi16(lhs, rhs), _mm256_srli_epi16(_mm256_set1_epi16(-1), 8));
                batch_type lower = _mm256_slli_epi16(_mm256_mullo_epi16(_mm256_srli_si256(lhs, 1), _mm256_srli_si256(rhs, 1)), 8);
                return _mm256_or_si256(upper, lower);
#else
                // Note implement with conversion to epi16
                XSIMD_MACRO_UNROLL_BINARY(*);
#endif
            }

            static batch_type div(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                auto to_float = [](__m256i val) {
                    // sign matters for conversion to epi32!
                    if (std::is_signed<T>::value)
                    {
                        return
                            _mm256_cvtepi32_ps(
                                _mm256_cvtepi8_epi32(
                                    _mm256_extractf128_si256(val, 0)
                                )
                            );
                    }
                    else
                    {
                        return
                            _mm256_cvtepi32_ps(
                                _mm256_cvtepu8_epi32(
                                    _mm256_extractf128_si256(val, 0)
                                )
                            );
                    }
                };

                auto to_int8 = [](__m256 x, __m256 y) {
                    auto v0 = _mm256_cvttps_epi32(x);
                    auto v1 = _mm256_cvttps_epi32(y);
                    // here the sign doesn't matter ... just an interpretation detail
                    auto a = _mm256_unpacklo_epi8(v0, v1);  // 08.. .... 19.. .... 4C.. .... 5D.. ....
                    auto b = _mm256_unpackhi_epi8(v0, v1);  // 2A.. .... 3B.. .... 6E.. .... 7F.. ....
                    auto c = _mm256_unpacklo_epi8(a, b);    // 028A .... .... .... 46CE ...
                    auto d = _mm256_unpackhi_epi8(a, b);    // 139B .... .... .... 57DF ...
                    auto e = _mm256_unpacklo_epi8(c, d);    // 0123 89AB .... .... 4567 CDEF ...
                    return _mm_unpacklo_epi32(_mm256_extractf128_si256(e, 0), 
                                              _mm256_extractf128_si256(e, 1));  // 0123 4567 89AB CDEF

                };

                auto insert = [](__m256i a, __m128i b)
                {
                    #if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                        return _mm256_inserti128_si256(a, b, 1);
                    #else
                        return _mm256_insertf128_si256(a, b, 1);
                    #endif
                };

                batch<float, 8> res_1 = _mm256_div_ps(to_float(lhs), to_float(rhs));
                batch<float, 8> res_2 = _mm256_div_ps(to_float(_mm256_permute4x64_epi64(lhs, 0x01)),
                                                      to_float(_mm256_permute4x64_epi64(rhs, 0x01)));
                batch<float, 8> res_3 = _mm256_div_ps(to_float(_mm256_permute4x64_epi64(lhs, 0x02)),
                                                      to_float(_mm256_permute4x64_epi64(rhs, 0x02)));
                batch<float, 8> res_4 = _mm256_div_ps(to_float(_mm256_permute4x64_epi64(lhs, 0x03)),
                                                      to_float(_mm256_permute4x64_epi64(rhs, 0x03)));

                return batch_type(
                    insert(_mm256_castsi128_si256(to_int8(res_1, res_2)),
                          to_int8(res_3, res_4))
                );
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
                return _mm256_cmpeq_epi8(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_cmpeq_epi8, lhs, rhs);
#endif
            }

            // TODO use conversion to int16_t
            static value_type hadd(const batch_type& lhs)
            {
                alignas(32) value_type tmp_lhs[32];
                lhs.store_aligned(&tmp_lhs[0]);
                value_type res = 0;
                unroller<32>([&](std::size_t i) {
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
                return _mm256_unpacklo_epi8(lhs, rhs);
            }

            static batch_type zip_hi(const batch_type& lhs, const batch_type& rhs)
            {
                return _mm256_unpackhi_epi8(lhs, rhs);
            }

        };

        template <>
        struct batch_kernel<int8_t, 32> : int8_batch_kernel<int8_t>
        {
            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_cmpgt_epi8(rhs, lhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_cmpgt_epi8, rhs, lhs);
#endif
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_min_epi8(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_min_epi8, lhs, rhs);
#endif
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_max_epi8(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_max_epi8, lhs, rhs);
#endif
            }

            static batch_type abs(const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_abs_epi8(rhs);
#else
                XSIMD_SPLIT_AVX(rhs);
                __m128i res_low = _mm_abs_epi8(rhs_low);
                __m128i res_high = _mm_abs_epi8(rhs_high);
                XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
            }
        };

        template <>
        struct batch_kernel<uint8_t, 32> : int8_batch_kernel<uint8_t>
        {
            static batch_bool_type lt(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                auto xor_lhs = _mm256_xor_si256(lhs, _mm256_set1_epi8(std::numeric_limits<int8_t>::lowest()));
                auto xor_rhs = _mm256_xor_si256(rhs, _mm256_set1_epi8(std::numeric_limits<int8_t>::lowest()));
                return _mm256_cmpgt_epi8(xor_rhs, xor_lhs);
#else
                XSIMD_SPLIT_AVX(lhs);
                XSIMD_SPLIT_AVX(rhs);
                auto xer = _mm_set1_epi8(std::numeric_limits<int8_t>::lowest());
                lhs_low  = _mm_xor_si128(lhs_low,  xer);
                lhs_high = _mm_xor_si128(lhs_high, xer);
                rhs_low  = _mm_xor_si128(rhs_low,  xer);
                rhs_high = _mm_xor_si128(rhs_high, xer);
                __m128i res_low =  _mm_cmpgt_epi8(rhs_low, lhs_low);
                __m128i res_high = _mm_cmpgt_epi8(rhs_high, lhs_high);
                XSIMD_RETURN_MERGED_SSE(res_low, res_high);
#endif
            }

            static batch_type min(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_min_epu8(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_min_epu8, lhs, rhs);
#endif
            }

            static batch_type max(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_max_epu8(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_max_epu8, lhs, rhs);
#endif
            }

            static batch_type abs(const batch_type& rhs)
            {
                return rhs;
            }

            static batch_type sadd(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_adds_epu8(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_adds_epu8, lhs, rhs);
#endif
            }

            static batch_type ssub(const batch_type& lhs, const batch_type& rhs)
            {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
                return _mm256_subs_epu8(lhs, rhs);
#else
                XSIMD_APPLY_SSE_FUNCTION(_mm_subs_epu8, lhs, rhs);
#endif
            }
        };
    }

    inline batch<int8_t, 32> operator<<(const batch<int8_t, 32>& lhs, int32_t rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_and_si256(_mm256_set1_epi8(0xFF << rhs), _mm256_slli_epi32(lhs, rhs));
#else
        return avx_detail::shift_impl([](int8_t val, int32_t s) { return val << s; }, lhs, rhs);
#endif
    }

    inline batch<int8_t, 32> operator>>(const batch<int8_t, 32>& lhs, int32_t rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        __m256i sign_mask = _mm256_set1_epi16((0xFF00 >> rhs) & 0x00FF);
        __m256i cmp_is_negative = _mm256_cmpgt_epi8(_mm256_setzero_si256(), lhs);
        __m256i res = _mm256_srai_epi16(lhs, rhs);
        return _mm256_or_si256(_mm256_and_si256(sign_mask, cmp_is_negative), _mm256_andnot_si256(sign_mask, res));
#else
        return avx_detail::shift_impl([](int8_t val, int32_t s) { return val >> s; }, lhs, rhs);
#endif
    }

    inline batch<int8_t, 32> operator<<(const batch<int8_t, 32>& lhs, const batch<int8_t, 32>& rhs)
    {
        return avx_detail::shift_impl([](int8_t val, int8_t s) { return val << s; }, lhs, rhs);
    }

    inline batch<int8_t, 32> operator>>(const batch<int8_t, 32>& lhs, const batch<int8_t, 32>& rhs)
    {
        return avx_detail::shift_impl([](int8_t val, int8_t s) { return val >> s; }, lhs, rhs);
    }

    XSIMD_DEFINE_LOAD_STORE_INT8(int8_t, 32, 32)
    XSIMD_DEFINE_LOAD_STORE_LONG(int8_t, 32, 32)

    inline batch<uint8_t, 32> operator<<(const batch<uint8_t, 32>& lhs, int32_t rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_and_si256(_mm256_set1_epi8(0xFF << rhs), _mm256_slli_epi32(lhs, rhs));
#else
        return avx_detail::shift_impl([](int8_t val, int32_t s) { return val << s; }, lhs, rhs);
#endif
    }

    inline batch<uint8_t, 32> operator>>(const batch<uint8_t, 32>& lhs, int32_t rhs)
    {
#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX2_VERSION
        return _mm256_and_si256(_mm256_set1_epi8(0xFF >> rhs), _mm256_srli_epi32(lhs, rhs));
#else
        return avx_detail::shift_impl([](uint8_t val, int32_t s) { return val >> s; }, lhs, rhs);
#endif
    }

    inline batch<uint8_t, 32> operator<<(const batch<uint8_t, 32>& lhs, const batch<int8_t, 32>& rhs)
    {
        return avx_detail::shift_impl([](uint8_t val, int8_t s) { return val << s; }, lhs, rhs);
    }

    inline batch<uint8_t, 32> operator>>(const batch<uint8_t, 32>& lhs, const batch<int8_t, 32>& rhs)
    {
        return avx_detail::shift_impl([](uint8_t val, int8_t s) { return val >> s; }, lhs, rhs);
    }

    XSIMD_DEFINE_LOAD_STORE_INT8(uint8_t, 32, 32)
    XSIMD_DEFINE_LOAD_STORE_LONG(uint8_t, 32, 32)
}

#endif
