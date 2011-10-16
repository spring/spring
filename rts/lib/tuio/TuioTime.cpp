/*
 TUIO C++ Library - part of the reacTIVision project
 http://reactivision.sourceforge.net/
 
 Copyright (c) 2005-2009 Martin Kaltenbrunner <mkalten@iua.upf.edu>
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "TuioTime.h"
using namespace TUIO;
	
long TuioTime::start_seconds = 0;
long TuioTime::start_micro_seconds = 0;

void TuioTime::initSession() {
	TuioTime startTime = TuioTime::getSystemTime();
	start_seconds = startTime.getSeconds();
	start_micro_seconds = startTime.getMicroseconds();
}

TuioTime TuioTime::getSessionTime() {
	return  (getSystemTime() - getStartTime());
}

TuioTime TuioTime::getStartTime() {
	return TuioTime(start_seconds,start_micro_seconds);
}

TuioTime TuioTime::getSystemTime() {
#ifdef WIN32
	TuioTime systemTime(GetTickCount());
#else
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv,&tz);
	TuioTime systemTime(tv.tv_sec,tv.tv_usec);
#endif	
	return systemTime;
}
