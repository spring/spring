// Copyright Hugh Perkins 2009
// hughperkins@gmail.com http://manageddreams.com
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
//  more details.
//
// You should have received a copy of the GNU General Public License along
// with this program in the file licence.txt; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-
// 1307 USA
// You can find the licence also on the web at:
// http://www.opensource.org/licenses/gpl-license.php
//

#include "boost/thread.hpp"

// this wraps a couple of useful boost c++ time functions, for use in sdlstub (which is written
// in C, and can't easily access C++ stuff directly)

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

