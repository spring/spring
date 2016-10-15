/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRINGFUTEX_H
#define SPRINGFUTEX_H

#include <cinttypes>


class spring_futex
{
private:
	typedef std::uint32_t native_type;

public:
	typedef native_type* native_handle_type;

	spring_futex() noexcept;
	~spring_futex();

	spring_futex(const spring_futex&) = delete;
	spring_futex& operator=(const spring_futex&) = delete;

	void lock();
	bool try_lock() noexcept;
	void unlock();

	native_handle_type native_handle() { return &mtx; }

protected:
	native_type mtx;
};

/*FIXME
class recursive_futex
{
private:
	typedef std::uint32_t native_type;

public:
	typedef native_type* native_handle_type;

	recursive_futex() noexcept;
	~recursive_futex();

	recursive_futex(const recursive_futex&) = delete;
	recursive_futex& operator=(const recursive_futex&) = delete;

	void lock();
	bool try_lock() noexcept;
	void unlock();

	native_handle_type native_handle() { return &mtx; }

protected:
	native_type mtx;
};
*/
#endif // SPRINGFUTEX_H
