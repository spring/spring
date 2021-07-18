/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_TRAITS_HPP
#define XSIMD_TRAITS_HPP

#include <type_traits>

#include "../types/xsimd_base.hpp"
#include "xsimd_types_include.hpp"

#undef XSIMD_BATCH_INT_SIZE
#undef XSIMD_BATCH_FLOAT_SIZE
#undef XSIMD_BATCH_DOUBLE_SIZE

#if XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX512_VERSION
#define XSIMD_BATCH_INT8_SIZE 64
#define XSIMD_BATCH_INT16_SIZE 32
#define XSIMD_BATCH_INT32_SIZE 16
#define XSIMD_BATCH_INT64_SIZE 8
#define XSIMD_BATCH_FLOAT_SIZE 16
#define XSIMD_BATCH_DOUBLE_SIZE 8
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_AVX_VERSION
#define XSIMD_BATCH_INT8_SIZE 32
#define XSIMD_BATCH_INT16_SIZE 16
#define XSIMD_BATCH_INT32_SIZE 8
#define XSIMD_BATCH_INT64_SIZE 4
#define XSIMD_BATCH_FLOAT_SIZE 8
#define XSIMD_BATCH_DOUBLE_SIZE 4
#elif XSIMD_X86_INSTR_SET >= XSIMD_X86_SSE2_VERSION
#define XSIMD_BATCH_INT8_SIZE 16
#define XSIMD_BATCH_INT16_SIZE 8
#define XSIMD_BATCH_INT32_SIZE 4
#define XSIMD_BATCH_INT64_SIZE 2
#define XSIMD_BATCH_FLOAT_SIZE 4
#define XSIMD_BATCH_DOUBLE_SIZE 2
#elif XSIMD_ARM_INSTR_SET >= XSIMD_ARM7_NEON_VERSION
#define XSIMD_BATCH_INT8_SIZE 16
#define XSIMD_BATCH_INT16_SIZE 8
#define XSIMD_BATCH_INT32_SIZE 4
#define XSIMD_BATCH_INT64_SIZE 2
#define XSIMD_BATCH_FLOAT_SIZE 4
#if XSIMD_ARM_INSTR_SET >= XSIMD_ARM8_64_NEON_VERSION
#define XSIMD_BATCH_DOUBLE_SIZE 2
#elif defined(XSIMD_ENABLE_FALLBACK)
#define XSIMD_BATCH_DOUBLE_SIZE 1
#endif
#endif

namespace xsimd
{
    template <class T>
    struct simd_traits
    {
        using type = T;
        using bool_type = bool;
        static constexpr size_t size = 1;
    };

    template <class T>
    struct revert_simd_traits
    {
        using type = T;
        static constexpr size_t size = simd_traits<type>::size;
    };

    template <class T>
    using simd_type = typename simd_traits<T>::type;

    template <class T>
    using simd_bool_type = typename simd_traits<T>::bool_type;

    template <class T>
    using revert_simd_type = typename revert_simd_traits<T>::type;

#ifdef XSIMD_BATCH_FLOAT_SIZE
    template <>
    struct simd_traits<int8_t>
    {
        using type = batch<int8_t, XSIMD_BATCH_INT8_SIZE>;
        using bool_type = simd_batch_traits<type>::batch_bool_type;
        static constexpr size_t size = type::size;
    };

    template <>
    struct revert_simd_traits<batch<int8_t, XSIMD_BATCH_INT8_SIZE>>
    {
        using type = int8_t;
        static constexpr size_t size = simd_traits<type>::size;
    };

    template <>
    struct simd_traits<uint8_t>
    {
        using type = batch<uint8_t, XSIMD_BATCH_INT8_SIZE>;
        using bool_type = simd_batch_traits<type>::batch_bool_type;
        static constexpr size_t size = type::size;
    };

    template <>
    struct revert_simd_traits<batch<uint8_t, XSIMD_BATCH_INT8_SIZE>>
    {
        using type = uint8_t;
        static constexpr size_t size = simd_traits<type>::size;
    };

    template <>
    struct simd_traits<int16_t>
    {
        using type = batch<int16_t, XSIMD_BATCH_INT16_SIZE>;
        using bool_type = simd_batch_traits<type>::batch_bool_type;
        static constexpr size_t size = type::size;
    };

    template <>
    struct revert_simd_traits<batch<int16_t, XSIMD_BATCH_INT16_SIZE>>
    {
        using type = int16_t;
        static constexpr size_t size = simd_traits<type>::size;
    };

    template <>
    struct simd_traits<uint16_t>
    {
        using type = batch<uint16_t, XSIMD_BATCH_INT16_SIZE>;
        using bool_type = simd_batch_traits<type>::batch_bool_type;
        static constexpr size_t size = type::size;
    };

    template <>
    struct revert_simd_traits<batch<uint16_t, XSIMD_BATCH_INT16_SIZE>>
    {
        using type = uint16_t;
        static constexpr size_t size = simd_traits<type>::size;
    };

    template <>
    struct simd_traits<char>
        : std::conditional<std::is_signed<char>::value, simd_traits<int8_t>, simd_traits<uint8_t>>::type
    {
    };

    template <>
    struct simd_traits<int32_t>
    {
        using type = batch<int32_t, XSIMD_BATCH_INT32_SIZE>;
        using bool_type = simd_batch_traits<type>::batch_bool_type;
        static constexpr size_t size = type::size;
    };

    template <>
    struct revert_simd_traits<batch<int32_t, XSIMD_BATCH_INT32_SIZE>>
    {
        using type = int32_t;
        static constexpr size_t size = simd_traits<type>::size;
    };

    template <>
    struct simd_traits<uint32_t>
    {
        using type = batch<uint32_t, XSIMD_BATCH_INT32_SIZE>;
        using bool_type = simd_batch_traits<type>::batch_bool_type;
        static constexpr size_t size = type::size;
    };

    template <>
    struct revert_simd_traits<batch<uint32_t, XSIMD_BATCH_INT32_SIZE>>
    {
        using type = uint32_t;
        static constexpr size_t size = simd_traits<type>::size;
    };

// On some architectures long is a different type from int32_t or int64_t
#ifdef XSIMD_32_BIT_ABI
    template <>
    struct simd_traits<long> : simd_traits<int32_t>
    {
    };

    template <>
    struct revert_simd_traits<batch<long, XSIMD_BATCH_INT32_SIZE>>
    {
        using type = long;
        static constexpr size_t size = simd_traits<type>::size;
    };

    template <>
    struct simd_traits<unsigned long> : simd_traits<uint32_t>
    {
    };

    template <>
    struct revert_simd_traits<batch<unsigned long, XSIMD_BATCH_INT32_SIZE>>
    {
        using type = unsigned long;
        static constexpr size_t size = simd_traits<type>::size;
    };
#endif // XSIMD_32_BIT_ABI

    template <>
    struct simd_traits<int64_t>
    {
        using type = batch<int64_t, XSIMD_BATCH_INT64_SIZE>;
        using bool_type = simd_batch_traits<type>::batch_bool_type;
        static constexpr size_t size = type::size;
    };

    template <>
    struct revert_simd_traits<batch<int64_t, XSIMD_BATCH_INT64_SIZE>>
    {
        using type = int64_t;
        static constexpr size_t size = simd_traits<type>::size;
    };

    template <>
    struct simd_traits<uint64_t>
    {
        using type = batch<uint64_t, XSIMD_BATCH_INT64_SIZE>;
        using bool_type = simd_batch_traits<type>::batch_bool_type;
        static constexpr size_t size = type::size;
    };

    template <>
    struct revert_simd_traits<batch<uint64_t, XSIMD_BATCH_INT64_SIZE>>
    {
        using type = uint64_t;
        static constexpr size_t size = simd_traits<type>::size;
    };

    template <>
    struct simd_traits<float>
    {
        using type = batch<float, XSIMD_BATCH_FLOAT_SIZE>;
        using bool_type = simd_batch_traits<type>::batch_bool_type;
        static constexpr size_t size = type::size;
    };

    template <>
    struct revert_simd_traits<batch<float, XSIMD_BATCH_FLOAT_SIZE>>
    {
        using type = float;
        static constexpr size_t size = simd_traits<type>::size;
    };

    template <>
    struct simd_traits<std::complex<float>>
    {
        using type = batch<std::complex<float>, XSIMD_BATCH_FLOAT_SIZE>;
        using bool_type = simd_batch_traits<type>::batch_bool_type;
        static constexpr size_t size = type::size;
    };

    template <>
    struct revert_simd_traits<batch<std::complex<float>, XSIMD_BATCH_FLOAT_SIZE>>
    {
        using type = std::complex<float>;
        static constexpr size_t size = simd_traits<type>::size;
    };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
    template <bool i3ec>
    struct simd_traits<xtl::xcomplex<float, float, i3ec>>
    {
        using type = batch<xtl::xcomplex<float, float, i3ec>, XSIMD_BATCH_FLOAT_SIZE>;
        using bool_type = typename simd_batch_traits<type>::batch_bool_type;
        static constexpr size_t size = type::size;
    };

    template <bool i3ec>
    struct revert_simd_traits<batch<xtl::xcomplex<float, float, i3ec>, XSIMD_BATCH_FLOAT_SIZE>>
    {
        using type = xtl::xcomplex<float, float, i3ec>;
        static constexpr size_t size = simd_traits<type>::size;
    };
#endif // XSIMD_ENABLE_XTL_COMPLEX
#endif // XSIMD_BATCH_FLOAT_SIZE

#ifdef XSIMD_BATCH_DOUBLE_SIZE
    template <>
    struct simd_traits<double>
    {
        using type = batch<double, XSIMD_BATCH_DOUBLE_SIZE>;
        using bool_type = simd_batch_traits<type>::batch_bool_type;
        static constexpr size_t size = type::size;
    };

    template <>
    struct revert_simd_traits<batch<double, XSIMD_BATCH_DOUBLE_SIZE>>
    {
        using type = double;
        static constexpr size_t size = simd_traits<type>::size;
    };

    template <>
    struct simd_traits<std::complex<double>>
    {
        using type = batch<std::complex<double>, XSIMD_BATCH_DOUBLE_SIZE>;
        using bool_type = simd_batch_traits<type>::batch_bool_type;
        static constexpr size_t size = type::size;
    };

    template <>
    struct revert_simd_traits<batch<std::complex<double>, XSIMD_BATCH_DOUBLE_SIZE>>
    {
        using type = std::complex<double>;
        static constexpr size_t size = simd_traits<type>::size;
    };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
    template <bool i3ec>
    struct simd_traits<xtl::xcomplex<double, double, i3ec>>
    {
        using type = batch<xtl::xcomplex<double, double, i3ec>, XSIMD_BATCH_DOUBLE_SIZE>;
        using bool_type = typename simd_batch_traits<type>::batch_bool_type;
        static constexpr size_t size = type::size;
    };

    template <bool i3ec>
    struct revert_simd_traits<batch<xtl::xcomplex<double, double, i3ec>, XSIMD_BATCH_DOUBLE_SIZE>>
    {
        using type = xtl::xcomplex<double, double, i3ec>;
        static constexpr size_t size = simd_traits<type>::size;
    };
#endif // XSIMD_ENABLE_XTL_COMPLEX
#endif // XSIMD_BATCH_DOUBLE_SIZE

    /********************
     * simd_return_type *
     ********************/

    namespace detail
    {
        template <class T1, class T2>
        struct simd_condition
        {
            static constexpr bool value =
                (std::is_same<T1, T2>::value && !std::is_same<T1, bool>::value) ||
                (std::is_same<T1, bool>::value && !std::is_same<T2, bool>::value) ||
                std::is_same<T1, float>::value ||
                std::is_same<T1, double>::value ||
                std::is_same<T1, int8_t>::value ||
                std::is_same<T1, uint8_t>::value ||
                std::is_same<T1, int16_t>::value ||
                std::is_same<T1, uint16_t>::value ||
                std::is_same<T1, int32_t>::value ||
                std::is_same<T1, uint32_t>::value ||
                std::is_same<T1, int64_t>::value ||
                std::is_same<T1, uint64_t>::value ||
                std::is_same<T1, char>::value ||
                detail::is_complex<T1>::value;
        };

        template <class T1, class T2, std::size_t N>
        struct simd_return_type_impl
            : std::enable_if<simd_condition<T1, T2>::value, batch<T2, N>>
        {
        };
        template <std::size_t N>
        struct simd_return_type_impl<char, char, N>
            : std::conditional<std::is_signed<char>::value,
                               simd_return_type_impl<int8_t, int8_t, N>,
                               simd_return_type_impl<uint8_t, uint8_t, N>>::type
        {
        };

        template <class T2, std::size_t N>
        struct simd_return_type_impl<bool, T2, N>
            : std::enable_if<simd_condition<bool, T2>::value, batch_bool<T2, N>>
        {
        };

        template <class T1, class T2, std::size_t N>
        struct simd_return_type_impl<std::complex<T1>, T2, N>
            : std::enable_if<simd_condition<T1, T2>::value, batch<std::complex<T2>, N>>
        {
        };

        template <class T1, class T2, std::size_t N>
        struct simd_return_type_impl<std::complex<T1>, std::complex<T2>, N>
            : std::enable_if<simd_condition<T1, T2>::value, batch<std::complex<T2>, N>>
        {
        };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
        template <class T1, bool i3ec, class T2, std::size_t N>
        struct simd_return_type_impl<xtl::xcomplex<T1, T1, i3ec>, T2, N>
            : std::enable_if<simd_condition<T1, T2>::value, batch<xtl::xcomplex<T2, T2, i3ec>, N>>
        {
        };

        template <class T1, class T2, bool i3ec, std::size_t N>
        struct simd_return_type_impl<xtl::xcomplex<T1, T1, i3ec>, xtl::xcomplex<T2, T2, i3ec>, N>
            : std::enable_if<simd_condition<T1, T2>::value, batch<xtl::xcomplex<T2, T2, i3ec>, N>>
        {
        };
#endif // XSIMD_ENABLE_XTL_COMPLEX
    }

    template <class T1, class T2, std::size_t N = simd_traits<T2>::size>
    using simd_return_type = typename detail::simd_return_type_impl<T1, T2, N>::type;

    /************
     * is_batch *
     ************/

    template <class V>
    struct is_batch : std::false_type
    {
    };

    template <class T, std::size_t N>
    struct is_batch<batch<T, N>> : std::true_type
    {
    };

    /*****************
     * is_batch_bool *
     *****************/

    template <class V>
    struct is_batch_bool : std::false_type
    {
    };

    template <class T, std::size_t N>
    struct is_batch_bool<batch_bool<T, N>> : std::true_type
    {
    };

    /********************
     * is_batch_complex *
     ********************/

    template <class V>
    struct is_batch_complex : std::false_type
    {
    };

    template <class T, std::size_t N>
    struct is_batch_complex<batch<std::complex<T>, N>> : std::true_type
    {
    };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
    template <class T, bool i3ec, std::size_t N>
    struct is_batch_complex<batch<xtl::xcomplex<T, T, i3ec>, N>> : std::true_type
    {
    };
#endif //XSIMD_ENABLE_XTL_COMPLEX

}

#endif
