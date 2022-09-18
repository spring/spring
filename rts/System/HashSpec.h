#pragma once

#include <tuple>
namespace spring {
    namespace
    {

        // Code from boost
        // Reciprocal of the golden ratio helps spread entropy
        //     and handles duplicates.
        // See Mike Seymour in magic-numbers-in-boosthash-combine:
        //     http://stackoverflow.com/questions/4948780

        template <class T>
        inline void hash_combine(std::uint64_t& seed, T const& v)
        {
            seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        }

        // Recursive template code derived from Matthieu M.
        template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
        struct HashValueImpl
        {
          static void apply(uint64_t& seed, Tuple const& tuple)
          {
            HashValueImpl<Tuple, Index-1>::apply(seed, tuple);
            hash_combine(seed, std::get<Index>(tuple));
          }
        };

        template <class Tuple>
        struct HashValueImpl<Tuple,0>
        {
          static void apply(uint64_t& seed, Tuple const& tuple)
          {
            hash_combine(seed, std::get<0>(tuple));
          }
        };
    }

    template <class T>
    inline uint64_t hash_combine(T const& v, std::uint64_t seed = 1337u)
    {
        seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
    template <class T>
    inline uint64_t hash_combine(uint64_t hashValue, std::uint64_t seed = 1337u)
    {
        seed ^= hashValue + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
}

template <typename ... TT>
struct std::hash<std::tuple<TT...>>
{
    uint64_t operator()(std::tuple<TT...> const& tt) const
    {
        uint64_t seed = 0;
        spring::HashValueImpl<std::tuple<TT...> >::apply(seed, tt);
        return seed;
    }
};