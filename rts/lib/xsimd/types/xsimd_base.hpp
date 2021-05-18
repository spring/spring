/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_BASE_HPP
#define XSIMD_BASE_HPP

#include <cstddef>
#include <cstring>
#include <complex>
#include <iterator>
#include <ostream>
#include <type_traits>

#ifdef XSIMD_ENABLE_XTL_COMPLEX
#include "xtl/xcomplex.hpp"
#endif

#include "../memory/xsimd_alignment.hpp"
#include "xsimd_utils.hpp"
#include "xsimd_base_bool.hpp"
#include "xsimd_base_constant.hpp"

namespace xsimd
{
    template <class X>
    class simd_base;

    template <class T, size_t N>
    class batch;

    namespace detail
    {
        template <class T, std::size_t N>
        struct batch_kernel;
    }

    template <class X>
    struct simd_batch_inner_types
    {
        using batch_reference = X&;
        using const_batch_reference = const X&;
    };

    template <class T>
    using batch_type_t = typename T::batch_type;

    namespace detail
    {
        template <class X>
        struct get_real_batch_type
        {
            using batch_type = batch_type_t<X>;
        };

        template <class T, std::size_t N>
        struct get_real_batch_type<batch<std::complex<T>, N>>
        {
            using batch_type = typename batch<std::complex<T>, N>::real_batch;
        };

        #ifdef XSIMD_ENABLE_XTL_COMPLEX
            template <class T, bool i3ec, std::size_t N>
            struct get_real_batch_type<batch<xtl::xcomplex<T, T, i3ec>, N>>
            {
                using batch_type = typename batch<xtl::xcomplex<T, T, i3ec>, N>::real_batch;
            };
        #endif
    }

    template <class T>
    using real_batch_type_t = typename detail::get_real_batch_type<typename T::batch_type>::batch_type;

    namespace detail
    {
        template <class X>
        struct is_simd_type : std::is_base_of<simd_base<X>, X>
        {
        };
    }

    template <class X>
    using enable_if_simd_t = typename std::enable_if<detail::is_simd_type<X>::value, batch_type_t<X>>::type;

    /*************
     * simd_base *
     *************/

    /**
     * @class simd_base
     * @brief Base class for batches and batch proxies.
     *
     * The simd_base class is the base class for all classes
     * representing a batch or a batch proxy. It provides very few
     * methods, so concrete batches usually inherit from intermediate
     * classes.
     *
     * @tparam X The most derived type
     */
    template <class X>
    class simd_base
    {
    public:

        using derived_class = X;
        using batch_reference = typename simd_batch_inner_types<X>::batch_reference;
        using const_batch_reference = typename simd_batch_inner_types<X>::const_batch_reference;

        batch_reference operator()();
        const_batch_reference operator()() const;

        X& derived_cast();
        const X& derived_cast() const;
    };

    /**************
     * simd_batch *
     **************/

    /**
     * @class simd_batch
     * @brief Base class for batch of integer or floating point values.
     *
     * The simd_batch class is the base class for all classes representing
     * a batch of integer or floating point values. Each type of batch (i.e.
     * a class inheriting from simd_batch) has its dedicated type of boolean
     * batch (i.e. a class inheriting from simd_batch_bool) for logical operations.
     *
     * @tparam X The derived type
     * @sa simd_batch_bool
     */
    template <class X>
    class simd_batch : public simd_base<X>
    {
    public:

        using base_type = simd_base<X>;
        using batch_reference = typename base_type::batch_reference;
        using const_batch_reference = typename base_type::const_batch_reference;
        using batch_type = X;
        using value_type = typename simd_batch_traits<X>::value_type;
        static constexpr std::size_t size = simd_batch_traits<X>::size;
        using storage_type = typename simd_batch_traits<X>::storage_type;
        using batch_bool_type = typename simd_batch_traits<X>::batch_bool_type;

        using iterator = value_type*;
        using const_iterator = const value_type*;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        static X broadcast(value_type v);

        template <class T>
        static X from_unaligned(T* src);

        template <class T>
        static X from_aligned(T* src);

        X& operator+=(const X& rhs);
        X& operator+=(const value_type& rhs);

        X& operator-=(const X& rhs);
        X& operator-=(const value_type& rhs);

        X& operator*=(const X& rhs);
        X& operator*=(const value_type& rhs);

        X& operator/=(const X& rhs);
        X& operator/=(const value_type& rhs);

        X& operator&=(const X& rhs);
        X& operator|=(const X& rhs);
        X& operator^=(const X& rhs);

        X& operator++();
        X& operator++(int);

        X& operator--();
        X& operator--(int);

        X& load_aligned(const char* src);
        X& load_unaligned(const char* src);

        void store_aligned(char* dst) const;
        void store_unaligned(char* dst) const;

        batch_reference get();
        const_batch_reference get() const;

        value_type& operator[](std::size_t index);
        const value_type& operator[](std::size_t index) const;

        iterator begin();
        iterator end();
        const_iterator begin() const;
        const_iterator end() const;
        const_iterator cbegin() const;
        const_iterator cend() const;

        reverse_iterator rbegin();
        reverse_iterator rend();
        const_reverse_iterator rbegin() const;
        const_reverse_iterator rend() const;
        const_reverse_iterator crbegin() const;
        const_reverse_iterator crend() const;

    protected:

        simd_batch() = default;
        ~simd_batch() = default;

        simd_batch(const simd_batch&) = default;
        simd_batch& operator=(const simd_batch&) = default;

        simd_batch(simd_batch&&) = default;
        simd_batch& operator=(simd_batch&&) = default;

        constexpr simd_batch(storage_type value);

        using char_itype =
            typename std::conditional<std::is_signed<char>::value, int8_t, uint8_t>::type;

        union
        {
            storage_type m_value;
            value_type m_array[size];
        };
    };



    template <class X>
    typename simd_batch_traits<X>::batch_bool_type
    operator!(const simd_base<X>& rhs);

    template <class X>
    batch_type_t<X> min(const simd_base<X>& lhs, const simd_base<X>& rhs);

    template <class X>
    batch_type_t<X> max(const simd_base<X>& lhs, const simd_base<X>& rhs);

    template <class X>
    batch_type_t<X> fmin(const simd_base<X>& lhs, const simd_base<X>& rhs);

    template <class X>
    batch_type_t<X> fmax(const simd_base<X>& lhs, const simd_base<X>& rhs);

    template <class X>
    real_batch_type_t<X> abs(const simd_base<X>& rhs);

    template <class X>
    batch_type_t<X> fabs(const simd_base<X>& rhs);

    template <class X>
    batch_type_t<X> sqrt(const simd_base<X>& rhs);

    template <class X>
    batch_type_t<X> fma(const simd_base<X>& x, const simd_base<X>& y, const simd_base<X>& z);

    template <class X>
    batch_type_t<X> fms(const simd_base<X>& x, const simd_base<X>& y, const simd_base<X>& z);

    template <class X>
    batch_type_t<X> fnma(const simd_base<X>& x, const simd_base<X>& y, const simd_base<X>& z);

    template <class X>
    batch_type_t<X> fnms(const simd_base<X>& x, const simd_base<X>& y, const simd_base<X>& z);

    template <class X>
    typename simd_batch_traits<X>::value_type
    hadd(const simd_base<X>& rhs);

    template <class X>
    enable_if_simd_t<X> haddp(const X* row);

    template <class X>
    batch_type_t<X> select(const typename simd_batch_traits<X>::batch_bool_type& cond, const simd_base<X>& a, const simd_base<X>& b);

    template <class X>
    batch_type_t<X> zip_lo(const simd_base<X>& lhs, const simd_base<X>& rhs);

    template <class X>
    batch_type_t<X> zip_hi(const simd_base<X>& lhs, const simd_base<X>& rhs);

    template <class X>
    typename simd_batch_traits<X>::batch_bool_type
    isnan(const simd_base<X>& x);

    template <class X>
    std::ostream& operator<<(std::ostream& out, const simd_batch<X>& rhs);

    /***************************
     * generic batch operators *
     ***************************/

    template <class T, std::size_t N>
    batch<T, N> operator&&(const batch<T, N>& lhs, const batch<T, N>& rhs);

    template <class T, std::size_t N>
    batch<T, N> operator||(const batch<T, N>& lhs, const batch<T, N>& rhs);

    template <class T, std::size_t N>
    batch<T, N> operator<<(const batch<T, N>& lhs, const batch<T, N>& rhs);

    template <class T, std::size_t N>
    batch<T, N> operator>>(const batch<T, N>& lhs, const batch<T, N>& rhs);

    /**************************
     * batch cast functions *
     **************************/

    // Provides a static_cast from batch<T_in, N> to batch<T_out, N>
    template <class T_in, class T_out, std::size_t N>
    struct batch_cast_impl
    {
        template <std::size_t... I>
        static inline batch<T_out, N> run_impl(const batch<T_in, N>& x, detail::index_sequence<I...>)
        {
            return batch<T_out, N>(static_cast<T_out>(x[I])...);
        }

    public:
        static inline batch<T_out, N> run(const batch<T_in, N>& x)
        {
            return run_impl(x, detail::make_index_sequence<N>{});
        }
    };

    template <class T, std::size_t N>
    struct batch_cast_impl<T, T, N>
    {
        static inline batch<T, N> run(const batch<T, N>& x)
        {
            return x;
        }
    };

    // Shorthand for defining an intrinsic-based batch_cast implementation
    #define XSIMD_BATCH_CAST_INTRINSIC(T_IN, T_OUT, N, INTRINSIC)               \
        template <>                                                             \
        struct batch_cast_impl<T_IN, T_OUT, N>                                  \
        {                                                                       \
            static inline batch<T_OUT, N> run(const batch<T_IN, N>& x)          \
            {                                                                   \
                return INTRINSIC(x);                                            \
            }                                                                   \
        };

    // Shorthand for defining an intrinsic-based batch_cast implementation that requires 2 intrinsics
    #define XSIMD_BATCH_CAST_INTRINSIC2(T_IN, T_OUT, N, INTRINSIC1, INTRINSIC2) \
        template <>                                                             \
        struct batch_cast_impl<T_IN, T_OUT, N>                                  \
        {                                                                       \
            static inline batch<T_OUT, N> run(const batch<T_IN, N>& x)          \
            {                                                                   \
                return INTRINSIC2(INTRINSIC1(x));                               \
            }                                                                   \
        };

    // Shorthand for defining an implicit batch_cast implementation
    #define XSIMD_BATCH_CAST_IMPLICIT(T_IN, T_OUT, N)                           \
        template <>                                                             \
        struct batch_cast_impl<T_IN, T_OUT, N>                                  \
        {                                                                       \
            static inline batch<T_OUT, N> run(const batch<T_IN, N>& x)          \
            {                                                                   \
                return batch<T_OUT, N>(x);                                      \
            }                                                                   \
        };

    /**************************
     * bitwise cast functions *
     **************************/

    // Provides a reinterpret_cast from batch<T_in, N_in> to batch<T_out, N_out>
    template <class B_in, class B_out>
    struct bitwise_cast_impl;

    // Shorthand for defining an intrinsic-based bitwise_cast implementation
    #define XSIMD_BITWISE_CAST_INTRINSIC(T_IN, N_IN, T_OUT, N_OUT, INTRINSIC)  \
        template <>                                                            \
        struct bitwise_cast_impl<batch<T_IN, N_IN>, batch<T_OUT, N_OUT>>       \
        {                                                                      \
            static inline batch<T_OUT, N_OUT> run(const batch<T_IN, N_IN>& x)  \
            {                                                                  \
                return INTRINSIC(x);                                           \
            }                                                                  \
        };


    // Backwards-compatible interface to bitwise_cast_impl
    template <class B, std::size_t N = simd_batch_traits<B>::size>
    B bitwise_cast(const batch<float, N>& x);

    template <class B, std::size_t N = simd_batch_traits<B>::size>
    B bitwise_cast(const batch<double, N>& x);

    template <class B, std::size_t N = simd_batch_traits<B>::size>
    B bitwise_cast(const batch<int32_t, N>& x);

    template <class B, std::size_t N = simd_batch_traits<B>::size>
    B bitwise_cast(const batch<int64_t, N>& x);

    template <class T, std::size_t N>
    batch<T, N> bitwise_cast(const batch_bool<T, N>& src);

    /****************
     * helper macro *
     ****************/

#define XSIMD_DECLARE_LOAD_STORE(TYPE, N, CVT_TYPE)                            \
    batch<TYPE, N>& load_aligned(const CVT_TYPE*);                             \
    batch<TYPE, N>& load_unaligned(const CVT_TYPE*);                           \
    void store_aligned(CVT_TYPE* dst) const;                                   \
    void store_unaligned(CVT_TYPE* dst) const;

#define XSIMD_DEFINE_LOAD_STORE(TYPE, N, CVT_TYPE, ALIGNMENT)                  \
    inline batch<TYPE, N>& batch<TYPE, N>::load_aligned(const CVT_TYPE* src)   \
    {                                                                          \
        alignas(ALIGNMENT) TYPE tmp[N];                                        \
        unroller<N>([&](std::size_t i) {                                       \
            tmp[i] = static_cast<TYPE>(src[i]);                                \
        });                                                                    \
        return load_aligned(tmp);                                              \
    }                                                                          \
    inline batch<TYPE, N>& batch<TYPE, N>::load_unaligned(const CVT_TYPE* src) \
    {                                                                          \
        return load_aligned(src);                                              \
    }                                                                          \
    inline void batch<TYPE, N>::store_aligned(CVT_TYPE* dst) const             \
    {                                                                          \
        alignas(ALIGNMENT) TYPE tmp[N];                                        \
        store_aligned(tmp);                                                    \
        unroller<N>([&](std::size_t i) {                                       \
            dst[i] = static_cast<CVT_TYPE>(tmp[i]);                            \
        });                                                                    \
    }                                                                          \
    inline void batch<TYPE, N>::store_unaligned(CVT_TYPE* dst) const           \
    {                                                                          \
        return store_aligned(dst);                                             \
    }

#ifdef XSIMD_32_BIT_ABI

#define XSIMD_DECLARE_LOAD_STORE_LONG(TYPE, N)                                 \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, long)                                   \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, unsigned long)                           \

    namespace detail
    {
        template <class T>
        struct get_int_type;

        template <>
        struct get_int_type<long>
        {
            using type = int32_t;
        };

        template <>
        struct get_int_type<unsigned long>
        {
            using type = uint32_t;
        };

        template <class T>
        using get_int_type_t = typename get_int_type<T>::type;
    }

#define XSIMD_DEFINE_LOAD_STORE_LONG_IMPL(TYPE, N, CVT_TYPE, ALIGNMENT)        \
    inline batch<TYPE, N>& batch<TYPE, N>::load_aligned(const CVT_TYPE* src)   \
    {                                                                          \
        using int_type = detail::get_int_type_t<CVT_TYPE>;                     \
        return this->load_aligned(reinterpret_cast<const int_type*>(src));     \
    }                                                                          \
    inline batch<TYPE, N>& batch<TYPE, N>::load_unaligned(const CVT_TYPE* src) \
    {                                                                          \
        using int_type = detail::get_int_type_t<CVT_TYPE>;                     \
        return this->load_unaligned(reinterpret_cast<const int_type*>(src));   \
    }                                                                          \
    inline void batch<TYPE, N>::store_aligned(CVT_TYPE* dst) const             \
    {                                                                          \
        using int_type = detail::get_int_type_t<CVT_TYPE>;                     \
        this->store_aligned(reinterpret_cast<int_type*>(dst));                 \
    }                                                                          \
    inline void batch<TYPE, N>::store_unaligned(CVT_TYPE* dst) const           \
    {                                                                          \
        using int_type = detail::get_int_type_t<CVT_TYPE>;                     \
        this->store_unaligned(reinterpret_cast<int_type*>(dst));               \
    }                                                                          \

#define XSIMD_DEFINE_LOAD_STORE_LONG(TYPE, N, ALIGNMENT)                       \
    XSIMD_DEFINE_LOAD_STORE_LONG_IMPL(TYPE, N, long, ALIGNMENT)                \
    XSIMD_DEFINE_LOAD_STORE_LONG_IMPL(TYPE, N, unsigned long, ALIGNMENT)       \

#else

#define XSIMD_DECLARE_LOAD_STORE_LONG(TYPE, N)
#define XSIMD_DEFINE_LOAD_STORE_LONG(TYPE, N, ALIGNMENT)

#endif // XSIMD_32_BIT_ABI

#define XSIMD_DECLARE_LOAD_STORE_INT8(TYPE, N)                                 \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, bool)                                    \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, int16_t)                                 \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, uint16_t)                                \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, int32_t)                                 \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, uint32_t)                                \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, int64_t)                                 \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, uint64_t)                                \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, float)                                   \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, double)

#define XSIMD_DEFINE_LOAD_STORE_INT8(TYPE, N, ALIGNMENT)                       \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, bool, ALIGNMENT)                          \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, int16_t, ALIGNMENT)                       \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, uint16_t, ALIGNMENT)                      \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, int32_t, ALIGNMENT)                       \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, uint32_t, ALIGNMENT)                      \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, int64_t, ALIGNMENT)                       \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, uint64_t, ALIGNMENT)                      \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, float, ALIGNMENT)                         \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, double, ALIGNMENT)

#define XSIMD_DECLARE_LOAD_STORE_INT16(TYPE, N)                                \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, bool)                                    \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, int8_t)                                  \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, uint8_t)                                 \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, int32_t)                                 \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, uint32_t)                                \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, int64_t)                                 \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, uint64_t)                                \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, float)                                   \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, double)

#define XSIMD_DEFINE_LOAD_STORE_INT16(TYPE, N, ALIGNMENT)                      \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, bool, ALIGNMENT)                          \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, int8_t, ALIGNMENT)                        \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, uint8_t, ALIGNMENT)                       \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, int32_t, ALIGNMENT)                       \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, uint32_t, ALIGNMENT)                      \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, int64_t, ALIGNMENT)                       \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, uint64_t, ALIGNMENT)                      \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, float, ALIGNMENT)                         \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, double, ALIGNMENT)

#define XSIMD_DECLARE_LOAD_STORE_INT32(TYPE, N)                                \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, bool)                                    \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, int8_t)                                  \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, uint8_t)                                 \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, int16_t)                                 \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, uint16_t)                                \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, int64_t)                                 \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, uint64_t)                                \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, float)                                   \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, double)

#define XSIMD_DEFINE_LOAD_STORE_INT32(TYPE, N, ALIGNMENT)                      \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, bool, ALIGNMENT)                          \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, int8_t, ALIGNMENT)                        \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, uint8_t, ALIGNMENT)                       \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, int16_t, ALIGNMENT)                       \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, uint16_t, ALIGNMENT)                      \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, int64_t, ALIGNMENT)                       \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, uint64_t, ALIGNMENT)                      \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, float, ALIGNMENT)                         \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, double, ALIGNMENT)

#define XSIMD_DECLARE_LOAD_STORE_INT64(TYPE, N)                                \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, bool)                                    \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, int8_t)                                  \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, uint8_t)                                 \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, int16_t)                                 \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, uint16_t)                                \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, int32_t)                                 \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, uint32_t)                                \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, float)                                   \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, double)

#define XSIMD_DEFINE_LOAD_STORE_INT64(TYPE, N, ALIGNMENT)                      \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, bool, ALIGNMENT)                          \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, int8_t, ALIGNMENT)                        \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, uint8_t, ALIGNMENT)                       \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, int16_t, ALIGNMENT)                       \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, uint16_t, ALIGNMENT)                      \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, int32_t, ALIGNMENT)                       \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, uint32_t, ALIGNMENT)                      \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, float, ALIGNMENT)                         \
    XSIMD_DEFINE_LOAD_STORE(TYPE, N, double, ALIGNMENT)

#define XSIMD_DECLARE_LOAD_STORE_ALL(TYPE, N)                                  \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, bool)                                    \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, int8_t)                                  \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, uint8_t)                                 \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, int16_t)                                 \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, uint16_t)                                \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, int32_t)                                 \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, uint32_t)                                \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, int64_t)                                 \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, uint64_t)                                \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, float)                                   \
    XSIMD_DECLARE_LOAD_STORE(TYPE, N, double)

#define XSIMD_DEFINE_BITWISE_CAST(TYPE, N)                                     \
    inline batch<TYPE, N> bitwise_cast(const batch_bool<TYPE, N>& src)         \
    {                                                                          \
        TYPE z(0);                                                             \
        return select(src, batch<TYPE, N>(TYPE(~z)), batch<TYPE, N>(z));       \
    }

#define XSIMD_DEFINE_BITWISE_CAST_FLOAT(TYPE, N)                               \
    inline batch<TYPE, N> bitwise_cast(const batch_bool<TYPE, N>& src)         \
    {                                                                          \
        TYPE z0(0), z1(0);                                                     \
        using int_type = as_unsigned_integer_t<TYPE>;                          \
        int_type value(~int_type(0));                                          \
        std::memcpy(&z1, &value, sizeof(int_type));                            \
        return select(src, batch<TYPE, N>(z1), batch<TYPE ,N>(z0));            \
    }

#define XSIMD_DEFINE_BITWISE_CAST_ALL(NMIN)                                    \
    XSIMD_DEFINE_BITWISE_CAST_FLOAT(double, NMIN)                              \
    XSIMD_DEFINE_BITWISE_CAST_FLOAT(float, NMIN * 2)                           \
    XSIMD_DEFINE_BITWISE_CAST(int64_t, NMIN)                                   \
    XSIMD_DEFINE_BITWISE_CAST(uint64_t, NMIN)                                  \
    XSIMD_DEFINE_BITWISE_CAST(int32_t, NMIN * 2)                               \
    XSIMD_DEFINE_BITWISE_CAST(uint32_t, NMIN * 2)                              \
    XSIMD_DEFINE_BITWISE_CAST(int16_t, NMIN * 4)                               \
    XSIMD_DEFINE_BITWISE_CAST(uint16_t, NMIN * 4)                              \
    XSIMD_DEFINE_BITWISE_CAST(int8_t, NMIN * 8)                                \
    XSIMD_DEFINE_BITWISE_CAST(uint8_t, NMIN * 8)

    /****************************
     * simd_base implementation *
     ****************************/

    /**
     * @name Static downcast functions
     */
    //@{
    /**
     * Returns a reference to the batch type used for computation. 
     */
    template <class X>
    inline auto simd_base<X>::operator()() -> batch_reference
    {
        return derived_cast().get();
    }

    /**
     * Returns a constant reference to the batch type used for computation. 
     */
    template <class X>
    inline auto simd_base<X>::operator()() const -> const_batch_reference
    {
        return derived_cast().get();
    }

    /**
     * Returns a reference to the actual derived type of simd_base.
     */
    template <class X>
    inline X& simd_base<X>::derived_cast()
    {
        return *static_cast<X*>(this);
    }

    /**
     * Returns a constant reference to the actual derived type of simd_base.
     */
    template <class X>
    inline const X& simd_base<X>::derived_cast() const
    {
        return *static_cast<const X*>(this);
    }
    //@}

    /*****************************
     * simd_batch implementation *
     *****************************/

    template <class X>
    constexpr inline simd_batch<X>::simd_batch(storage_type value)
        : m_value(value)
    {
    }

    /**
     * @name Static builders
     */
    //@{
    /**
     * Creates a batch from the single value \c v.
     * @param v the value used to initialize the batch
     * @return a new batch instance
     */
    template <class X>
    inline X simd_batch<X>::broadcast(value_type v)
    {
        return X(v);
    }

    /**
     * Creates a batch from the buffer \c src. The
     * memory does not need to be aligned.
     * @param src the memory buffer to read
     * @return a new batch instance
     */
    template <class X>
    template <class T>
    inline X simd_batch<X>::from_unaligned(T* src)
    {
        X res;
        res.load_unaligned(src);
        return res;
    }

    /**
     * Creates a batch from the buffer \c src. The
     * memory needs to be aligned.
     * @param src the memory buffer to read
     * @return a new batch instance
     */
    template <class X>
    template <class T>
    inline X simd_batch<X>::from_aligned(T* src)
    {
        X res;
        res.load_aligned(src);
        return res;
    }
    //@}
    
    /**
     * @name Arithmetic computed assignment
     */
    //@{
    /**
     * Adds the batch \c rhs to \c this.
     * @param rhs the batch to add.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_batch<X>::operator+=(const X& rhs)
    {
        (*this)() = (*this)() + rhs;
        return (*this)();
    }

    /**
     * Adds the scalar \c rhs to each value contained in \c this.
     * @param rhs the scalar to add.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_batch<X>::operator+=(const value_type& rhs)
    {
        (*this)() = (*this)() + X(rhs);
        return (*this)();
    }

    /**
     * Substracts the batch \c rhs to \c this.
     * @param rhs the batch to substract.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_batch<X>::operator-=(const X& rhs)
    {
        (*this)() = (*this)() - rhs;
        return (*this)();
    }

    /**
     * Substracts the scalar \c rhs to each value contained in \c this.
     * @param rhs the scalar to substract.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_batch<X>::operator-=(const value_type& rhs)
    {
        (*this)() = (*this)() - X(rhs);
        return (*this)();
    }

    /**
     * Multiplies \c this with the batch \c rhs.
     * @param rhs the batch involved in the multiplication.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_batch<X>::operator*=(const X& rhs)
    {
        (*this)() = (*this)() * rhs;
        return (*this)();
    }

    /**
     * Multiplies each scalar contained in \c this with the scalar \c rhs.
     * @param rhs the scalar involved in the multiplication.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_batch<X>::operator*=(const value_type& rhs)
    {
        (*this)() = (*this)() * X(rhs);
        return (*this)();
    }

    /**
     * Divides \c this by the batch \c rhs.
     * @param rhs the batch involved in the division.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_batch<X>::operator/=(const X& rhs)
    {
        (*this)() = (*this)() / rhs;
        return (*this)();
    }

    /**
     * Divides each scalar contained in \c this by the scalar \c rhs.
     * @param rhs the scalar involved in the division.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_batch<X>::operator/=(const value_type& rhs)
    {
        (*this)() = (*this)() / X(rhs);
        return (*this)();
    }
    //@}

    /**
     * @name Bitwise computed assignment
     */
    /**
     * Assigns the bitwise and of \c rhs and \c this.
     * @param rhs the batch involved in the operation.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_batch<X>::operator&=(const X& rhs)
    {
        (*this)() = (*this)() & rhs;
        return (*this)();
    }

    /**
     * Assigns the bitwise or of \c rhs and \c this.
     * @param rhs the batch involved in the operation.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_batch<X>::operator|=(const X& rhs)
    {
        (*this)() = (*this)() | rhs;
        return (*this)();
    }

    /**
     * Assigns the bitwise xor of \c rhs and \c this.
     * @param rhs the batch involved in the operation.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_batch<X>::operator^=(const X& rhs)
    {
        (*this)() = (*this)() ^ rhs;
        return (*this)();
    }
    //@}

    /**
     * @name Increment and decrement operators
     */
    //@{
    /**
     * Pre-increment operator.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_batch<X>::operator++()
    {
        (*this)() += value_type(1);
        return (*this)();
    }

    /**
     * Post-increment operator.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_batch<X>::operator++(int)
    {
        X tmp = (*this)();
        (*this)() += value_type(1);
        return tmp;
    }

    /**
     * Pre-decrement operator.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_batch<X>::operator--()
    {
        (*this)() -= value_type(1);
        return (*this)();
    }

    /**
     * Post-decrement operator.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_batch<X>::operator--(int)
    {
        X tmp = (*this)();
        (*this)() -= value_type(1);
        return tmp;
    }
    //@}

    template <class X>
    inline X& simd_batch<X>::load_aligned(const char* src)
    {
        return (*this)().load_aligned(reinterpret_cast<const char_itype*>(src));
    }

    template <class X>
    inline X& simd_batch<X>::load_unaligned(const char* src)
    {
        return (*this)().load_unaligned(reinterpret_cast<const char_itype*>(src));
    }

    template <class X>
    void simd_batch<X>::store_aligned(char* dst) const
    {
        return (*this)().store_aligned(reinterpret_cast<char_itype*>(dst));
    }

    template <class X>
    void simd_batch<X>::store_unaligned(char* dst) const
    {
        return (*this)().store_unaligned(reinterpret_cast<char_itype*>(dst));
    }

    template <class X>
    inline auto simd_batch<X>::get() -> batch_reference
    {
        return this->derived_cast();
    }

    template <class X>
    inline auto simd_batch<X>::get() const -> const_batch_reference
    {
        return this->derived_cast();
    }
    
    template <class X>
    inline auto simd_batch<X>::operator[](std::size_t index) -> value_type&
    {
        return m_array[index & (size - 1)];
    }

    template <class X>
    inline auto simd_batch<X>::operator[](std::size_t index) const -> const value_type&
    {
        return m_array[index & (size - 1)];
    }

    template <class X>
    inline auto simd_batch<X>::begin() -> iterator
    {
        return m_array;
    }

    template <class X>
    inline auto simd_batch<X>::end() -> iterator
    {
        return m_array + size;
    }

    template <class X>
    inline auto simd_batch<X>::begin() const -> const_iterator
    {
        return cbegin();
    }

    template <class X>
    inline auto simd_batch<X>::end() const -> const_iterator
    {
        return cend();
    }

    template <class X>
    inline auto simd_batch<X>::cbegin() const -> const_iterator
    {
        return m_array;
    }

    template <class X>
    inline auto simd_batch<X>::cend() const -> const_iterator
    {
        return m_array + size;
    }

    template <class X>
    inline auto simd_batch<X>::rbegin() -> reverse_iterator
    {
        return reverse_iterator(end());
    }

    template <class X>
    inline auto simd_batch<X>::rend() -> reverse_iterator
    {
        return reverse_iterator(begin());
    }

    template <class X>
    inline auto simd_batch<X>::rbegin() const -> const_reverse_iterator
    {
        return crbegin();
    }

    template <class X>
    inline auto simd_batch<X>::rend() const -> const_reverse_iterator
    {
        return crend();
    }

    template <class X>
    inline auto simd_batch<X>::crbegin() const -> const_reverse_iterator
    {
        return const_reverse_iterator(end());
    }

    template <class X>
    inline auto simd_batch<X>::crend() const -> const_reverse_iterator
    {
        return const_reverse_iterator(begin());
    }

#define XSIMD_UNARY_OP(OP, FUNC)                                                                   \
    template <class X>                                                                             \
    inline batch_type_t<X> operator OP(const simd_base<X>& rhs)                                    \
    {                                                                                              \
        using value_type = typename simd_batch_traits<X>::value_type;                              \
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;               \
        return kernel::FUNC(rhs());                                                                \
    }

#define XSIMD_BINARY_OP(OP, FUNC)                                                                  \
    template <class X, class Y>                                                                    \
    inline batch_type_t<X> operator OP(const simd_base<X>& lhs, const simd_base<Y>& rhs)           \
    {                                                                                              \
        using value_type = typename simd_batch_traits<X>::value_type;                              \
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;               \
        return kernel::FUNC(lhs(), rhs());                                                         \
    }                                                                                              \
                                                                                                   \
    template <class X>                                                                             \
    inline batch_type_t<X> operator OP(const typename simd_batch_traits<X>::value_type& lhs,       \
                                       const simd_base<X>& rhs)                                    \
    {                                                                                              \
        return batch_type_t<X>(lhs) OP rhs();                                                      \
    }                                                                                              \
                                                                                                   \
    template <class X>                                                                             \
    inline batch_type_t<X> operator OP(const simd_base<X>& lhs,                                    \
                                       const typename simd_batch_traits<X>::value_type& rhs)       \
    {                                                                                              \
        return lhs() OP batch_type_t<X>(rhs);                                                      \
    }

#define XSIMD_BINARY_BOOL_OP(OP, FUNC)                                                             \
    template <class X>                                                                             \
    inline typename simd_batch_traits<X>::batch_bool_type operator OP(const simd_base<X>& lhs,     \
                                                                      const simd_base<X>& rhs)     \
    {                                                                                              \
        using value_type = typename simd_batch_traits<X>::value_type;                              \
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;               \
        return kernel::FUNC(lhs(), rhs());                                                         \
    }                                                                                              \
                                                                                                   \
    template <class X>                                                                             \
    inline typename simd_batch_traits<X>::batch_bool_type operator OP(                             \
        const typename simd_batch_traits<X>::value_type& lhs, const simd_base<X>& rhs)             \
    {                                                                                              \
        return batch_type_t<X>(lhs) OP rhs();                                                      \
    }                                                                                              \
                                                                                                   \
    template <class X>                                                                             \
    inline typename simd_batch_traits<X>::batch_bool_type operator OP(                             \
        const simd_base<X>& lhs, const typename simd_batch_traits<X>::value_type& rhs)             \
    {                                                                                              \
        return lhs() OP batch_type_t<X>(rhs);                                                      \
    }

#define XSIMD_BINARY_BOOL_OP_DERIVED(OP, BASE_OP)                                                  \
    template <class X>                                                                             \
    inline typename simd_batch_traits<X>::batch_bool_type operator OP(const simd_base<X>& lhs,     \
                                                                      const simd_base<X>& rhs)     \
    {                                                                                              \
        return rhs() BASE_OP lhs();                                                                \
    }                                                                                              \
                                                                                                   \
    template <class X>                                                                             \
    inline typename simd_batch_traits<X>::batch_bool_type operator OP(                             \
        const typename simd_batch_traits<X>::value_type& lhs, const simd_base<X>& rhs)             \
    {                                                                                              \
        return rhs() BASE_OP batch_type_t<X>(lhs);                                                 \
    }                                                                                              \
                                                                                                   \
    template <class X>                                                                             \
    inline typename simd_batch_traits<X>::batch_bool_type operator OP(                             \
        const simd_base<X>& lhs, const typename simd_batch_traits<X>::value_type& rhs)             \
    {                                                                                              \
        return batch_type_t<X>(rhs) BASE_OP lhs();                                                 \
    }

    /**
     * @defgroup simd_batch_arithmetic Arithmetic operators
     */

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the opposite of the batch \c rhs.
     * @tparam X the actual type of batch.
     * @param rhs batch involved in the operation.
     * @return the opposite of \c rhs.
     */
    template <class X>
    inline batch_type_t<X> operator-(const simd_base<X>& rhs);

    XSIMD_UNARY_OP(-, neg)

    /**
     * @ingroup simd_batch_arithmetic
     *
     * No-op on \c rhs.
     * @tparam X the actual type of batch.
     * @param rhs batch involved in the operation.
     * @return \c rhs.
     */
    template <class X>
    inline X operator+(const simd_batch<X>& rhs)
    {
        return rhs();
    }

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the sum of the batches \c lhs and \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the addition.
     * @param rhs batch involved in the addition.
     * @return the result of the addition.
     */
    template <class X, class Y>
    batch_type_t<X> operator+(const simd_base<X>& lhs, const simd_base<Y>& rhs);

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the sum of the batch \c lhs and the scalar \c rhs. Equivalent to the
     * sum of two batches where all the values of the second one are initialized to
     * \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the addition.
     * @param rhs scalar involved in the addition.
     * @return the result of the addition.
     */
    template <class X>
    batch_type_t<X> operator+(const simd_base<X>& lhs, const typename simd_batch_traits<X>::value_type& rhs);

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the sum of the scalar \c lhs and the batch \c rhs. Equivalent to the
     * sum of two batches where all the values of the first one are initialized to
     * \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs scalar involved in the addition.
     * @param rhs batch involved in the addition.
     * @return the result of the addition.
     */
    template <class X>
    batch_type_t<X> operator+(const typename simd_batch_traits<X>::value_type& lhs, const simd_base<X>& rhs);

    XSIMD_BINARY_OP(+, add)


    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the saturate sum of the batch \c lhs and the batch \c rhs.
     * \c lhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the saturated addition.
     * @param rhs batch involved in the saturated addition.
     * @return the result of the saturated addition.
     */
    template <class X>
    inline batch_type_t<X> sadd(const simd_base<X>& lhs, const simd_base<X>& rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::sadd(lhs(), rhs());
    }

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the saturate sum of the scalar \c lhs and the batch \c rhs. Equivalent to the
     * saturated sum of two batches where all the values of the first one are initialized to
     * \c lhs.
     * @tparam X the actual type of batch.
     * @param lhs scalar involved in the saturated addition.
     * @param rhs batch involved in the saturated addition.
     * @return the result of the saturated addition.
     */
    template <class X>
    inline batch_type_t<X> sadd(const typename simd_batch_traits<X>::value_type& lhs,
                                       const simd_base<X>& rhs)
    {
        return sadd(batch_type_t<X>(lhs),rhs());
    }

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the saturate sum of the batch \c lhs and the scalar \c rhs. Equivalent to the
     * saturated sum of two batches where all the values of the second one are initialized to
     * \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the saturated addition.
     * @param rhs scalar involved in the saturated addition.
     * @return the result of the saturated addition.
     */
    template <class X>
    inline batch_type_t<X> sadd(const simd_base<X>& lhs,
                                       const typename simd_batch_traits<X>::value_type& rhs)
    {
        return sadd(lhs(),batch_type_t<X>(rhs));
    }

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the difference of the batches \c lhs and \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the difference.
     * @param rhs batch involved in the difference.
     * @return the result of the difference.
     */
    template <class X, class Y>
    batch_type_t<X> operator-(const simd_base<X>& lhs, const simd_base<Y>& rhs);

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the difference of the batch \c lhs and the scalar \c rhs. Equivalent to the
     * difference of two batches where all the values of the second one are initialized to
     * \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the difference.
     * @param rhs scalar involved in the difference.
     * @return the result of the difference.
     */
    template <class X>
    batch_type_t<X> operator-(const simd_base<X>& lhs, const typename simd_batch_traits<X>::value_type& rhs);

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the difference of the scalar \c lhs and the batch \c rhs. Equivalent to the
     * difference of two batches where all the values of the first one are initialized to
     * \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs scalar involved in the difference.
     * @param rhs batch involved in the difference.
     * @return the result of the difference.
     */
    template <class X>
    batch_type_t<X> operator-(const typename simd_batch_traits<X>::value_type& lhs, const simd_base<X>& rhs);

    XSIMD_BINARY_OP(-, sub)

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the saturate difference of the batch \c lhs and the batch \c rhs.
     * \c lhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the saturated difference.
     * @param rhs batch involved in the saturated difference.
     * @return the result of the saturated difference.
     */                 
    template <class X>                                                                    
    inline batch_type_t<X> ssub(const simd_base<X>& lhs, const simd_base<X>& rhs)           
    {                                                                                              
        using value_type = typename simd_batch_traits<X>::value_type;                              
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;               
        return kernel::ssub(lhs(), rhs());                                                         
    }                                                                                              

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the saturate difference of the scalar \c lhs and the batch \c rhs. Equivalent to the
     * saturated sum of two batches where all the values of the first one are initialized to
     * \c lhs.
     * @tparam X the actual type of batch.
     * @param lhs scalar involved in the saturated difference.
     * @param rhs batch involved in the saturated difference.
     * @return the result of the saturated difference.
     */                                                                                               
    template <class X>                                                                             
    inline batch_type_t<X> ssub(const typename simd_batch_traits<X>::value_type& lhs,       
                                       const simd_base<X>& rhs)                                    
    {                                                                                              
        return ssub(batch_type_t<X>(lhs),rhs());                                                      
    }                                                                                              

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the saturate difference of the batch \c lhs and the scalar \c rhs. Equivalent to the
     * saturated difference of two batches where all the values of the second one are initialized to
     * \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the saturated difference.
     * @param rhs scalar involved in the saturated difference.
     * @return the result of the saturated difference.
     */                                                                                                     
    template <class X>                                                                             
    inline batch_type_t<X> ssub(const simd_base<X>& lhs,                                    
                                       const typename simd_batch_traits<X>::value_type& rhs)       
    {                                                                                              
        return ssub(lhs(),batch_type_t<X>(rhs));                                                      
    }

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the product of the batches \c lhs and \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the product.
     * @param rhs batch involved in the product.
     * @return the result of the product.
     */
    template <class X, class Y>
    batch_type_t<X> operator*(const simd_base<X>& lhs, const simd_base<Y>& rhs);

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the product of the batch \c lhs and the scalar \c rhs. Equivalent to the
     * product of two batches where all the values of the second one are initialized to
     * \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the product.
     * @param rhs scalar involved in the product.
     * @return the result of the product.
     */
    template <class X>
    batch_type_t<X> operator*(const simd_base<X>& lhs, const typename simd_batch_traits<X>::value_type& rhs);

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the product of the scalar \c lhs and the batch \c rhs. Equivalent to the
     * difference of two batches where all the values of the first one are initialized to
     * \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs scalar involved in the product.
     * @param rhs batch involved in the product.
     * @return the result of the product.
     */
    template <class X>
    batch_type_t<X> operator*(const typename simd_batch_traits<X>::value_type& lhs, const simd_base<X>& rhs);

    XSIMD_BINARY_OP(*, mul)

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the division of the batch \c lhs by the batch \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the division.
     * @param rhs batch involved in the division.
     * @return the result of the division.
     */
    template <class X, class Y>
    batch_type_t<X> operator/(const simd_base<X>& lhs, const simd_base<Y>& rhs);

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the division of the batch \c lhs by the scalar \c rhs. Equivalent to the
     * division of two batches where all the values of the second one are initialized to
     * \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the division.
     * @param rhs scalar involved in the division.
     * @return the result of the division.
     */
    template <class X>
    batch_type_t<X> operator/(const simd_base<X>& lhs, const typename simd_batch_traits<X>::value_type& rhs);

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the division of the scalar \c lhs and the batch \c rhs. Equivalent to the
     * difference of two batches where all the values of the first one are initialized to
     * \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs scalar involved in the division.
     * @param rhs batch involved in the division.
     * @return the result of the division.
     */
    template <class X>
    batch_type_t<X> operator/(const typename simd_batch_traits<X>::value_type& lhs, const simd_base<X>& rhs);

    XSIMD_BINARY_OP(/, div)

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the integer modulo of the batch \c lhs by the batch \c rhs.
     * @param lhs batch involved in the modulo.
     * @param rhs batch involved in the modulo.
     * @return the result of the modulo.
     */
    template <class X, class Y>
    batch_type_t<X> operator%(const simd_base<X>& lhs, const simd_base<Y>& rhs);

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the integer modulo of the batch \c lhs by the scalar \c rhs. Equivalent to the
     * modulo of two batches where all the values of the second one are initialized to
     * \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the modulo.
     * @param rhs scalar involved in the modulo.
     * @return the result of the modulo.
     */
    template <class X>
    batch_type_t<X> operator%(const simd_base<X>& lhs, const typename simd_batch_traits<X>::value_type& rhs);

    /**
     * @ingroup simd_batch_arithmetic
     *
     * Computes the integer modulo of the scalar \c lhs and the batch \c rhs. Equivalent to the
     * difference of two batches where all the values of the first one are initialized to
     * \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs scalar involved in the modulo.
     * @param rhs batch involved in the modulo.
     * @return the result of the modulo.
     */
    template <class X>
    batch_type_t<X> operator%(const typename simd_batch_traits<X>::value_type& lhs, const simd_base<X>& rhs);

    XSIMD_BINARY_OP(%, mod)

    /**
     * @defgroup simd_batch_comparison Comparison operators
     */

     /**
      * @ingroup simd_batch_comparison
      *
      * Element-wise equality comparison of batches \c lhs and \c rhs.
      * @param lhs batch involved in the comparison.
      * @param rhs batch involved in the comparison.
      * @return a boolean batch.
      */
    template <class X>
    typename simd_batch_traits<X>::batch_bool_type
    operator==(const simd_base<X>& lhs, const simd_base<X>& rhs);

    XSIMD_BINARY_BOOL_OP(==, eq)

    /**
     * @ingroup simd_batch_comparison
     *
     * Element-wise inequality comparison of batches \c lhs and \c rhs.
     * @param lhs batch involved in the comparison.
     * @param rhs batch involved in the comparison.
     * @return a boolean batch.
     */
    template <class X>
    typename simd_batch_traits<X>::batch_bool_type
    operator!=(const simd_base<X>& lhs, const simd_base<X>& rhs);

    XSIMD_BINARY_BOOL_OP(!=, neq)

    /**
     * @ingroup simd_batch_comparison
     *
     * Element-wise lesser than comparison of batches \c lhs and \c rhs.
     * @param lhs batch involved in the comparison.
     * @param rhs batch involved in the comparison.
     * @return a boolean batch.
     */
    template <class X>
    typename simd_batch_traits<X>::batch_bool_type
    operator<(const simd_base<X>& lhs, const simd_base<X>& rhs);

    XSIMD_BINARY_BOOL_OP(<, lt)

    /**
     * @ingroup simd_batch_comparison
     *
     * Element-wise lesser or equal to comparison of batches \c lhs and \c rhs.
     * @param lhs batch involved in the comparison.
     * @param rhs batch involved in the comparison.
     * @return a boolean batch.
     */
    template <class X>
    typename simd_batch_traits<X>::batch_bool_type
    operator<=(const simd_base<X>& lhs, const simd_base<X>& rhs);

    XSIMD_BINARY_BOOL_OP(<=, lte)

    /**
     * @ingroup simd_batch_comparison
     *
     * Element-wise greater than comparison of batches \c lhs and \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the comparison.
     * @param rhs batch involved in the comparison.
     * @return a boolean batch.
     */
    template <class X>
    typename simd_batch_traits<X>::batch_bool_type
    operator>(const simd_base<X>& lhs, const simd_base<X>& rhs);

    XSIMD_BINARY_BOOL_OP_DERIVED(>, <)

    /**
     * @ingroup simd_batch_comparison
     *
     * Element-wise greater or equal comparison of batches \c lhs and \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the comparison.
     * @param rhs batch involved in the comparison.
     * @return a boolean batch.
     */
    template <class X>
    typename simd_batch_traits<X>::batch_bool_type
    operator>=(const simd_base<X>& lhs, const simd_base<X>& rhs);

    XSIMD_BINARY_BOOL_OP_DERIVED(>=, <=)

    /**
     * @defgroup simd_batch_bitwise Bitwise operators
     */

    /**
     * @ingroup simd_batch_bitwise
     *
     * Computes the bitwise and of the batches \c lhs and \c rhs.
     * @param lhs batch involved in the operation.
     * @param rhs batch involved in the operation.
     * @return the result of the bitwise and.
     */
    template <class X, class Y>
    inline batch_type_t<X> operator&(const simd_base<X>& lhs, const simd_base<Y>& rhs);

    XSIMD_BINARY_OP(&, bitwise_and)

    /**
     * @ingroup simd_batch_bitwise
     *
     * Computes the bitwise or of the batches \c lhs and \c rhs.
     * @param lhs batch involved in the operation.
     * @param rhs batch involved in the operation.
     * @return the result of the bitwise or.
     */
    template <class X, class Y>
    inline batch_type_t<X> operator|(const simd_base<X>& lhs, const simd_base<Y>& rhs);

    XSIMD_BINARY_OP(|, bitwise_or)

    /**
     * @ingroup simd_batch_bitwise
     *
     * Computes the bitwise xor of the batches \c lhs and \c rhs.
     * @param lhs batch involved in the operation.
     * @param rhs batch involved in the operation.
     * @return the result of the bitwise xor.
     */
    template <class X, class Y>
    inline batch_type_t<X> operator^(const simd_base<X>& lhs, const simd_base<Y>& rhs);

    XSIMD_BINARY_OP(^, bitwise_xor)

    /**
     * @ingroup simd_batch_bitwise
     *
     * Computes the bitwise not of the batches \c lhs and \c rhs.
     * @param rhs batch involved in the operation.
     * @return the result of the bitwise not.
     */
    template <class X>
    batch_type_t<X> operator~(const simd_base<X>& rhs);

    XSIMD_UNARY_OP(~, bitwise_not)

    /**
     * @ingroup simd_batch_bitwise
     *
     * Computes the bitwise andnot of the batches \c lhs and \c rhs.
     * @param lhs batch involved in the operation.
     * @param rhs batch involved in the operation.
     * @return the result of the bitwise andnot.
     */
    template <class X>
    inline batch_type_t<X> bitwise_andnot(const simd_batch<X>& lhs, const simd_batch<X>& rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;                              \
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;               \
        return kernel::bitwise_andnot(lhs(), rhs());      
    }

    /**
     * Element-wise not of \c rhs.
     * @tparam X the actual type of batch.
     * @param rhs batch involved in the logical not operation.
     * @return boolean batch.
     */
    template <class X>
    inline typename simd_batch_traits<X>::batch_bool_type
    operator!(const simd_base<X>& rhs)
    {
        using b_type = typename X::batch_type;
        using value_type = typename simd_batch_traits<b_type>::value_type;
        return rhs() == b_type(value_type(0));
    }

    /**
     * Returns the smaller values of the batches \c lhs and \c rhs.
     * @param lhs a batch of integer or floating point values.
     * @param rhs a batch of integer or floating point values.
     * @return a batch of the smaller values.
     */
    template <class X>
    inline batch_type_t<X> min(const simd_base<X>& lhs, const simd_base<X>& rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::min(lhs(), rhs());
    }

    /**
     * Returns the larger values of the batches \c lhs and \c rhs.
     * @param lhs a batch of integer or floating point values.
     * @param rhs a batch of integer or floating point values.
     * @return a batch of the larger values.
     */
    template <class X>
    inline batch_type_t<X> max(const simd_base<X>& lhs, const simd_base<X>& rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::max(lhs(), rhs());
    }

    /**
     * Returns the smaller values of the batches \c lhs and \c rhs.
     * @param lhs a batch of floating point values.
     * @param rhs a batch of floating point values.
     * @return a batch of the smaller values.
     */
    template <class X>
    inline batch_type_t<X> fmin(const simd_batch<X>& lhs, const simd_batch<X>& rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::fmin(lhs(), rhs());
    }

    /**
     * Returns the larger values of the batches \c lhs and \c rhs.
     * @param lhs a batch of floating point values.
     * @param rhs a batch of floating point values.
     * @return a batch of the larger values.
     */
    template <class X>
    inline batch_type_t<X> fmax(const simd_batch<X>& lhs, const simd_batch<X>& rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::fmax(lhs(), rhs());
    }

    /**
     * Computes the absolute values of each scalar in the batch \c rhs.
     * @param rhs batch of integer or floating point values.
     * @return the asbolute values of \c rhs.
     */
    template <class X>
    inline real_batch_type_t<X> abs(const simd_base<X>& rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::abs(rhs());
    }

    /**
     * Computes the absolute values of each scalar in the batch \c rhs.
     * @param rhs batch floating point values.
     * @return the asbolute values of \c rhs.
     */
    template <class X>
    inline batch_type_t<X> fabs(const simd_base<X>& rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::fabs(rhs());
    }

    /**
     * Computes the square root of the batch \c rhs.
     * @param rhs batch of floating point values.
     * @return the square root of \c rhs.
     */
    template <class X>
    inline batch_type_t<X> sqrt(const simd_base<X>& rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::sqrt(rhs());
    }

    /**
     * Computes <tt>(x*y) + z</tt> in a single instruction when possible.
     * @param x a batch of integer or floating point values.
     * @param y a batch of integer or floating point values.
     * @param z a batch of integer or floating point values.
     * @return the result of the fused multiply-add operation.
     */
    template <class X>
    inline batch_type_t<X> fma(const simd_base<X>& x, const simd_base<X>& y, const simd_base<X>& z)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::fma(x(), y(), z());
    }

    /**
     * Computes <tt>(x*y) - z</tt> in a single instruction when possible.
     * @param x a batch of integer or floating point values.
     * @param y a batch of integer or floating point values.
     * @param z a batch of integer or floating point values.
     * @return the result of the fused multiply-sub operation.
     */
    template <class X>
    inline batch_type_t<X> fms(const simd_base<X>& x, const simd_base<X>& y, const simd_base<X>& z)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::fms(x(), y(), z());
    }

    /**
     * Computes <tt>-(x*y) + z</tt> in a single instruction when possible.
     * @param x a batch of integer or floating point values.
     * @param y a batch of integer or floating point values.
     * @param z a batch of integer or floating point values.
     * @return the result of the fused negated multiply-add operation.
     */
    template <class X>
    inline batch_type_t<X> fnma(const simd_base<X>& x, const simd_base<X>& y, const simd_base<X>& z)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::fnma(x(), y(), z());
    }

    /**
     * Computes <tt>-(x*y) - z</tt> in a single instruction when possible.
     * @param x a batch of integer or floating point values.
     * @param y a batch of integer or floating point values.
     * @param z a batch of integer or floating point values.
     * @return the result of the fused negated multiply-sub operation.
     */
    template <class X>
    inline batch_type_t<X> fnms(const simd_base<X>& x, const simd_base<X>& y, const simd_base<X>& z)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::fnms(x(), y(), z());
    }

    /**
     * @defgroup simd_batch_reducers Reducers
     */

    /**
     * @ingroup simd_batch_reducers
     *
     * Adds all the scalars of the batch \c rhs.
     * @param rhs batch involved in the reduction
     * @return the result of the reduction.
     */
    template <class X>
    inline typename simd_batch_traits<X>::value_type
    hadd(const simd_base<X>& rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::hadd(rhs());
    }

    /**
     * @ingroup simd_batch_reducers
     *
     * Parallel horizontal addition: adds the scalars of each batch
     * in the array pointed by \c row and store them in a returned
     * batch.
     * @param row an array of \c N batches
     * @return the result of the reduction.
     */
    template <class X>
    enable_if_simd_t<X> haddp(const X* row)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::haddp(row);
    }

    /**
     * @defgroup simd_batch_miscellaneous Miscellaneous
     */

    /**
     * @ingroup simd_batch_miscellaneous
     *
     * Ternary operator for batches: selects values from the batches \c a or \c b
     * depending on the boolean values in \c cond. Equivalent to
     * \code{.cpp}
     * for(std::size_t i = 0; i < N; ++i)
     *     res[i] = cond[i] ? a[i] : b[i];
     * \endcode
     * @param cond batch condition.
     * @param a batch values for truthy condition.
     * @param b batch value for falsy condition.
     * @return the result of the selection.
     */
    template <class X>
    inline batch_type_t<X> select(const typename simd_batch_traits<X>::batch_bool_type& cond, const simd_base<X>& a, const simd_base<X>& b)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::select(cond(), a(), b());
    }

    /**
     * @ingroup simd_batch_miscellaneous
     *
     * Ternary operator for batches: selects values from the batches \c a or \c b
     * depending on the boolean values in the constant batch \c cond. Equivalent to
     * \code{.cpp}
     * for(std::size_t i = 0; i < N; ++i)
     *     res[i] = cond[i] ? a[i] : b[i];
     * \endcode
     * @param cond constant batch condition.
     * @param a batch values for truthy condition.
     * @param b batch value for falsy condition.
     * @return the result of the selection.
     */
    template <class X, bool... Masks>
    inline batch_type_t<X> select(const batch_bool_constant<typename simd_batch_traits<X>::value_type, Masks...>& cond, const simd_base<X>& a, const simd_base<X>& b)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::select(cond, a(), b());
    }

    /**
     * Unpack and interleave data from the LOW half of batches \c lhs and \c rhs.
     * Store the results in the Return value.
     * @param lhs a batch of integer or floating point or double precision values.
     * @param rhs a batch of integer or floating point or double precision values.
     * @return a batch of the low part of shuffled values.
     */
    template <class X>
    inline batch_type_t<X> zip_lo(const simd_base<X>& lhs, const simd_base<X>& rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::zip_lo(lhs(), rhs());
    }

    /**
     * Unpack and interleave data from the HIGH half of batches \c lhs and \c rhs.
     * Store the results in the Return value.
     * @param lhs a batch of integer or floating point or double precision values.
     * @param rhs a batch of integer or floating point or double precision values.
     * @return a batch of the high part of shuffled values.
     */
    template <class X>
    inline batch_type_t<X> zip_hi(const simd_base<X>& lhs, const simd_base<X>& rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::zip_hi(lhs(), rhs());
    }

    /**
     * Determines if the scalars in the given batch \c x are NaN values.
     * @param x batch of floating point values.
     * @return a batch of booleans.
     */
    template <class X>
    inline typename simd_batch_traits<X>::batch_bool_type
    isnan(const simd_base<X>& x)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::isnan(x());
    }

    /**
     * Insert the batch \c rhs into the stream \c out.
     * @tparam X the actual type of batch.
     * @param out the output stream.
     * @param rhs the batch to output.
     * @return the output stream.
     */
    template <class X>
    inline std::ostream& operator<<(std::ostream& out, const simd_batch<X>& rhs)
    {
        out << '(';
        std::size_t s = simd_batch<X>::size;
        for (std::size_t i = 0; i < s - 1; ++i)
        {
            out << rhs()[i] << ", ";
        }
        out << rhs()[s - 1] << ')';
        return out;
    }

    /******************************************
     * generic batch operators implementation *
     ******************************************/

#define GENERIC_OPERATOR_IMPLEMENTATION(OP)        \
    using traits = simd_batch_traits<batch<T, N>>; \
    constexpr std::size_t align = traits::align;   \
    alignas(align) T tmp_lhs[N];                   \
    alignas(align) T tmp_rhs[N];                   \
    alignas(align) T tmp_res[N];                   \
    lhs.store_aligned(tmp_lhs);                    \
    rhs.store_aligned(tmp_rhs);                    \
    for (std::size_t i = 0; i < traits::size; ++i) \
    {                                              \
        tmp_res[i] = tmp_lhs[i] OP tmp_rhs[i];     \
    }                                              \
    return batch<T, N>(tmp_res, aligned_mode())

    template <class T, std::size_t N>
    inline batch<T, N> operator&&(const batch<T, N>& lhs, const batch<T, N>& rhs)
    {
        GENERIC_OPERATOR_IMPLEMENTATION(&&);
    }

    template <class T, std::size_t N>
    inline batch<T, N> operator||(const batch<T, N>& lhs, const batch<T, N>& rhs)
    {
        GENERIC_OPERATOR_IMPLEMENTATION(||);
    }

    template <class T, std::size_t N>
    inline batch<T, N> operator<<(const batch<T, N>& lhs, const batch<T, N>& rhs)
    {
        GENERIC_OPERATOR_IMPLEMENTATION(<<);
    }

    template <class T, std::size_t N>
    inline batch<T, N> operator>>(const batch<T, N>& lhs, const batch<T, N>& rhs)
    {
        GENERIC_OPERATOR_IMPLEMENTATION(>>);
    }

    /*****************************************
     * batch cast functions implementation *
     *****************************************/

    template <class T_out, class T_in, std::size_t N>
    inline batch<T_out, N> batch_cast(const batch<T_in, N>& x)
    {
        return batch_cast_impl<T_in, T_out, N>::run(x);
    }

    /*****************************************
     * bitwise cast functions implementation *
     *****************************************/

    template <class B, std::size_t N>
    inline B bitwise_cast(const batch<float, N>& x)
    {
        return bitwise_cast_impl<batch<float, N>, B>::run(x);
    }

    template <class B, std::size_t N>
    inline B bitwise_cast(const batch<double, N>& x)
    {
        return bitwise_cast_impl<batch<double, N>, B>::run(x);
    }

    template <class B, std::size_t N>
    inline B bitwise_cast(const batch<int32_t, N>& x)
    {
        return bitwise_cast_impl<batch<int32_t, N>, B>::run(x);
    }

    template <class B, std::size_t N>
    inline B bitwise_cast(const batch<int64_t, N>& x)
    {
        return bitwise_cast_impl<batch<int64_t, N>, B>::run(x);
    }

    template <class T, std::size_t N>
    inline batch<T, N> bitwise_cast(const batch_bool<T, N>& src)
    {
        return batch<T, N>(src.get_value());
    }


    /***********************************
     * Workaround for Clang on Windows *
     ***********************************/

#if defined(_WIN32) && defined(__clang__)
    /**
     * On Windows, the return type of fma is the promote type of its
     * arguments if they are integral or floating point types, float
     * otherwise. The implementation does not rely on SFINAE to
     * remove it from the overload resolution set when the argument
     * types are neither integral types nor floating point type.
     *
     * The fma overload defined xsimd accepts simd_base<batch<T, N>>
     * arguments, not batch<T, N>. Thus a call to this latter is not
     * more specialized than a call to the STL overload, which is
     * considered. Since there is no mean to convert batch<double, 2>
     * to float for instance, this results in a compilation error.
     */

    template <class T, std::size_t N>
    inline batch<T, N> fma(const batch<T, N>& a, const batch<T, N>& b, const batch<T, N>& c)
    {
        using base_type = simd_base<batch<T, N>>;
        const base_type& sba = a;
        const base_type& sbb = b;
        const base_type& sbc = c;
        return fma(sba, sbb, sbc);
    }
#endif
}

#endif
