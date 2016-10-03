/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Futex.h"
#include <sys/syscall.h>
#include <linux/futex.h>
#include <cstdlib>


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
		syscall(SYS_futex, &mtx, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
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

