/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SAFE_UTIL_H
#define SAFE_UTIL_H

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
};

#endif

