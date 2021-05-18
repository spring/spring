/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_ALGORITHMS_HPP
#define XSIMD_ALGORITHMS_HPP

#include "../memory/xsimd_load_store.hpp"

namespace xsimd
{
    template <class I1, class I2, class O1, class UF>
    void transform(I1 first, I2 last, O1 out_first, UF&& f)
    {
        using value_type = typename std::decay<decltype(*first)>::type;
        using traits = simd_traits<value_type>;
        using batch_type = typename traits::type;

        std::size_t size = static_cast<std::size_t>(std::distance(first, last));
        std::size_t simd_size = traits::size;

        const auto* ptr_begin = &(*first);
        auto* ptr_out = &(*out_first);

        std::size_t align_begin = xsimd::get_alignment_offset(ptr_begin, size, simd_size);
        std::size_t out_align = xsimd::get_alignment_offset(ptr_out, size, simd_size);
        std::size_t align_end = align_begin + ((size - align_begin) & ~(simd_size - 1));

        if (align_begin == out_align)
        {
            for (std::size_t i = 0; i < align_begin; ++i)
            {
                out_first[i] = f(first[i]);
            }

            batch_type batch;
            for (std::size_t i = align_begin; i < align_end; i += simd_size)
            {
                xsimd::load_aligned(&first[i], batch);
                xsimd::store_aligned(&out_first[i], f(batch));
            }

            for (std::size_t i = align_end; i < size; ++i)
            {
                out_first[i] = f(first[i]);
            }
        }
        else
        {
            for (std::size_t i = 0; i < align_begin; ++i)
            {
                out_first[i] = f(first[i]);
            }

            batch_type batch;
            for (std::size_t i = align_begin; i < align_end; i += simd_size)
            {
                xsimd::load_aligned(&first[i], batch);
                xsimd::store_unaligned(&out_first[i], f(batch));
            }

            for (std::size_t i = align_end; i < size; ++i)
            {
                out_first[i] = f(first[i]);
            }
        }
    }

    template <class I1, class I2, class I3, class O1, class UF>
    void transform(I1 first_1, I2 last_1, I3 first_2, O1 out_first, UF&& f)
    {
        using value_type = typename std::decay<decltype(*first_1)>::type;
        using traits = simd_traits<value_type>;
        using batch_type = typename traits::type;

        std::size_t size = static_cast<std::size_t>(std::distance(first_1, last_1));
        std::size_t simd_size = traits::size;

        const auto* ptr_begin_1 = &(*first_1);
        const auto* ptr_begin_2 = &(*first_2);
        auto* ptr_out = &(*out_first);

        std::size_t align_begin_1 = xsimd::get_alignment_offset(ptr_begin_1, size, simd_size);
        std::size_t align_begin_2 = xsimd::get_alignment_offset(ptr_begin_2, size, simd_size);
        std::size_t out_align = xsimd::get_alignment_offset(ptr_out, size, simd_size);
        std::size_t align_end = align_begin_1 + ((size - align_begin_1) & ~(simd_size - 1));

        #define XSIMD_LOOP_MACRO(A1, A2, A3)                                    \
            for (std::size_t i = 0; i < align_begin_1; ++i)                     \
            {                                                                   \
                out_first[i] = f(first_1[i], first_2[i]);                       \
            }                                                                   \
                                                                                \
            batch_type batch_1, batch_2;                                        \
            for (std::size_t i = align_begin_1; i < align_end; i += simd_size)  \
            {                                                                   \
                xsimd::A1(&first_1[i], batch_1);                                \
                xsimd::A2(&first_2[i], batch_2);                                \
                xsimd::A3(&out_first[i], f(batch_1, batch_2));                  \
            }                                                                   \
                                                                                \
            for (std::size_t i = align_end; i < size; ++i)                      \
            {                                                                   \
                out_first[i] = f(first_1[i], first_2[i]);                       \
            }                                                                   \

        if (align_begin_1 == out_align && align_begin_1 == align_begin_2)
        {
            XSIMD_LOOP_MACRO(load_aligned, load_aligned, store_aligned);
        }
        else if (align_begin_1 == out_align && align_begin_1 != align_begin_2)
        {
            XSIMD_LOOP_MACRO(load_aligned, load_unaligned, store_aligned);
        }
        else if (align_begin_1 != out_align && align_begin_1 == align_begin_2)
        {
            XSIMD_LOOP_MACRO(load_aligned, load_aligned, store_unaligned);
        }
        else if (align_begin_1 != out_align && align_begin_1 != align_begin_2)
        {
            XSIMD_LOOP_MACRO(load_aligned, load_unaligned, store_unaligned);
        }

        #undef XSIMD_LOOP_MACRO
    }


    // TODO: Remove this once we drop C++11 support
    namespace detail
    {
        struct plus
        {
            template <class X, class Y>
            auto operator()(X&& x, Y&& y) -> decltype(x + y) { return x + y; }
        };
    }


    template <class Iterator1, class Iterator2, class Init, class BinaryFunction = detail::plus>
    Init reduce(Iterator1 first, Iterator2 last, Init init, BinaryFunction&& binfun = detail::plus{})
    {
        using value_type = typename std::decay<decltype(*first)>::type;
        using traits = simd_traits<value_type>;
        using batch_type = typename traits::type;

        std::size_t size = static_cast<std::size_t>(std::distance(first, last));
        constexpr std::size_t simd_size = traits::size;

        if(size < simd_size)
        {
            while(first != last)
            {
                init = binfun(init, *first++);
            }
            return init;
        }

        const auto* const ptr_begin = &(*first);

        std::size_t align_begin = xsimd::get_alignment_offset(ptr_begin, size, simd_size);
        std::size_t align_end = align_begin + ((size - align_begin) & ~(simd_size - 1));

        // reduce initial unaligned part
        for (std::size_t i = 0; i < align_begin; ++i)
        {
            init = binfun(init, first[i]);
        }

        // reduce aligned part
        batch_type batch_init, batch;
        auto ptr = ptr_begin + align_begin;
        xsimd::load_aligned(ptr, batch_init);
        ptr += simd_size;
        for (auto const end = ptr_begin + align_end; ptr < end; ptr += simd_size)
        {
            xsimd::load_aligned(ptr, batch);
            batch_init = binfun(batch_init, batch);
        }

        // reduce across batch
        alignas(batch_type) std::array<value_type, simd_size> arr;
        xsimd::store_aligned(arr.data(), batch_init);
        for (auto x : arr) init = binfun(init, x);

        // reduce final unaligned part
        for (std::size_t i = align_end; i < size; ++i)
        {
            init = binfun(init, first[i]);
        }

        return init;
    }

}

#endif
