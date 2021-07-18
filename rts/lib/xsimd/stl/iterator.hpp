/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_ITERATOR_HPP
#define XSIMD_ITERATOR_HPP

namespace xsimd
{
    template <class B>
    class batch_proxy;

    template <class B>
    struct simd_batch_traits<batch_proxy<B>>
        : simd_batch_traits<B>
    {
    };

    template <class X>
    struct simd_batch_inner_types<batch_proxy<X>>
    {
        using batch_reference = X;
        using const_batch_reference = X;
    };

    /**
     * Aligned proxy that iterators can dereference to
     */
    template <class B>
    class batch_proxy : public simd_base<batch_proxy<B>>
    {
    public:

        using self_type = batch_proxy<B>;
        using base_type = simd_type<self_type>;
        using batch_reference = typename base_type::batch_reference;
        using const_batch_reference = typename base_type::const_batch_reference;
        using batch_type = B;
        using value_type = typename B::value_type;
        using pointer = value_type*;

        batch_proxy(pointer ptr);

        batch_reference get();
        const_batch_reference get() const;

        self_type& set(const batch_type& rhs);
        self_type& set(const self_type& rhs);

        operator batch_type() const;

        batch_proxy& operator=(const batch_type& rhs);

    private:

        value_type* m_ptr;
    };

    template <class X>
    struct is_proxy : std::false_type
    {
    };

    template <class X>
    struct is_proxy<batch_proxy<X>> : std::true_type
    {
    };

    template <class X>
    std::ostream& operator<<(std::ostream& os, const batch_proxy<X>& bp)
    {
        return os << bp.get();
    }

    template <class B>
    class aligned_iterator
    {
    public:

        using self_type = aligned_iterator<B>;
        using batch_type = B;
        using value_type = typename B::value_type;
        static constexpr std::size_t batch_size = B::size;

        using proxy_type = batch_proxy<B>;
        using pointer = value_type*;
        using reference = proxy_type;

        aligned_iterator(pointer memory);

        reference operator*();

        void operator++(int);
        aligned_iterator& operator++();

        bool equal(const aligned_iterator& rhs) const;

    private:

        pointer m_cur_pointer;
    };

    /******************************
     * batch proxy implementation *
     *****************************/

    template <class B>
    inline batch_proxy<B>::batch_proxy(pointer ptr)
        : m_ptr(ptr)
    {
    }

    template <class B>
    inline auto batch_proxy<B>::get() -> batch_reference
    {
        return batch_type(*this);
    }

    template <class B>
    inline auto batch_proxy<B>::get() const -> const_batch_reference
    {
        return batch_type(*this);
    }

    template <class B>
    inline auto batch_proxy<B>::set(const batch_type& rhs) -> self_type&
    {
        xsimd::store_aligned(m_ptr, rhs);
        return *this;
    }

    template <class B>
    inline auto batch_proxy<B>::set(const self_type& rhs) -> self_type&
    {
        xsimd::store_aligned(m_ptr, rhs.get());
        return *this;
    }

    template <class B>
    inline batch_proxy<B>::operator batch_type() const
    {
        batch_type m_reg;
        m_reg.load_aligned(m_ptr);
        return m_reg;
    }

    template <class B>
    inline auto batch_proxy<B>::operator=(const batch_type& rhs) -> batch_proxy&
    {
        xsimd::store_aligned(m_ptr, rhs);
        return *this;
    }

    /***********************************
     * aligned iterator implementation *
     ***********************************/

    template <class B>
    inline aligned_iterator<B>::aligned_iterator(pointer memory)
        : m_cur_pointer(memory)
    {
    }

    template <class B>
    inline auto aligned_iterator<B>::operator*() -> reference
    {
        return m_cur_pointer;
    }

    template <class B>
    inline aligned_iterator<B>& aligned_iterator<B>::operator++()
    {
        m_cur_pointer += batch_size;
        return *this;
    }

    template <class B>
    inline void aligned_iterator<B>::operator++(int)
    {
        m_cur_pointer += batch_size;
    }

    template <class B>
    inline bool aligned_iterator<B>::equal(const aligned_iterator& rhs) const
    {
        return m_cur_pointer == rhs.m_cur_pointer;
    }

    template <class B>
    inline bool operator==(const aligned_iterator<B>& lhs, const aligned_iterator<B>& rhs)
    {
        return lhs.equal(rhs);
    }

    template <class B>
    inline bool operator!=(const aligned_iterator<B>& lhs, const aligned_iterator<B>& rhs)
    {
        return !lhs.equal(rhs);
    }

#if defined(_WIN32) && defined(__clang__)
    // See comment at the end of simd_base.hpp
    template <class B>
    inline B fma(const batch_proxy<B>& a, const batch_proxy<B>& b, const batch_proxy<B>& c)
    {
        using base_type = simd_base<batch_proxy<B>>;
        const base_type& sba = a;
        const base_type& sbb = b;
        const base_type& sbc = c;
        return fma(sba, sbb, sbc);
    }
#endif
}

#endif
