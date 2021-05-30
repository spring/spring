/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SAFE_UTIL_H
#define SAFE_UTIL_H

#include <limits>

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


	template<typename TIn, typename TOut>
	TOut SafeCast(const TIn input)
	{
		if constexpr (std::is_same_v<TIn, TOut>)
			return input;

		constexpr TOut minOut = std::numeric_limits<TOut>::lowest();
		constexpr TOut maxOut = std::numeric_limits<TOut>::max();
		constexpr TIn  minIn  = std::numeric_limits<TIn >::lowest();
		constexpr TIn  maxIn  = std::numeric_limits<TIn >::max();

		const TIn realMinIn = minOut <= minIn ? minIn : static_cast<TIn>(minOut);
		const TIn realMaxIn = maxOut >= maxIn ? maxIn : static_cast<TIn>(maxOut);

		if (input < realMinIn) return realMinIn; // underflow
		if (input > realMaxIn) return realMaxIn; // overflow
		return static_cast<TOut>(input);
	}

};

#endif

