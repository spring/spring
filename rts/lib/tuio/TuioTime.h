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

#ifndef INCLUDED_TUIOTIME_H
#define INCLUDED_TUIOTIME_H

#ifndef WIN32
#include <pthread.h>
#include <sys/time.h>
#else
#include <windows.h>
#endif

#define MSEC_SECOND 1000
#define USEC_SECOND 1000000
#define USEC_MILLISECOND 1000

namespace TUIO {
	
	/**
	 * The TuioTime class is a simple structure that is used to reprent the time that has elapsed since the session start.
	 * The time is internally represented as seconds and fractions of microseconds which should be more than sufficient for gesture related timing requirements.
	 * Therefore at the beginning of a typical TUIO session the static method initSession() will set the reference time for the session. 
	 * Another important static method getSessionTime will return a TuioTime object representing the time elapsed since the session start.
	 * The class also provides various addtional convience method, which allow some simple time arithmetics.
	 *
	 * @author Martin Kaltenbrunner
	 * @version 1.4
	 */ 
	class TuioTime {
		
	private:
		long seconds, micro_seconds;
		static long start_seconds, start_micro_seconds;
		
	public:

		/**
		 * The default constructor takes no arguments and sets   
		 * the Seconds and Microseconds attributes of the newly created TuioTime both to zero.
		 */
		TuioTime () {
			seconds = 0;
			micro_seconds = 0;
		};

		/**
		 * The destructor is doing nothing in particular. 
		 */
		~TuioTime() {}
		
		/**
		 * This constructor takes the provided time represented in total Milliseconds 
		 * and assigs this value to the newly created TuioTime.
		 *
		 * @param  msec  the total time in Millseconds
		 */
		TuioTime (long msec) {
			seconds = msec/MSEC_SECOND;
			micro_seconds = USEC_MILLISECOND*(msec%MSEC_SECOND);
		};
		
		/**
		 * This constructor takes the provided time represented in Seconds and Microseconds   
		 * and assigs these value to the newly created TuioTime.
		 *
		 * @param  sec  the total time in seconds
		 * @param  usec	the microseconds time component
		 */	
		TuioTime (long sec, long usec) {
			seconds = sec;
			micro_seconds = usec;
		};

		/**
		 * Sums the provided time value represented in total Microseconds to this TuioTime.
		 *
		 * @param  us	the total time to add in Microseconds
		 * @return the sum of this TuioTime with the provided argument in microseconds
		 */	
		TuioTime operator+(long us) {
			long sec = seconds + us/USEC_SECOND;
			long usec = micro_seconds + us%USEC_SECOND;
			return TuioTime(sec,usec);
		};
		
		/**
		 * Sums the provided TuioTime to the private Seconds and Microseconds attributes.  
		 *
		 * @param  ttime	the TuioTime to add
		 * @return the sum of this TuioTime with the provided TuioTime argument
		 */
		TuioTime operator+(TuioTime ttime) {
			long sec = seconds + ttime.getSeconds();
			long usec = micro_seconds + ttime.getMicroseconds();
			sec += usec/USEC_SECOND;
			usec = usec%USEC_SECOND;
			return TuioTime(sec,usec);
		};

		/**
		 * Subtracts the provided time represented in Microseconds from the private Seconds and Microseconds attributes.
		 *
		 * @param  us	the total time to subtract in Microseconds
		 * @return the subtraction result of this TuioTime minus the provided time in Microseconds
		 */		
		TuioTime operator-(long us) {
			long sec = seconds - us/USEC_SECOND;
			long usec = micro_seconds - us%USEC_SECOND;
			
			if (usec<0) {
				usec += USEC_SECOND;
				sec--;
			}			
			
			return TuioTime(sec,usec);
		};

		/**
		 * Subtracts the provided TuioTime from the private Seconds and Microseconds attributes.
		 *
		 * @param  ttime	the TuioTime to subtract
		 * @return the subtraction result of this TuioTime minus the provided TuioTime
		 */	
		TuioTime operator-(TuioTime ttime) {
			long sec = seconds - ttime.getSeconds();
			long usec = micro_seconds - ttime.getMicroseconds();
			
			if (usec<0) {
				usec += USEC_SECOND;
				sec--;
			}
			
			return TuioTime(sec,usec);
		};

		
		/**
		 * Assigns the provided TuioTime to the private Seconds and Microseconds attributes.
		 *
		 * @param  ttime	the TuioTime to assign
		 */	
		void operator=(TuioTime ttime) {
			seconds = ttime.getSeconds();
			micro_seconds = ttime.getMicroseconds();
		};
		
		/**
		 * Takes a TuioTime argument and compares the provided TuioTime to the private Seconds and Microseconds attributes.
		 *
		 * @param  ttime	the TuioTime to compare
		 * @return true if the two TuioTime have equal Seconds and Microseconds attributes
		 */	
		bool operator==(TuioTime ttime) {
			if ((seconds==(long)ttime.getSeconds()) && (micro_seconds==(long)ttime.getMicroseconds())) return true;
			else return false;
		};

		/**
		 * Takes a TuioTime argument and compares the provided TuioTime to the private Seconds and Microseconds attributes.
		 *
		 * @param  ttime	the TuioTime to compare
		 * @return true if the two TuioTime have differnt Seconds or Microseconds attributes
		 */	
		bool operator!=(TuioTime ttime) {
			if ((seconds!=(long)ttime.getSeconds()) || (micro_seconds!=(long)ttime.getMicroseconds())) return true;
			else return false;
		};
		
		/**
		 * Resets the seconds and micro_seconds attributes to zero.
		 */
		void reset() {
			seconds = 0;
			micro_seconds = 0;
		};
		
		/**
		 * Returns the TuioTime Seconds component.
		 * @return the TuioTime Seconds component
		 */	
		long getSeconds() {
			return seconds;
		};
		
		/**
		 * Returns the TuioTime Microseconds component.
		 * @return the TuioTime Microseconds component
		 */	
		long getMicroseconds() {
			return micro_seconds;
		};
		
		/**
		 * Returns the total TuioTime in Milliseconds.
		 * @return the total TuioTime in Milliseconds
		 */	
		long getTotalMilliseconds() {
			return seconds*MSEC_SECOND+micro_seconds/MSEC_SECOND;
		};
		
		/**
		 * This static method globally resets the TUIO session time.
		 */		
		static void initSession();
		
		/**
		 * Returns the present TuioTime representing the time since session start.
		 * @return the present TuioTime representing the time since session start
		 */			
		static TuioTime getSessionTime();
		
		/**
		 * Returns the absolut TuioTime representing the session start.
		 * @return the absolut TuioTime representing the session start
		 */			
		static TuioTime getStartTime();
		
		/**
		 * Returns the absolut TuioTime representing the current system time.
		 * @return the absolut TuioTime representing the current system time
		 */	
		static TuioTime getSystemTime();
	};
};
#endif /* INCLUDED_TUIOTIME_H */
