/***************************************************************************
* Copyright (c) Serge Guelton                                              *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_BASE_CONSTANT_HPP
#define XSIMD_BASE_CONSTANT_HPP

namespace xsimd
{
    template <class X>
    class simd_base;

    template <class T, bool... Values>
    struct batch_bool_constant
    {
        static constexpr std::size_t size = sizeof...(Values);
        using value_type = bool;
        using batch_type =
            typename simd_batch_traits<batch<T, size>>::batch_bool_type;

        batch_type operator()() const { return *this; }

        operator batch_type() const { return {Values...}; }

        bool operator[](size_t i) const
        {
            return std::array<value_type, size>{{Values...}}[i];
        }
        static constexpr int mask()
        {
            return mask_helper(0, static_cast<int>(Values)...);
        }

      private:
        static constexpr int mask_helper(int acc) { return acc; }
        template <class... Tys>
        static constexpr int mask_helper(int acc, int mask, Tys... masks)
        {
            return mask_helper(acc | mask, (masks << 1)...);
        }
    };

    template <class T, T... Values>
    struct batch_constant
    {
        static constexpr std::size_t size = sizeof...(Values);
        using value_type = T;
        using batch_type = batch<T, size>;

        batch_type operator()() const { return *this; }

        operator batch_type() const { return {Values...}; }

        constexpr T operator[](size_t i) const
        {
            return std::array<value_type, size>{Values...}[i];
        }
    };

    namespace detail
    {
        template <class G, std::size_t... Is>
        constexpr auto make_batch_constant(detail::index_sequence<Is...>)
            -> batch_constant<decltype(G::get(0, 0)),
                              G::get(Is, sizeof...(Is))...>
        {
            return {};
        }
        template <class T, class G, std::size_t... Is>
        constexpr auto make_batch_bool_constant(detail::index_sequence<Is...>)
            -> batch_bool_constant<T, G::get(Is, sizeof...(Is))...>
        {
            return {};
        }
        template <class T, T value, std::size_t... Is>
        constexpr auto make_batch_constant(detail::index_sequence<Is...>)
            -> batch_constant<T, (Is, value)...>
        {
            return {};
        }
        template <class T, T value, std::size_t... Is>
        constexpr auto make_batch_bool_constant(detail::index_sequence<Is...>)
            -> batch_bool_constant<T, (Is, value)...>
        {
            return {};
        }
    } // namespace detail

    template <class G, std::size_t N>
    constexpr auto make_batch_constant() -> decltype(
        detail::make_batch_constant<G>(detail::make_index_sequence<N>()))
    {
        return detail::make_batch_constant<G>(detail::make_index_sequence<N>());
    }

    template <class T, class G, std::size_t N>
    constexpr auto make_batch_bool_constant()
        -> decltype(detail::make_batch_bool_constant<T, G>(
            detail::make_index_sequence<N>()))
    {
        return detail::make_batch_bool_constant<T, G>(
            detail::make_index_sequence<N>());
    }

} // namespace xsimd

#endif
