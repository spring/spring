/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * This wraps a couple of useful boost c++ time functions,
 * for use in sdlstub
 * (which is written in C, and ca not easily access C++ stuff directly)
 */

#include "boost/thread.hpp"

extern "C" int getsystemmilliseconds() {
   boost::xtime t;
		boost::xtime_get(&t, boost::TIME_UTC);
   int milliseconds = t.sec * 1000 + (t.nsec / 1000000 );   
   return milliseconds;
}

extern "C" void sleepmilliseconds( int milliseconds ) {
   boost::xtime t;
		boost::xtime_get(&t, boost::TIME_UTC);
		t.nsec += 1000000 * milliseconds;
		boost::thread::sleep(t);
}

