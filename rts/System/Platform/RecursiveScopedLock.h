/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RECURSIVE_SCOPED_LOCK_H
#define RECURSIVE_SCOPED_LOCK_H

#include <boost/version.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/recursive_mutex.hpp>


namespace Threading {

typedef boost::recursive_mutex RecursiveMutex;

class RecursiveScopedLock {
	char sl_lock[sizeof(boost::recursive_mutex::scoped_lock)];
public:
	RecursiveScopedLock(boost::recursive_mutex& m, bool locked = true) {
#if (BOOST_VERSION >= 103500)
		if (locked) {
			new (sl_lock) boost::recursive_mutex::scoped_lock(m);
		} else {
			new (sl_lock) boost::recursive_mutex::scoped_lock(m, boost::defer_lock);
		}
#else
		new (sl_lock) boost::recursive_mutex::scoped_lock(m, locked);
#endif
	}
	virtual ~RecursiveScopedLock() {
#if (BOOST_VERSION >= 103500)
		((boost::recursive_mutex::scoped_lock*)sl_lock)->~unique_lock();
#else
		((boost::recursive_mutex::scoped_lock*)sl_lock)->~scoped_lock();
#endif
	}
};

};

#endif // RECURSIVE_SCOPED_LOCK_H
