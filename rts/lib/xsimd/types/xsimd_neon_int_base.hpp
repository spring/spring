/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_NEON_INT_BASE_HPP
#define XSIMD_NEON_INT_BASE_HPP

namespace xsimd
{
    namespace detail
    {
        template <class B>
        struct neon_int_kernel_base
        {
            using batch_type = B;

            static batch_type fmin(const batch_type& lhs, const batch_type& rhs)
            {
                return min(lhs, rhs);
            }

            static batch_type fmax(const batch_type& lhs, const batch_type& rhs)
            {
                return max(lhs, rhs);
            }

            static batch_type fabs(const batch_type& rhs)
            {
                return abs(rhs);
            }
            static batch_type fma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return x * y + z;
            }

            static batch_type fms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return x * y - z;
            }

            static batch_type fnma(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return -x * y + z;
            }

            static batch_type fnms(const batch_type& x, const batch_type& y, const batch_type& z)
            {
                return -x * y - z;
            }
        };
    }
}

#endif

