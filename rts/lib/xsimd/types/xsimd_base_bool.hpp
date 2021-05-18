/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_BASE_BOOL_HPP
#define XSIMD_BASE_BOOL_HPP

#include <ostream>

namespace xsimd
{

    template <class T, std::size_t N>
    class batch_bool;

    template <class X>
    struct simd_batch_traits;

    namespace detail
    {
        template <class T, std::size_t N>
        struct batch_bool_kernel;
    }

    /**************
     * bool_proxy *
     **************/

    template <class T>
    class bool_proxy
    {
    public:

        bool_proxy(T& ref);

        bool_proxy(const bool_proxy&) = default;
        bool_proxy& operator=(const bool_proxy&) = default;

        bool_proxy(bool_proxy&&) = default;
        bool_proxy& operator=(bool_proxy&&) = default;

        bool_proxy& operator=(bool rhs);
        operator bool() const;

    private:

        T& m_ref;
    };

    /*******************
     * simd_batch_bool *
     *******************/

    /**
     * @class simd_batch_bool
     * @brief Base class for batch of boolean values.
     *
     * The simd_batch_bool class is the base class for all classes representing
     * a batch of boolean values. Batch of boolean values is meant for operations
     * that may involve batches of integer or floating point values. Thus,
     * the boolean values are stored as integer or floating point values, and each
     * type of batch has its dedicated type of boolean batch.
     *
     * @tparam X The derived type
     * @sa simd_batch
     */
    template <class X>
    class simd_batch_bool
    {
    public:

        using value_type = typename simd_batch_traits<X>::value_type;
        static constexpr std::size_t size = simd_batch_traits<X>::size;

        X& operator&=(const X& rhs);
        X& operator|=(const X& rhs);
        X& operator^=(const X& rhs);

        X& operator()();
        const X& operator()() const;

        X& load_aligned(const bool* src);
        X& load_unaligned(const bool* src);

        void store_aligned(bool* dst) const;
        void store_unaligned(bool* dst) const;

        template <class P>
        X& load_aligned(const P& src);
        template <class P>
        X& load_unaligned(const P& src);

        template <class P>
        void store_aligned(P& dst) const;
        template <class P>
        void store_unaligned(P& dst) const;

    protected:

        simd_batch_bool() = default;
        ~simd_batch_bool() = default;

        simd_batch_bool(const simd_batch_bool&) = default;
        simd_batch_bool& operator=(const simd_batch_bool&) = default;

        simd_batch_bool(simd_batch_bool&&) = default;
        simd_batch_bool& operator=(simd_batch_bool&&) = default;

    private:

        template <class P, std::size_t... I>
        X& load_impl(detail::index_sequence<I...>, const P& src);

        template <class P>
        void store_impl(P& dst) const;
    };

    template <class X>
    X operator&(const simd_batch_bool<X>& lhs, const simd_batch_bool<X>&rhs);

    template <class X>
    X operator|(const simd_batch_bool<X>& lhs, const simd_batch_bool<X>&rhs);

    template <class X>
    X operator^(const simd_batch_bool<X>& lhs, const simd_batch_bool<X>&rhs);

    template <class X>
    X operator~(const simd_batch_bool<X>& rhs);

    template <class X>
    X bitwise_andnot(const simd_batch_bool<X>& lhs, const simd_batch_bool<X>&rhs);

    template <class X>
    X operator==(const simd_batch_bool<X>& lhs, const simd_batch_bool<X>&rhs);

    template <class X>
    X operator!=(const simd_batch_bool<X>& lhs, const simd_batch_bool<X>&rhs);

    template <class X>
    bool all(const simd_batch_bool<X>& rhs);

    template <class X>
    bool any(const simd_batch_bool<X>& rhs);

    template <class X>
    X operator&&(const simd_batch_bool<X>& lhs, const simd_batch_bool<X>& rhs);

    template <class X>
    X operator&&(const simd_batch_bool<X>& lhs, bool rhs);

    template <class X>
    X operator&&(bool lhs, const simd_batch_bool<X>& rhs);

    template <class X>
    X operator||(const simd_batch_bool<X>& lhs, const simd_batch_bool<X>& rhs);

    template <class X>
    X operator||(const simd_batch_bool<X>& lhs, bool rhs);

    template <class X>
    X operator||(bool lhs, const simd_batch_bool<X>& rhs);

    template <class X>
    X operator!(const simd_batch_bool<X>& rhs);

    template <class X>
    std::ostream& operator<<(std::ostream& out, const simd_batch_bool<X>& rhs);

    /*****************************
     * bool_proxy implementation *
     *****************************/

    template <class T>
    inline bool_proxy<T>::bool_proxy(T& ref)
        : m_ref(ref)
    {
    }

    template <class T>
    inline bool_proxy<T>& bool_proxy<T>::operator=(bool rhs)
    {
        m_ref = static_cast<T>(rhs);
        return *this;
    }

    template <class T>
    inline bool_proxy<T>::operator bool() const
    {
        return static_cast<bool>(m_ref);
    }

    /**********************************
     * simd_batch_bool implementation *
     **********************************/

    /**
     * @name Bitwise computed assignement
     */
    //@{
    /**
     * Assigns the bitwise and of \c rhs and \c this.
     * @param rhs the batch involved in the operation.
     * @return a reference to \c this.
     */
    template <class X>
    inline X& simd_batch_bool<X>::operator&=(const X& rhs)
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
    inline X& simd_batch_bool<X>::operator|=(const X& rhs)
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
    inline X& simd_batch_bool<X>::operator^=(const X& rhs)
    {
        (*this)() = (*this)() ^ rhs;
        return (*this)();
    }
    //@}

    /**
     * @name Static downcast functions
     */
    //@{
    /**
     * Returns a reference to the actual derived type of the simd_batch_bool.
     */
    template <class X>
    inline X& simd_batch_bool<X>::operator()()
    {
        return *static_cast<X*>(this);
    }

    /**
     * Returns a constant reference to the actual derived type of the simd_batch_bool.
     */
    template <class X>
    const X& simd_batch_bool<X>::operator()() const
    {
        return *static_cast<const X*>(this);
    }
    //@}
    
    template <class X>
    inline X& simd_batch_bool<X>::load_aligned(const bool* src)
    {
        return load_impl(detail::make_index_sequence<size>(), src);
    }

    template <class X>
    inline X& simd_batch_bool<X>::load_unaligned(const bool* src)
    {
        return load_aligned(src);
    }

    template <class X>
    inline void simd_batch_bool<X>::store_aligned(bool* dst) const
    {
        store_impl(dst);
    }

    template <class X>
    inline void simd_batch_bool<X>::store_unaligned(bool* dst) const
    {
        store_impl(dst);
    }

    template <class X>
    template <class P>
    inline X& simd_batch_bool<X>::load_aligned(const P& src)
    {
        return load_impl(detail::make_index_sequence<size>(), src);
    }

    template <class X>
    template <class P>
    inline X& simd_batch_bool<X>::load_unaligned(const P& src)
    {
        return load_aligned(src);
    }   

    template <class X>
    template <class P>
    inline void simd_batch_bool<X>::store_aligned(P& dst) const
    {
        store_impl(dst);
    }

    template <class X>
    template <class P>
    inline void simd_batch_bool<X>::store_unaligned(P& dst) const
    {
        store_impl(dst);
    }
    
    template <class X>
    template <class P, std::size_t... I>
    inline X& simd_batch_bool<X>::load_impl(detail::index_sequence<I...>, const P& src)
    {
        return (*this)().load_values(src[I]...);
    }

    template <class X>
    template <class P>
    inline void simd_batch_bool<X>::store_impl(P& dst) const
    {
        for(std::size_t i = 0; i < size; ++i)
        {
            dst[i] = (*this)()[i];
        }
    }

    /**
    * @defgroup simd_batch_bool_bitwise Bitwise functions
    */

    /**
     * @ingroup simd_batch_bool_bitwise
     *
     * Computes the bitwise and of batches \c lhs and \c rhs.
     * @tparam X the actual type of boolean batch.
     * @param lhs batch involved in the operation.
     * @param rhs batch involved in the operation.
     * @return the result of the bitwise and.
     */
    template <class X>
    inline X operator&(const simd_batch_bool<X>& lhs, const simd_batch_bool<X>&rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_bool_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::bitwise_and(lhs(), rhs());
    }

    /**
     * @ingroup simd_batch_bool_bitwise
     *
     * Computes the bitwise or of batches \c lhs and \c rhs.
     * @tparam X the actual type of boolean batch.
     * @param lhs batch involved in the operation.
     * @param rhs batch involved in the operation.
     * @return the result of the bitwise or.
     */
    template <class X>
    inline X operator|(const simd_batch_bool<X>& lhs, const simd_batch_bool<X>&rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_bool_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::bitwise_or(lhs(), rhs());
    }

    /**
     * @ingroup simd_batch_bool_bitwise
     *
     * Computes the bitwise xor of batches \c lhs and \c rhs.
     * @tparam X the actual type of boolean batch.
     * @param lhs batch involved in the operation.
     * @param rhs batch involved in the operation.
     * @return the result of the bitwise xor.
     */
    template <class X>
    inline X operator^(const simd_batch_bool<X>& lhs, const simd_batch_bool<X>&rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_bool_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::bitwise_xor(lhs(), rhs());
    }

    /**
     * @ingroup simd_batch_bool_bitwise
     *
     * Computes the bitwise not of batch \c rhs.
     * @tparam X the actual type of boolean batch.
     * @param rhs batch involved in the operation.
     * @return the result of the bitwise not.
     */
    template <class X>
    inline X operator~(const simd_batch_bool<X>& rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_bool_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::bitwise_not(rhs());
    }

    /**
     * @ingroup simd_batch_bool_bitwise
     *
     * Computes the bitwise and not of batches \c lhs and \c rhs.
     * @tparam X the actual type of boolean batch.
     * @param lhs batch involved in the operation.
     * @param rhs batch involved in the operation.
     * @return the result of the bitwise and not.
     */
    template <class X>
    X bitwise_andnot(const simd_batch_bool<X>& lhs, const simd_batch_bool<X>& rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_bool_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::bitwise_andnot(lhs(), rhs());
    }

    /**
     * @defgroup simd_batch_bool_comparison Comparison operators
     */

    /**
     * @ingroup simd_batch_bool_comparison
     *
     * Element-wise equality of batches \c lhs and \c rhs.
     * @param lhs batch involved in the comparison.
     * @param rhs batch involved in the comparison.
     * @return the result of the equality comparison.
     */
    template <class X>
    inline X operator==(const simd_batch_bool<X>& lhs, const simd_batch_bool<X>&rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_bool_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::equal(lhs(), rhs());
    }

    /**
     * @ingroup simd_batch_bool_comparison
     *
     * Element-wise inequality of batches \c lhs and \c rhs.
     * @param lhs batch involved in the comparison.
     * @param rhs batch involved in the comparison.
     * @return the result of the inequality comparison.
     */
    template <class X>
    inline X operator!=(const simd_batch_bool<X>& lhs, const simd_batch_bool<X>&rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_bool_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::not_equal(lhs(), rhs());
    }

    /**
     * @defgroup simd_batch_bool_reducers Reducers
     */

    /**
     * @ingroup simd_batch_bool_reducers
     *
     * Returns true if all the boolean values in the batch are true,
     * false otherwise.
     * @param rhs the batch to reduce.
     * @return a boolean scalar.
     */
    template <class X>
    inline bool all(const simd_batch_bool<X>& rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_bool_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::all(rhs());
    }

    /**
     * @ingroup simd_batch_bool_reducers
     *
     * Return true if any of the boolean values in the batch is true,
     * false otherwise.
     * @param rhs the batch to reduce.
     * @return a boolean scalar.
     */
    template <class X>
    inline bool any(const simd_batch_bool<X>& rhs)
    {
        using value_type = typename simd_batch_traits<X>::value_type;
        using kernel = detail::batch_bool_kernel<value_type, simd_batch_traits<X>::size>;
        return kernel::any(rhs());
    }

    /**
     * @defgroup simd_batch_bool_logical Logical functions
     */

    /**
     * @ingroup simd_batch_bool_logical
     *
     * Computes the logical and of batches \c lhs and \c rhs.
     * @tparam X the actual type of boolean batch.
     * @param lhs batch involved in the operation.
     * @param rhs batch involved in the operation.
     * @return the result of the logical and.
     */
    template <class X>
    inline X operator&&(const simd_batch_bool<X>& lhs, const simd_batch_bool<X>& rhs)
    {
        return lhs() & rhs();
    }

    /**
     * @ingroup simd_batch_bool_logical
     *
     * Computes the logical and of the batch \c lhs and the scalar \c rhs.
     * Equivalent to the logical and of two boolean batches, where all the
     * values of the second one are initialized to \c rhs.
     * @tparam X the actual type of boolean batch.
     * @param lhs batch involved in the operation.
     * @param rhs boolean involved in the operation.
     * @return the result of the logical and.
     */
    template <class X>
    inline X operator&&(const simd_batch_bool<X>& lhs, bool rhs)
    {
        return lhs() & X(rhs);
    }

    /**
     * @ingroup simd_batch_bool_logical
     *
     * Computes the logical and of the scalar \c lhs and the batch \c rhs.
     * Equivalent to the logical and of two boolean batches, where all the
     * values of the first one are initialized to \c lhs.
     * @tparam X the actual type of boolean batch.
     * @param lhs boolean involved in the operation.
     * @param rhs batch involved in the operation.
     * @return the result of the logical and.
     */
    template <class X>
    inline X operator&&(bool lhs, const simd_batch_bool<X>& rhs)
    {
        return X(lhs) & rhs();
    }

    /**
     * @ingroup simd_batch_bool_logical
     *
     * Computes the logical or of batches \c lhs and \c rhs.
     * @tparam X the actual type of boolean batch.
     * @param lhs batch involved in the operation.
     * @param rhs batch involved in the operation.
     * @return the result of the logical or.
     */
    template <class X>
    inline X operator||(const simd_batch_bool<X>& lhs, const simd_batch_bool<X>& rhs)
    {
        return lhs() | rhs();
    }

    /**
     * @ingroup simd_batch_bool_logical
     *
     * Computes the logical or of the batch \c lhs and the scalar \c rhs.
     * Equivalent to the logical or of two boolean batches, where all the
     * values of the second one are initialized to \c rhs.
     * @tparam X the actual type of boolean batch.
     * @param lhs batch involved in the operation.
     * @param rhs boolean involved in the operation.
     * @return the result of the logical or.
     */
    template <class X>
    inline X operator||(const simd_batch_bool<X>& lhs, bool rhs)
    {
        return lhs() | X(rhs);
    }

    /**
     * @ingroup simd_batch_bool_logical
     *
     * Computes the logical or of the scalar \c lhs and the batch \c rhs.
     * Equivalent to the logical or of two boolean batches, where all the
     * values of the first one are initialized to \c lhs.
     * @tparam X the actual type of boolean batch.
     * @param lhs boolean involved in the operation.
     * @param rhs batch involved in the operation.
     * @return the result of the logical or.
     */
    template <class X>
    inline X operator||(bool lhs, const simd_batch_bool<X>& rhs)
    {
        return X(lhs) | rhs();
    }

    /*
     * @ingroup simd_batch_bool_logical
     *
     * Computes the logical not of \c rhs.
     * @tparam X the actual type of boolean batch.
     * @param rhs batch involved in the operation.
     * @return the result og the logical not.
     */
    template <class X>
    inline X operator!(const simd_batch_bool<X>& rhs)
    {
        return rhs() == X(false);
    }

    /**
     * Insert the batch \c rhs into the stream \c out.
     * @tparam X the actual type of batch.
     * @param out the output stream.
     * @param rhs the batch to output.
     * @return the output stream.
     */
    template <class X>
    inline std::ostream& operator<<(std::ostream& out, const simd_batch_bool<X>& rhs)
    {
        out << '(';
        std::size_t s = simd_batch_bool<X>::size;
        for (std::size_t i = 0; i < s - 1; ++i)
        {
            out << (rhs()[i] ? 'T' : 'F') << ", ";
        }
        out << (rhs()[s - 1] ? 'T' : 'F') << ')';
        return out;
    }
}

#endif

