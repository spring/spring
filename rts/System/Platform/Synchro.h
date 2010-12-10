/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SYNCHRO_H_
#define _SYNCHRO_H_

#include <boost/version.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include "Rendering/GL/myGL.h"

namespace Threading {

typedef boost::recursive_mutex RecursiveMutex;

class RecursiveScopedLock {
	char sl_lock[sizeof(boost::recursive_mutex::scoped_lock)];
#if GML_DEBUG_MUTEX
	boost::recursive_mutex *m1;
#endif
public:
	RecursiveScopedLock(boost::recursive_mutex &m, bool locked = true) {
#if (BOOST_VERSION >= 103500)
		if(locked)
			new (sl_lock) boost::recursive_mutex::scoped_lock(m);
		else
			new (sl_lock) boost::recursive_mutex::scoped_lock(m, boost::defer_lock);
#else
		new (sl_lock) boost::recursive_mutex::scoped_lock(m, locked);
#endif
#if GML_DEBUG_MUTEX
		if(locked) {
			m1 = &m;
			GML_STDMUTEX_LOCK(lm);
			std::map<boost::recursive_mutex *, int> &lockmmap = lockmmaps[gmlThreadNumber];
			std::map<boost::recursive_mutex *, int>::iterator locki = lockmmap.find(m1);
			if(locki == lockmmap.end())
				lockmmap[m1] = 1;
			else
				lockmmap[m1] = (*locki).second + 1;
		}
		else
			m1 = NULL;
#endif
	}
	virtual ~RecursiveScopedLock() {
#if (BOOST_VERSION >= 103500)
		((boost::recursive_mutex::scoped_lock *)sl_lock)->~unique_lock();
#else
		((boost::recursive_mutex::scoped_lock *)sl_lock)->~scoped_lock();
#endif
#if GML_DEBUG_MUTEX
		if(m1) {
			GML_STDMUTEX_LOCK(lm);
			std::map<boost::recursive_mutex *, int> &lockmmap = lockmmaps[gmlThreadNumber];
			lockmmap[m1] = (*lockmmap.find(m1)).second - 1;
		}
#endif
	}
};

};

#endif // _SYNCHRO_H_
