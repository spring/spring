/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SAFE_UTIL_H
#define SAFE_UTIL_H

#include <limits>
#include <cstring>

namespace spring {
	template<class T> inline void SafeDestruct(T*& p)
	{
		if (p == nullptr) return;
		T* tmp = p; p = nullptr; tmp->~T();
	}

	/**
	 * @brief Safe alternative to "delete obj;"
	 * Safely deletes an object, by first setting the pointer to NULL and then
	 * deleting.
	 * This way, it is guaranteed that other objects can not access the object
	 * through the pointer while the object is running its destructor.
	 */
	template<class T> inline void SafeDelete(T& p)
	{
		T tmp = p; p = nullptr; delete tmp;
	}

	/**
	 * @brief Safe alternative to "delete [] obj;"
	 * Safely deletes an array object, by first setting the pointer to NULL and then
	 * deleting.
	 * This way, it is guaranteed that other objects can not access the object
	 * through the pointer while the object is running its destructor.
	 */
	template<class T> inline void SafeDeleteArray(T*& p)
	{
		T* tmp = p; p = nullptr; delete[] tmp;
	}


	template<typename T> inline T SafeDivide(const T a, const T b)
	{
		if (b == T(0))
			return a;

		return (a / b);
	}

	// Updated version of https://stackoverflow.com/questions/49658182/does-c-have-an-equivalent-boostnumeric-castdesttypesourcetype

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
    template<typename TOut, typename TIn>
    inline TOut SafeCast(TIn value)
    {
        using DstLim = std::numeric_limits<TOut>;
        using SrcLim = std::numeric_limits<TIn>;

        constexpr bool positive_overflow_possible = DstLim::max() < SrcLim::max();
        constexpr bool negative_overflow_possible = SrcLim::is_signed || (DstLim::lowest() > SrcLim::lowest());

        // unsigned <-- unsigned
        if constexpr ((!DstLim::is_signed) && (!SrcLim::is_signed)) {
            if (positive_overflow_possible && (value > DstLim::max())) {
                return DstLim::max();
            }
        }
        // unsigned <-- signed
        else if constexpr ((!DstLim::is_signed) && SrcLim::is_signed) {
            if (positive_overflow_possible && (value > DstLim::max())) {
                return DstLim::max();
            }
            else if (negative_overflow_possible && (value < 0)) {
                return static_cast<TOut>(0);
            }

        }
        // signed <-- unsigned
        else if constexpr (DstLim::is_signed && (!SrcLim::is_signed)) {
            if (positive_overflow_possible && (value > DstLim::max())) {
                return DstLim::max();
            }
        }
        // signed <-- signed
        else if constexpr (DstLim::is_signed && SrcLim::is_signed) {
            if (positive_overflow_possible && (value > DstLim::max())) {
                return DstLim::max();
            }
            else if (negative_overflow_possible && (value < DstLim::lowest())) {
                return DstLim::lowest();
            }
        }

        // limits have been checked, therefore safe to cast
        return static_cast<TOut>(value);
    }
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

    template<typename TOut, typename TIn>
    TOut bit_cast(TIn t1) {
        static_assert(sizeof(TIn) == sizeof(TOut), "Types must match sizes");
        static_assert(std::is_trivially_copyable<TIn>::value , "Requires TriviallyCopyable input");
        static_assert(std::is_trivially_copyable<TOut>::value, "Requires TriviallyCopyable output");
        static_assert(std::is_trivially_constructible_v<TOut>,
            "This implementation additionally requires destination type to be trivially constructible");

        TOut t2;
        std::memcpy(std::addressof(t2), std::addressof(t1), sizeof(TIn));
        return t2;
    }


};

#endif

