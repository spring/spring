/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Futex.h"
#include <cstdlib>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <climits>
#include <algorithm>


spring_futex::spring_futex() noexcept
{
	mtx = 0;
}


spring_futex::~spring_futex()
{
	mtx = 0;
}


void spring_futex::lock()
{
	native_type c;
	if ((c = __sync_val_compare_and_swap(&mtx, 0, 1)) == 0)
		return;

	do {
		if ((c == 2) || __sync_val_compare_and_swap(&mtx, 1, 2) != 0)
			syscall(SYS_futex, &mtx, FUTEX_WAIT_PRIVATE, 2, NULL, NULL, 0);
	} while((c = __sync_val_compare_and_swap(&mtx, 0, 2)) != 0);
}


bool spring_futex::try_lock() noexcept
{
	return __sync_bool_compare_and_swap(&mtx, 0, 1);
}


void spring_futex::unlock()
{
	if (__sync_fetch_and_sub(&mtx, 1) != 1) {
		mtx = 0;
		syscall(SYS_futex, &mtx, FUTEX_WAKE_PRIVATE, 4, NULL, NULL, 0);
	}
}



/*
recursive_futex::recursive_futex() noexcept
{
	mtx = 0;
}


recursive_futex::~recursive_futex()
{
	mtx = 0;
}


void recursive_futex::lock()
{
	native_type c;
	if ((c = __sync_val_compare_and_swap(&mtx, 0, 1)) == 0)
		return;

	do {
		if ((c == 2) || __sync_val_compare_and_swap(&mtx, 1, 2) != 0)
			syscall(SYS_futex, &mtx, FUTEX_WAIT_PRIVATE, 2, NULL, NULL, 0);
	} while((c = __sync_val_compare_and_swap(&mtx, 0, 2)) != 0);
}


bool recursive_futex::try_lock() noexcept
{
	return __sync_bool_compare_and_swap(&mtx, 0, 1);
}


void recursive_futex::unlock()
{
	if (__sync_fetch_and_sub(&mtx, 1) != 1) {
		mtx = 0;
		syscall(SYS_futex, &mtx, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
	}
}
*/














linux_signal::linux_signal() noexcept
: sleepers(false)
, gen(0)
, mtx(0)
{
}


linux_signal::~linux_signal()
{
	gen = {0};
	mtx = 0;
}


void linux_signal::wait()
{
	int m; // cur gen
	const int g = gen.load(); // our gen
	sleepers++;
	while ((g - (m = mtx)) >= 0) {
		syscall(SYS_futex, &mtx, FUTEX_WAIT_PRIVATE, m, NULL, NULL, 0);
	}
	sleepers--;
}


void linux_signal::wait_for(spring_time t)
{
	int m; // cur gen
	const int g = gen.load(); // our gen
	sleepers++;

	struct timespec linux_t;
	linux_t.tv_sec  = 0;
	linux_t.tv_nsec = t.toNanoSecsi();

	const spring_time endTimer = spring_now() + t;

	while (((g - (m = mtx)) >= 0) && (spring_now() < endTimer)) {
		syscall(SYS_futex, &mtx, FUTEX_WAIT_PRIVATE, m, &linux_t, NULL, 0);
	}
	sleepers--;
}


void linux_signal::notify_all(const int min_sleepers)
{
	if (sleepers.load() < std::max(1, min_sleepers))
		return;

	mtx = gen++;
	syscall(SYS_futex, &mtx, FUTEX_WAKE_PRIVATE, INT_MAX, NULL, NULL, 0);
}

