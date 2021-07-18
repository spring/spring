/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_ALIGNED_STACK_BUFFER_HPP
#define XSIMD_ALIGNED_STACK_BUFFER_HPP

#include <type_traits>

#include "xsimd_aligned_allocator.hpp"

namespace xsimd
{

    template <class T, size_t Align>
    class aligned_stack_buffer
    {
    public:

        using allocator = aligned_allocator<T, Align>;
        using value_type = typename allocator::value_type;
        using pointer = typename allocator::pointer;
        using const_pointer = typename allocator::const_pointer;
        using reference = typename allocator::reference;
        using const_reference = typename allocator::const_reference;
        using size_type = typename allocator::size_type;

        static constexpr alignment = allocator::alignment;

        explicit aligned_stack_buffer(size_type n);
        ~aligned_stack_buffer();

        aligned_stack_buffer(const aligned_stack_buffer&) = delete;
        aligned_stack_buffer& operator=(const aligned_stack_buffer&) = delete;

        aligned_stack_buffer(aligned_stack_buffer&&) = delete;
        aligned_stack_buffer& operator=(aligned_stack_buffer&&) = delete;

        size_type size() const noexcept;

        reference operator[](size_type);
        const_reference operator[](size_type) const;

        operator pointer() noexcept;

    private:

        pointer m_ptr;
        size_type m_size;
        bool m_heap_allocation;
    };


    /***************************************
     * aligned_stack_buffer implementation *
     ***************************************/

    namespace detail
    {
        inline void* aligned_alloc_stack(size_t size, size_t alignment)
        {
            return reinterpret_cast<void*>(
                       reinterpret_cast<size_t>(XSIMD_ALLOCA(size + alignment)) &
                       ~(size_t(alignment - 1))) +
                alignment;
        }
    }

    template <class T, size_t A>
    inline aligned_stack_buffer<T, A>::aligned_stack_buffer(size_type n)
        : m_size(n)
    {
#ifdef XSIMD_ALLOCA
        if (sizeof(T) * n <= XSIMD_STACK_ALLOCATION_LIMIT)
        {
            m_ptr = reinterpret_cast<pointer>(
                (reinterpret_cast<size_t>(XSIMD_ALLOCA(n + A)) &
                 ~(size_t(A - 1))) +
                A);
            m_heap_allocation = false;
        }
        else
        {
            m_ptr = reinterpret_cast<pointer>(aligned_malloc(n, A));
            m_heap_allocation = true;
        }
#else
        m_ptr = reinterpret_cast<pointer>(aligned_malloc(n, A));
        m_heap_allocation = true;
#endif
    }

    template <class T, size_t A>
    inline aligned_stack_buffer<T, A>::~aligned_stack_buffer()
    {
        if (!std::is_trivially_destructible<T>::value && m_ptr != 0)
        {
            for (auto p = m_ptr; p < m_ptr + m_size; ++p)
            {
                p->~T();
            }
        }
        if (m_heap_allocation)
        {
            aligned_free(m_ptr);
        }
    }

    template <class T, size_t A>
    inline auto aligned_stack_buffer<T, A>::size() const noexcept -> size_type
    {
        return m_size;
    }

    template <class T, size_t A>
    inline auto
        aligned_stack_buffer<T, A>::operator[](size_type i) -> reference
    {
        return m_ptr[i];
    }

    template <class T, size_t A>
    inline auto
        aligned_stack_buffer<T, A>::operator[](size_type i) const -> const_reference
    {
        return m_ptr[i];
    }

    template <class T, size_t A>
    inline aligned_stack_buffer<T, A>::operator pointer() noexcept
    {
        return m_ptr;
    }
}

#endif
