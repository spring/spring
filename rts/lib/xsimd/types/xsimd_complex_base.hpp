/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_COMPLEX_BASE_HPP
#define XSIMD_COMPLEX_BASE_HPP

#include <complex>
#include <cstddef>
#include <limits>
#include <ostream>
#include <cassert>

#ifdef XSIMD_ENABLE_XTL_COMPLEX
#include "xtl/xcomplex.hpp"
#endif

#include "xsimd_base.hpp"
#include "xsimd_utils.hpp"

namespace xsimd
{
    /*****************************
     * complex_batch_bool_traits *
     *****************************/

    template <class C, class R, std::size_t N, std::size_t Align>
    struct complex_batch_bool_traits
    {
        using value_type = C;
        static constexpr std::size_t size = N;
        using batch_type = batch<C, N>;
        static constexpr std::size_t align = Align;
        using real_batch = batch_bool<R, N>;
    };

    /***************************
     * simd_complex_batch_bool *
     ***************************/

     /**
      * @class simd_complex_batch_bool
      * @brief Base class for complex batch of boolean values.
      *
      * The simd_complex_batch_bool class is the base class for all classes representing
      * a complex batch of boolean values. Complex batch of boolean values is meant for operations
      * that may involve batches of complex numbers. Thus, the boolean values are stored as floating
      * point values, and each type of batch of complex has its dedicated type of boolean batch.
      *
      * @tparam X The derived type
      * @sa simd_complex_batch
      */
    template <class X>
    class simd_complex_batch_bool : public simd_batch_bool<X>
    {
    public:

        using value_type = typename simd_batch_traits<X>::value_type;
        static constexpr std::size_t size = simd_batch_traits<X>::size;
        using real_batch = typename simd_batch_traits<X>::real_batch;

        simd_complex_batch_bool() = default;
        simd_complex_batch_bool(bool b);
        simd_complex_batch_bool(const real_batch& b);

        const real_batch& value() const;

        bool operator[](std::size_t index) const;

    private:

        real_batch m_value;
    };

    /************************
     * complex_batch_traits *
     ************************/

    template <class C, class R, std::size_t N, std::size_t Align>
    struct complex_batch_traits
    {
        using value_type = C;
        static constexpr std::size_t size = N;
        using batch_bool_type = batch_bool<C, N>;
        static constexpr std::size_t align = Align;
        using real_batch = batch<R, N>;
    };

    /**********************
     * simd_complex_batch *
     **********************/

    template <class T>
    struct is_ieee_compliant;
    
    template <class T>
    struct is_ieee_compliant<std::complex<T>>
        : std::integral_constant<bool, std::numeric_limits<std::complex<T>>::is_iec559>
    {
    };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
    template <class T>
    struct is_ieee_compliant<xtl::xcomplex<T, T, false>> : std::false_type
    {
    };
#endif

    /**
     * @class simd_complex_batch
     * @brief Base class for batch complex numbers.
     *
     * The simd_complex_batch class is the base class for all classes representing
     * a batch of complex numbers. Each type of batch (i.e. a class inheriting from
     * simd_complex_batch) has its dedicated type of boolean batch (i.e. a class
     * inheriting from simd_complex_batch_bool) for logical operations.
     *
     * Internally, a batch of complex numbers holds two batches of real numbers, one
     * for the real part and one for the imaginary part.
     *
     * @tparam X The derived type
     * @sa simd_complex_batch_bool
     */
    template <class X>
    class simd_complex_batch : public simd_base<X>
    {
    public:

        using base_type = simd_base<X>;
        using batch_reference = typename base_type::batch_reference;
        using const_batch_reference = typename base_type::const_batch_reference;
        using batch_type = X;
        using value_type = typename simd_batch_traits<X>::value_type;
        static constexpr std::size_t size = simd_batch_traits<X>::size;
        using real_batch = typename simd_batch_traits<X>::real_batch;
        using real_value_type = typename value_type::value_type;

        simd_complex_batch() = default;
        explicit simd_complex_batch(const value_type& v);
        explicit simd_complex_batch(const real_value_type& v);
        explicit simd_complex_batch(const real_batch& re);
        explicit simd_complex_batch(const real_value_type* v) : simd_complex_batch(real_batch(v)) {}
        simd_complex_batch(const real_batch& re, const real_batch& im);
        simd_complex_batch(const real_value_type* re, const real_value_type* im) : simd_complex_batch(real_batch(re), real_batch(im)) {}

        real_batch& real();
        real_batch& imag();
        
        const real_batch& real() const;
        const real_batch& imag() const;

        X& operator+=(const X& rhs);
        X& operator+=(const value_type& rhs);
        X& operator+=(const real_batch& rhs);
        X& operator+=(const real_value_type& rhs);

        X& operator-=(const X& rhs);
        X& operator-=(const value_type& rhs);
        X& operator-=(const real_batch& rhs);
        X& operator-=(const real_value_type& rhs);

        X& operator*=(const X& rhs);
        X& operator*=(const value_type& rhs);
        X& operator*=(const real_batch& rhs);
        X& operator*=(const real_value_type& rhs);

        X& operator/=(const X& rhs);
        X& operator/=(const value_type& rhs);
        X& operator/=(const real_batch& rhs);
        X& operator/=(const real_value_type& rhs);

        template <class T>
        X& load_aligned(const T* real_src, const T* imag_src);
        template <class T>
        X& load_unaligned(const T* real_src, const T* imag_src);

        template <class T>
        void store_aligned(T* real_dst, T* imag_dst) const;
        template <class T>
        void store_unaligned(T* real_dst, T* imag_dst) const;

        template <class T>
        typename std::enable_if<detail::is_complex<T>::value, X&>::type
        load_aligned(const T* src);
        template <class T>
        typename std::enable_if<detail::is_complex<T>::value, X&>::type
        load_unaligned(const T* src);

        template <class T>
        typename std::enable_if<!detail::is_complex<T>::value, X&>::type
        load_aligned(const T* src);
        template <class T>
        typename std::enable_if<!detail::is_complex<T>::value, X&>::type
        load_unaligned(const T* src);

        template <class T>
        inline typename std::enable_if<detail::is_complex<T>::value, void>::type
        store_aligned(T* dst) const;
        template <class T>
        inline typename std::enable_if<detail::is_complex<T>::value, void>::type
        store_unaligned(T* dst) const;

        template <class T>
        inline typename std::enable_if<!detail::is_complex<T>::value, void>::type
        store_aligned(T* dst) const;
        template <class T>
        inline typename std::enable_if<!detail::is_complex<T>::value, void>::type
        store_unaligned(T* dst) const;

        value_type operator[](std::size_t index) const;

        batch_reference get();
        const_batch_reference get() const;

    protected:

        real_batch m_real;
        real_batch m_imag;
    };

    template <class X>
    X operator+(const simd_complex_batch<X>& rhs);

    template <class X>
    X operator-(const simd_complex_batch<X>& rhs);

    template <class X>
    X operator+(const simd_complex_batch<X>& lhs, const simd_complex_batch<X>& rhs);
    template <class X>
    X operator+(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::value_type& rhs);
    template <class X>
    X operator+(const typename simd_batch_traits<X>::value_type& lhs, const simd_complex_batch<X>& rhs);
    template <class X>
    X operator+(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::real_batch& rhs);
    template <class X>
    X operator+(const typename simd_batch_traits<X>::real_batch& lhs, const simd_complex_batch<X>& rhs);
    template <class X>
    X operator+(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::real_value_type& rhs);
    template <class X>
    X operator+(const typename simd_batch_traits<X>::real_value_type& lhs, const simd_complex_batch<X>& rhs);

    template <class X>
    X operator-(const simd_complex_batch<X>& lhs, const simd_complex_batch<X>& rhs);
    template <class X>
    X operator-(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::value_type& rhs);
    template <class X>
    X operator-(const typename simd_batch_traits<X>::value_type& lhs, const simd_complex_batch<X>& rhs);
    template <class X>
    X operator-(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::real_batch& rhs);
    template <class X>
    X operator-(const typename simd_batch_traits<X>::real_batch& lhs, const simd_complex_batch<X>& rhs);
    template <class X>
    X operator-(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::real_value_type& rhs);
    template <class X>
    X operator-(const typename simd_batch_traits<X>::real_value_type& lhs, const simd_complex_batch<X>& rhs);

    template <class X>
    X operator*(const simd_complex_batch<X>& lhs, const simd_complex_batch<X>& rhs);
    template <class X>
    X operator*(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::value_type& rhs);
    template <class X>
    X operator*(const typename simd_batch_traits<X>::value_type& lhs, const simd_complex_batch<X>& rhs);
    template <class X>
    X operator*(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::real_batch& rhs);
    template <class X>
    X operator*(const typename simd_batch_traits<X>::real_batch& lhs, const simd_complex_batch<X>& rhs);
    template <class X>
    X operator*(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::real_value_type& rhs);
    template <class X>
    X operator*(const typename simd_batch_traits<X>::real_value_type& lhs, const simd_complex_batch<X>& rhs);

    template <class X>
    X operator/(const simd_complex_batch<X>& lhs, const simd_complex_batch<X>& rhs);
    template <class X>
    X operator/(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::value_type& rhs);
    template <class X>
    X operator/(const typename simd_batch_traits<X>::value_type& lhs, const simd_complex_batch<X>& rhs);
    template <class X>
    X operator/(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::real_batch& rhs);
    template <class X>
    X operator/(const typename simd_batch_traits<X>::real_batch& lhs, const simd_complex_batch<X>& rhs);
    template <class X>
    X operator/(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::real_value_type& rhs);
    template <class X>
    X operator/(const typename simd_batch_traits<X>::real_value_type& lhs, const simd_complex_batch<X>& rhs);

    template <class X>
    typename simd_batch_traits<X>::value_type
    hadd(const simd_complex_batch<X>& rhs);

    template <class X>
    X select(const typename simd_batch_traits<X>::batch_bool_type& cond,
             const simd_complex_batch<X>& a,
             const simd_complex_batch<X>& b);

    template <class X>
    typename simd_batch_traits<X>::batch_bool_type
    operator==(const simd_complex_batch<X>& lhs, const simd_complex_batch<X>& rhs);

    template <class X>
    typename simd_batch_traits<X>::batch_bool_type
    operator!=(const simd_complex_batch<X>& lhs, const simd_complex_batch<X>& rhs);

    template <class X>
    std::ostream& operator<<(std::ostream& out, const simd_complex_batch<X>& rhs);

    /*******************************************
     * xsimd_complex_batch_bool implementation *
     *******************************************/

    /**
     * Initializes all the values of the batch to \c b 
     */
    template <class X>
    inline simd_complex_batch_bool<X>::simd_complex_batch_bool(bool b)
        : m_value(b)
    {
    }

    /**
     * Initializes the values of the batch with those of the real batch \c b.
     * A real batch contains scalars whose type is the \c value_type of
     * the complex number type.
     */
    template <class X>
    inline simd_complex_batch_bool<X>::simd_complex_batch_bool(const real_batch& b)
        : m_value(b)
    {
    }

    template <class X>
    inline auto simd_complex_batch_bool<X>::value() const -> const real_batch&
    {
        return m_value;
    }

    template <class X>
    inline bool simd_complex_batch_bool<X>::operator[](std::size_t index) const
    {
        return m_value[index];
    }

    namespace detail
    {
        template <class T, std::size_t N>
        struct batch_bool_complex_kernel
        {
            using batch_type = batch_bool<T, N>;

            static batch_type bitwise_and(const batch_type& lhs, const batch_type& rhs)
            {
                return lhs.value() & rhs.value();
            }

            static batch_type bitwise_or(const batch_type& lhs, const batch_type& rhs)
            {
                return lhs.value() | rhs.value();
            }

            static batch_type bitwise_xor(const batch_type& lhs, const batch_type& rhs)
            {
                return lhs.value() ^ rhs.value();
            }

            static batch_type bitwise_not(const batch_type& rhs)
            {
                return ~(rhs.value());
            }

            static batch_type bitwise_andnot(const batch_type& lhs, const batch_type& rhs)
            {
                return xsimd::bitwise_andnot(lhs.value(), rhs.value());
            }

            static batch_type equal(const batch_type& lhs, const batch_type& rhs)
            {
                return lhs.value() == rhs.value();
            }

            static batch_type not_equal(const batch_type& lhs, const batch_type& rhs)
            {
                return lhs.value() != rhs.value();
            }

            static bool all(const batch_type& rhs)
            {
                return xsimd::all(rhs.value());
            }

            static bool any(const batch_type& rhs)
            {
                return xsimd::any(rhs.value());
            }
        };

        template <class T, std::size_t N>
        struct batch_bool_kernel<std::complex<T>, N>
            : batch_bool_complex_kernel<std::complex<T>, N>
        {
        };

#ifdef XSIMD_ENABLE_XTL_COMPLEX
        template <class T, std::size_t N, bool ieee_compliant>
        struct batch_bool_kernel<xtl::xcomplex<T, T, ieee_compliant>, N>
            : batch_bool_complex_kernel<xtl::xcomplex<T, T, ieee_compliant>, N>
        {
        };
#endif
    }

    /**************************************
     * xsimd_complex_batch implementation *
     **************************************/

    /**
     * Initializes all the values of the batch to the complex value \c v.
     */
    template <class X>
    inline simd_complex_batch<X>::simd_complex_batch(const value_type& v)
        : m_real(v.real()), m_imag(v.imag())
    {
    }

    /**
     * Initializes all the values of the batch to the real value \c v.
     */
    template <class X>
    inline simd_complex_batch<X>::simd_complex_batch(const real_value_type& v)
        : m_real(v), m_imag(real_value_type(0))
    {
    }

    /**
     * Initializes the values of the batch whith those of the real batch \c re.
     * Imaginary parts are set to 0.
     */
    template <class X>
    inline simd_complex_batch<X>::simd_complex_batch(const real_batch& re)
        : m_real(re), m_imag(real_value_type(0))
    {
    }

    /**
     * Initializes the batch with two real batch, one for the real part and one for the inamginary
     * part.
     */
    template <class X>
    inline simd_complex_batch<X>::simd_complex_batch(const real_batch& re, const real_batch& im)
        : m_real(re), m_imag(im)
    {
    }

    /**
     * Returns a batch for the real part.
     */
    template <class X>
    inline auto simd_complex_batch<X>::real() -> real_batch&
    {
        return m_real;
    }

    /**
     * Returns a batch for the imaginary part.
     */
    template <class X>
    inline auto simd_complex_batch<X>::imag() -> real_batch&
    {
        return m_imag;
    }

    /**
     * Returns a const batch for the real part
     */
    template <class X>
    inline auto simd_complex_batch<X>::real() const -> const real_batch&
    {
        return m_real;
    }

    /**
     * Returns a const batch for the imaginary part.
     */
    template <class X>
    inline auto simd_complex_batch<X>::imag() const -> const real_batch&
    {
        return m_imag;
    }

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
    inline X& simd_complex_batch<X>::operator+=(const X& rhs)
    {
        m_real += rhs.real();
        m_imag += rhs.imag();
        return (*this)();
    }

    /**
     * Adds the scalar \c rhs to each value contained in \c this.
     * @param rhs the scalar to add.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_complex_batch<X>::operator+=(const value_type& rhs)
    {
        return (*this)() += X(rhs);
    }

    /**
     * Adds the real batch \c rhs to \c this.
     * @param rhs the real batch to add.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_complex_batch<X>::operator+=(const real_batch& rhs)
    {
        m_real += rhs;
        return (*this)();
    }

    /**
     * Adds the real scalar \c rhs to each value contained in \c this.
     * @param rhs the real scalar to add.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_complex_batch<X>::operator+=(const real_value_type& rhs)
    {
        return (*this)() += real_batch(rhs);
    }

    /**
     * Substracts the batch \c rhs to \c this.
     * @param rhs the batch to substract.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_complex_batch<X>::operator-=(const X& rhs)
    {
        m_real -= rhs.real();
        m_imag -= rhs.imag();
        return (*this)();
    }

    /**
     * Substracts the scalar \c rhs to each value contained in \c this.
     * @param rhs the scalar to substract.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_complex_batch<X>::operator-=(const value_type& rhs)
    {
        return (*this)() -= X(rhs);
    }

    /**
     * Substracts the real batch \c rhs to \c this.
     * @param rhs the batch to substract.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_complex_batch<X>::operator-=(const real_batch& rhs)
    {
        m_real -= rhs;
        return (*this)();
    }

    /**
     * Substracts the real scalar \c rhs to each value contained in \c this.
     * @param rhs the real scalar to substract.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_complex_batch<X>::operator-=(const real_value_type& rhs)
    {
        return (*this)() -= real_batch(rhs);
    }

    namespace detail
    {
        template <class X, bool ieee_compliant>
        struct complex_batch_multiplier
        {
            using real_batch = typename simd_batch_traits<X>::real_batch;

            inline static X mul(const X& lhs, const X& rhs)
            {
                real_batch a = lhs.real();
                real_batch b = lhs.imag();
                real_batch c = rhs.real();
                real_batch d = rhs.imag();
                return X(a*c - b*d, a*d + b*c);
            }

            inline static X div(const X& lhs, const X& rhs)
            {
                real_batch a = lhs.real();
                real_batch b = lhs.imag();
                real_batch c = rhs.real();
                real_batch d = rhs.imag();
                real_batch e = c*c + d*d;
                return X((c*a + d*b) / e, (c*b - d*a) / e);
            }
        };
    }

    /**
     * Multiplies \c this with the batch \c rhs.
     * @param rhs the batch involved in the multiplication.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_complex_batch<X>::operator*=(const X& rhs)
    {
        using kernel = detail::complex_batch_multiplier<X, is_ieee_compliant<value_type>::value>;
        (*this)() = kernel::mul((*this)(), rhs);
        return (*this)();
    }

    /**
     * Multiplies each scalar contained in \c this with the scalar \c rhs.
     * @param rhs the scalar involved in the multiplication.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_complex_batch<X>::operator*=(const value_type& rhs)
    {
        return (*this)() *= X(rhs);
    }

    /**
     * Multiplies \c this with the real batch \c rhs.
     * @param rhs the real batch involved in the multiplication.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_complex_batch<X>::operator*=(const real_batch& rhs)
    {
        m_real *= rhs;
        m_imag *= rhs;
        return (*this)();
    }

    /**
     * Multiplies each scalar contained in \c this with the real scalar \c rhs.
     * @param rhs the real scalar involved in the multiplication.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_complex_batch<X>::operator*=(const real_value_type& rhs)
    {
        return (*this)() *= real_batch(rhs);
    }

    /**
     * Divides \c this by the batch \c rhs.
     * @param rhs the batch involved in the division.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_complex_batch<X>::operator/=(const X& rhs)
    {
        using kernel = detail::complex_batch_multiplier<X, is_ieee_compliant<value_type>::value>;
        (*this)() = kernel::div((*this)(), rhs);
        return (*this)();
    }

    /**
     * Divides each scalar contained in \c this by the scalar \c rhs.
     * @param rhs the scalar involved in the division.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_complex_batch<X>::operator/=(const value_type& rhs)
    {
        return (*this)() /= X(rhs);
    }

    /**
     * Divides \c this by the real batch \c rhs.
     * @param rhs the real batch involved in the division.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_complex_batch<X>::operator/=(const real_batch& rhs)
    {
        m_real /= rhs;
        m_imag /= rhs;
        return (*this)();
    }

    /**
     * Divides each scalar contained in \c this by the real scalar \c rhs.
     * @param rhs the real scalar involved in the division.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_complex_batch<X>::operator/=(const real_value_type& rhs)
    {
        return (*this)() /= real_batch(rhs);
    }
    //@}

    /**
     * @name Load and store methods
     */
    //@{
    /**
     * Loads the N contiguous values pointed by \c real_src into the batch holding
     * the real values, and N contiguous values pointed by \c imag_src into the
     * batch holding the imaginary values.
     * \c real_src and \c imag_src must be aligned.
     */
    template <class X>
    template <class T>
    inline X& simd_complex_batch<X>::load_aligned(const T* real_src, const T* imag_src)
    {
        m_real.load_aligned(real_src);
        m_imag.load_aligned(imag_src);
        return (*this)();
    }

    /**
     * Loads the N contiguous values pointed by \c real_src into the batch holding
     * the real values, and N contiguous values pointed by \c imag_src into the
     * batch holding the imaginary values.
     * \c real_src and \c imag_src are not required to be aligned.
     */
    template <class X>
    template <class T>
    inline X& simd_complex_batch<X>::load_unaligned(const T* real_src, const T* imag_src)
    {
        m_real.load_unaligned(real_src);
        m_imag.load_unaligned(imag_src);
        return (*this)();
    }
    
    /**
     * Stores the N values of the batch holding the real values into a contiguous array
     * pointed by \c real_dst., and the N values of the batch holding the imaginary values
     * into a contiguous array pointer by \c imag_dst.
     * \c real_dst and \c imag_dst must be aligned.
     */
    template <class X>
    template <class T>
    inline void simd_complex_batch<X>::store_aligned(T* real_dst, T* imag_dst) const
    {
        m_real.store_aligned(real_dst);
        m_imag.store_aligned(imag_dst);
    }

    /**
     * Stores the N values of the batch holding the real values into a contiguous array
     * pointed by \c real_dst., and the N values of the batch holding the imaginary values
     * into a contiguous array pointer by \c imag_dst.
     * \c real_dst and \c imag_dst are not required to be aligned.
     */
    template <class X>
    template <class T>
    inline void simd_complex_batch<X>::store_unaligned(T* real_dst, T* imag_dst) const
    {
        m_real.store_unaligned(real_dst);
        m_imag.store_unaligned(imag_dst);
    }

    /**
     * Loads the N contiguous values pointed by \c src into the batch.
     * \c src must be aligned.
     */
    template <class X>
    template <class T>
    inline typename std::enable_if<detail::is_complex<T>::value, X&>::type
    simd_complex_batch<X>::load_aligned(const T* src)
    {
        using tmp_value_type = typename T::value_type;
        const tmp_value_type* rbuf = reinterpret_cast<const tmp_value_type*>(src);
        real_batch hi, lo;
        hi.load_aligned(rbuf);
        lo.load_aligned(rbuf + size);
        return (*this)().load_complex(hi, lo);
    }

    /**
     * Loads the N contiguous values pointed by \c src into the batch.
     * \c src is not required to be aligned.
     */
    template <class X>
    template <class T>
    inline typename std::enable_if<detail::is_complex<T>::value, X&>::type
    simd_complex_batch<X>::load_unaligned(const T* src)
    {
        using tmp_value_type = typename T::value_type;
        const tmp_value_type* rbuf = reinterpret_cast<const tmp_value_type*>(src);
        real_batch hi, lo;
        hi.load_unaligned(rbuf);
        lo.load_unaligned(rbuf + size);
        return (*this)().load_complex(hi, lo);
    }

    ///@cond DOXYGEN_INCLUDE_SFINAE
    template <class X>
    template <class T>
    inline typename std::enable_if<!detail::is_complex<T>::value, X&>::type
    simd_complex_batch<X>::load_aligned(const T* src)
    {
        m_real.load_aligned(src);
        m_imag = real_batch(real_value_type(0));
        return (*this)();
    }

    template <class X>
    template <class T>
    inline typename std::enable_if<!detail::is_complex<T>::value, X&>::type
    simd_complex_batch<X>::load_unaligned(const T* src)
    {
        m_real.load_unaligned(src);
        m_imag = real_batch(real_value_type(0));
        return (*this)();
    }
    /// @endcond

    /**
     * Stores the N values of the batch into a contiguous array of complex
     * pointed by \c dst. \c dst must be aligned.
     */
    template <class X>
    template <class T>
    inline typename std::enable_if<detail::is_complex<T>::value, void>::type
    simd_complex_batch<X>::store_aligned(T* dst) const
    {
        real_batch hi = (*this)().get_complex_high();
        real_batch lo = (*this)().get_complex_low();
        using tmp_value_type = typename T::value_type;
        tmp_value_type* rbuf = reinterpret_cast<tmp_value_type*>(dst);
        hi.store_aligned(rbuf);
        lo.store_aligned(rbuf + size);
    }

    /**
     * Stores the N values of the batch into a contiguous array of reals
     * pointed by \c dst. \c dst must be aligned.
     */
    template <class X>
    template <class T>
    inline typename std::enable_if<!detail::is_complex<T>::value, void>::type
    simd_complex_batch<X>::store_aligned(T* dst) const
    {
        m_real.store_aligned(dst);
        assert(all(m_imag == 0) && "no imaginary part");
    }

    /**
     * Stores the N values of the batch into a contiguous array of complex
     * pointed by \c dst. \c dst is not required to be aligned.
     */
    template <class X>
    template <class T>
    inline typename std::enable_if<detail::is_complex<T>::value, void>::type
    simd_complex_batch<X>::store_unaligned(T* dst) const
    {
        real_batch hi = (*this)().get_complex_high();
        real_batch lo = (*this)().get_complex_low();
        using tmp_value_type = typename T::value_type;
        tmp_value_type* rbuf = reinterpret_cast<tmp_value_type*>(dst);
        hi.store_unaligned(rbuf);
        lo.store_unaligned(rbuf + size);
    }

    /**
     * Stores the N values of the batch into a contiguous array of reals
     * pointed by \c dst. \c dst is not required to be aligned.
     */
    template <class X>
    template <class T>
    inline typename std::enable_if<!detail::is_complex<T>::value, void>::type
    simd_complex_batch<X>::store_unaligned(T* dst) const
    {
        m_real.store_aligned(dst);
        assert(all(m_imag == 0) && "no imaginary part");
    }

    //@}

    template <class X>
    inline auto simd_complex_batch<X>::operator[](std::size_t index) const -> value_type
    {
        return value_type(m_real[index], m_imag[index]);
    }

    template <class X>
    inline auto simd_complex_batch<X>::get() -> batch_reference
    {
        return this->derived_cast();
    }

    template <class X>
    inline auto simd_complex_batch<X>::get() const -> const_batch_reference
    {
        return this->derived_cast();
    }

    /********************************
     * simd_complex_batch operators *
     ********************************/

    /**
     * @defgroup simd_complex_batch_arithmetic Arithmetic operators
     */

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * No-op on \c rhs.
     * @tparam X the actual type of batch.
     * @param rhs batch involved in the operation.
     * @return the opposite of \c rhs.
     */
    template <class X>
    inline X operator+(const simd_complex_batch<X>& rhs)
    {
        return rhs();
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the opposite of the batch \c rhs.
     * @tparam X the actual type of batch.
     * @param rhs batch involved in the operation.
     * @return the opposite of \c rhs.
     */
    template <class X>
    inline X operator-(const simd_complex_batch<X>& rhs)
    {
        return X(-rhs().real(), -rhs().imag());
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the sum of the batches \c lhs and \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the addition.
     * @param rhs batch involved in the addition.
     * @return the result of the addition.
     */
    template <class X>
    inline X operator+(const simd_complex_batch<X>& lhs, const simd_complex_batch<X>& rhs)
    {
        X tmp(lhs());
        return tmp += rhs();
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
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
    inline X operator+(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::value_type& rhs)
    {
        X tmp(lhs());
        return tmp += X(rhs);
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
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
    inline X operator+(const typename simd_batch_traits<X>::value_type& lhs, const simd_complex_batch<X>& rhs)
    {
        X tmp(lhs);
        return tmp += rhs();
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the sum of the batches \c lhs and \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the addition.
     * @param rhs real batch involved in the addition.
     * @return the result of the addition.
     */
    template <class X>
    inline X operator+(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::real_batch& rhs)
    {
        X tmp(lhs());
        return tmp += X(rhs);
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the sum of the batches \c lhs and \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs real batch involved in the addition.
     * @param rhs batch involved in the addition.
     * @return the result of the addition.
     */
    template <class X>
    inline X operator+(const typename simd_batch_traits<X>::real_batch& lhs, const simd_complex_batch<X>& rhs)
    {
        X tmp(lhs);
        return tmp += rhs();
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the sum of the batch \c lhs and the real scalar \c rhs. Equivalent to the
     * sum of two batches where all the values of the second one are initialized to
     * \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the addition.
     * @param rhs real scalar involved in the addition.
     * @return the result of the addition.
     */
    template <class X>
    inline X operator+(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::real_value_type& rhs)
    {
        X tmp(lhs());
        return tmp += X(rhs);
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the sum of the real scalar \c lhs and the batch \c rhs. Equivalent to the
     * sum of two batches where all the values of the first one are initialized to
     * \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs real scalar involved in the addition.
     * @param rhs batch involved in the addition.
     * @return the result of the addition.
     */
    template <class X>
    inline X operator+(const typename simd_batch_traits<X>::real_value_type& lhs, const simd_complex_batch<X>& rhs)
    {
        X tmp(lhs);
        return tmp += rhs();
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the difference of the batches \c lhs and \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the difference.
     * @param rhs batch involved in the difference.
     * @return the result of the difference.
     */
    template <class X>
    inline X operator-(const simd_complex_batch<X>& lhs, const simd_complex_batch<X>& rhs)
    {
        X tmp(lhs());
        return tmp -= rhs();
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
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
    inline X operator-(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::value_type& rhs)
    {
        X tmp(lhs());
        return tmp -= X(rhs);
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
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
    inline X operator-(const typename simd_batch_traits<X>::value_type& lhs, const simd_complex_batch<X>& rhs)
    {
        X tmp(lhs);
        return tmp -= rhs();
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the difference of the batches \c lhs and \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the difference.
     * @param rhs real batch involved in the difference.
     * @return the result of the difference.
     */
    template <class X>
    inline X operator-(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::real_batch& rhs)
    {
        X tmp(lhs());
        return tmp -= X(rhs);
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the difference of the batches \c lhs and \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs real batch involved in the difference.
     * @param rhs batch involved in the difference.
     * @return the result of the difference.
     */
    template <class X>
    inline X operator-(const typename simd_batch_traits<X>::real_batch& lhs, const simd_complex_batch<X>& rhs)
    {
        X tmp(lhs);
        return tmp -= rhs();
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the difference of the batch \c lhs and the real scalar \c rhs. Equivalent to the
     * difference of two batches where all the values of the second one are initialized to
     * \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the difference.
     * @param rhs real scalar involved in the difference.
     * @return the result of the difference.
     */
    template <class X>
    inline X operator-(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::real_value_type& rhs)
    {
        X tmp(lhs());
        return tmp -= X(rhs);
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the difference of the real scalar \c lhs and the batch \c rhs. Equivalent to the
     * difference of two batches where all the values of the first one are initialized to
     * \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs real scalar involved in the difference.
     * @param rhs batch involved in the difference.
     * @return the result of the difference.
     */
    template <class X>
    inline X operator-(const typename simd_batch_traits<X>::real_value_type& lhs, const simd_complex_batch<X>& rhs)
    {
        X tmp(lhs);
        return tmp -= rhs();
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the product of the batches \c lhs and \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the product.
     * @param rhs batch involved in the product.
     * @return the result of the product.
     */
    template <class X>
    inline X operator*(const simd_complex_batch<X>& lhs, const simd_complex_batch<X>& rhs)
    {
        X tmp(lhs());
        return tmp *= rhs();
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
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
    inline X operator*(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::value_type& rhs)
    {
        X tmp(lhs());
        return tmp *= X(rhs);
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
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
    inline X operator*(const typename simd_batch_traits<X>::value_type& lhs, const simd_complex_batch<X>& rhs)
    {
        X tmp(lhs);
        return tmp *= rhs();
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the product of the batches \c lhs and \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the product.
     * @param rhs real batch involved in the product.
     * @return the result of the product.
     */
    template <class X>
    inline X operator*(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::real_batch& rhs)
    {
        X tmp(lhs());
        return tmp *= X(rhs);
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the product of the batches \c lhs and \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs real batch involved in the product.
     * @param rhs batch involved in the product.
     * @return the result of the product.
     */
    template <class X>
    inline X operator*(const typename simd_batch_traits<X>::real_batch& lhs, const simd_complex_batch<X>& rhs)
    {
        X tmp(lhs);
        return tmp *= rhs();
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the product of the batch \c lhs and the real scalar \c rhs. Equivalent to the
     * product of two batches where all the values of the second one are initialized to
     * \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the product.
     * @param rhs real scalar involved in the product.
     * @return the result of the product.
     */
    template <class X>
    inline X operator*(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::real_value_type& rhs)
    {
        X tmp(lhs());
        return tmp *= X(rhs);
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the product of the real scalar \c lhs and the batch \c rhs. Equivalent to the
     * difference of two batches where all the values of the first one are initialized to
     * \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs real scalar involved in the product.
     * @param rhs batch involved in the product.
     * @return the result of the product.
     */
    template <class X>
    inline X operator*(const typename simd_batch_traits<X>::real_value_type& lhs, const simd_complex_batch<X>& rhs)
    {
        X tmp(lhs);
        return tmp *= rhs();
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the division of the batch \c lhs by the batch \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the division.
     * @param rhs batch involved in the division.
     * @return the result of the division.
     */
    template <class X>
    inline X operator/(const simd_complex_batch<X>& lhs, const simd_complex_batch<X>& rhs)
    {
        X tmp(lhs());
        return tmp /= rhs();
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
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
    inline X operator/(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::value_type& rhs)
    {
        X tmp(lhs());
        return tmp /= X(rhs);
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
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
    inline X operator/(const typename simd_batch_traits<X>::value_type& lhs, const simd_complex_batch<X>& rhs)
    {
        X tmp(lhs);
        return tmp /= rhs();
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the division of the batch \c lhs by the batch \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the division.
     * @param rhs real batch involved in the division.
     * @return the result of the division.
     */
    template <class X>
    inline X operator/(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::real_batch& rhs)
    {
        X tmp(lhs());
        return tmp /= X(rhs);
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the division of the batch \c lhs by the batch \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs real batch involved in the division.
     * @param rhs batch involved in the division.
     * @return the result of the division.
     */
    template <class X>
    inline X operator/(const typename simd_batch_traits<X>::real_batch& lhs, const simd_complex_batch<X>& rhs)
    {
        X tmp(lhs);
        return tmp /= rhs();
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the division of the batch \c lhs by the real scalar \c rhs. Equivalent to the
     * division of two batches where all the values of the second one are initialized to
     * \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs batch involved in the division.
     * @param rhs real scalar involved in the division.
     * @return the result of the division.
     */
    template <class X>
    inline X operator/(const simd_complex_batch<X>& lhs, const typename simd_batch_traits<X>::real_value_type& rhs)
    {
        X tmp(lhs());
        return tmp /= X(rhs);
    }

    /**
     * @ingroup simd_complex_batch_arithmetic
     *
     * Computes the division of the real scalar \c lhs and the batch \c rhs. Equivalent to the
     * difference of two batches where all the values of the first one are initialized to
     * \c rhs.
     * @tparam X the actual type of batch.
     * @param lhs real scalar involved in the division.
     * @param rhs batch involved in the division.
     * @return the result of the division.
     */
    template <class X>
    inline X operator/(const typename simd_batch_traits<X>::real_value_type& lhs, const simd_complex_batch<X>& rhs)
    {
        X tmp(lhs);
        return tmp /= rhs();
    }


    /**
     * @defgroup simd_complex_batch_reducers Reducers
     */

    /**
     * @ingroup simd_complex_batch_reducers
     *
     * Adds all the scalars of the batch \c rhs.
     * @param rhs batch involved in the reduction
     * @return the result of the reduction.
     */
    template <class X>
    inline typename simd_batch_traits<X>::value_type
    hadd(const simd_complex_batch<X>& rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        return value_type(hadd(rhs.real()), hadd(rhs.imag()));
    }

    /**
     * @defgroup simd_complex_batch_miscellaneous Miscellaneous
     */

    /**
    * @ingroup simd_complex_batch_miscellaneous
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
    inline X select(const typename simd_batch_traits<X>::batch_bool_type& cond,
                    const simd_complex_batch<X>& a,
                    const simd_complex_batch<X>& b)
    {
        return X(select(cond.value(), a.real(), b.real()), select(cond.value(), a.imag(), b.imag()));
    }

    /**
     * @defgroup simd_complex_batch_comparison Comparison operators
     */

    /**
     * @ingroup simd_complex_batch_comparison
     *
     * Element-wise equality comparison of batches \c lhs and \c rhs.
     * @param lhs batch involved in the comparison.
     * @param rhs batch involved in the comparison.
     * @return a boolean batch.
     */
    template <class X>
    inline typename simd_batch_traits<X>::batch_bool_type
    operator==(const simd_complex_batch<X>& lhs, const simd_complex_batch<X>& rhs)
    {
        return (lhs.real() == rhs.real()) && (lhs.imag() == rhs.imag());
    }

    /**
     * @ingroup simd_complex_batch_comparison
     *
     * Element-wise inequality comparison of batches \c lhs and \c rhs.
     * @param lhs batch involved in the comparison.
     * @param rhs batch involved in the comparison.
     * @return a boolean batch.
     */
    template <class X>
    inline typename simd_batch_traits<X>::batch_bool_type
    operator!=(const simd_complex_batch<X>& lhs, const simd_complex_batch<X>& rhs)
    {
        return !(lhs == rhs);
    }

    /**
     * Insert the batch \c rhs into the stream \c out.
     * @tparam X the actual type of batch.
     * @param out the output stream.
     * @param rhs the batch to output.
     * @return the output stream.
     */
    template <class X>
    inline std::ostream& operator<<(std::ostream& out, const simd_complex_batch<X>& rhs)
    {
        out << '(';
        std::size_t s = simd_complex_batch<X>::size;
        for (std::size_t i = 0; i < s - 1; ++i)
        {
            out << "(" << rhs()[i].real() << "," << rhs()[i].imag() << "), ";
        }
        out << "(" << rhs()[s - 1].real() << "," << rhs()[s - 1].imag() << "))";
        return out;
    }
}

#endif

