/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_NEON_UTILS_HPP
#define XSIMD_NEON_UTILS_HPP

namespace xsimd
{
    namespace neon_detail
    {
        template <class R, class T, class F, std::size_t... I>
        inline R unroll_op_impl(F&& f, detail::index_sequence<I...>)
        {
            return R{static_cast<T>(f(I))...};
        }

        template <std::size_t N, class R, class T, class F>
        inline R unroll_op(F&& f)
        {
            return unroll_op_impl<R, T>(f, detail::make_index_sequence<N>{});
        }
    }
}

#endif
