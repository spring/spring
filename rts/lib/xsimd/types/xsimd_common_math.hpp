/***************************************************************************
* Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and         *
* Martin Renou                                                             *
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XSIMD_COMMON_MATH_HPP
#define XSIMD_COMMON_MATH_HPP

#include <limits>
#include <type_traits>

namespace xsimd
{
    /*********************************************
     * Some utility math operations shared       *
     * across scalar versio and fallback         *
     * versions                                  *
     *********************************************/
    namespace detail
    {
        template <class T0, class T1>
        inline T0
        ipow(const T0& t0, const T1& t1)
        {
            static_assert(std::is_integral<T1>::value, "second argument must be an integer");
            T0 a = t0;
            T1 b = t1;
            bool const recip = b < 0;
            T0 r{static_cast<T0>(1)};
            while (1)
            {
                if (b & 1)
                {
                    r *= a;
                }
                b /= 2;
                if (b == 0)
                {
                    break;
                }
                a *= a;
            }
            return recip ? 1 / r : r;
        }
        template<typename T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
        T sadd(const T& lhs, const T& rhs)
        {
            if (std::numeric_limits<T>::is_signed)
            {
                if ((lhs > 0) && (rhs > std::numeric_limits<T>::max() - lhs))
                {
                    return std::numeric_limits<T>::max();
                }
                else if ((lhs < 0) && (rhs < std::numeric_limits<T>::lowest() - lhs))
                {
                    return std::numeric_limits<T>::lowest();
                }
                else {
                    return lhs + rhs;
                }
            }
            else
            {
                if (rhs > std::numeric_limits<T>::max() - lhs)
                {
                    return std::numeric_limits<T>::max();
                }
                else
                {
                    return lhs + rhs;
                }

            }
        }

        template<typename T, class = typename std::enable_if<std::is_scalar<T>::value>::type>
        T ssub(const T& lhs, const T& rhs)
        {
            if (std::numeric_limits<T>::is_signed)
            {
                return sadd(lhs, (T)-rhs);
            }
            else
            {
                if (lhs < rhs)
                {
                    return std::numeric_limits<T>::lowest();
                }
                else
                {
                    return lhs - rhs;
                }

            }
        }
    }
}

#endif
