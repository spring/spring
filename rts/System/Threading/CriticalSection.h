/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CRITICALSECTION_H
#define CRITICALSECTION_H

#include <windows.h>


class criticalsection
{
private:
	typedef CRITICAL_SECTION native_type;

public:
	typedef native_type* native_handle_type;

	criticalsection() noexcept;
	~criticalsection();

	criticalsection(const criticalsection&) = delete;
	criticalsection& operator=(const criticalsection&) = delete;

	void lock();
	bool try_lock() noexcept;
	void unlock();

	native_handle_type native_handle() { return &mtx; }

protected:
	native_type mtx;
};

#endif // CRITICALSECTION_H
