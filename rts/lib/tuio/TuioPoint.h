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

#ifndef INCLUDED_TUIOPOINT_H
#define INCLUDED_TUIOPOINT_H
#include "TuioTime.h"
#include <iostream>

#ifndef M_PI
#define M_PI	3.14159265358979323846

#endif

namespace TUIO {

	/**
	 * The TuioPoint class on the one hand is a simple container and utility class to handle TUIO positions in general,
	 * on the other hand the TuioPoint is the base class for the TuioCursor and TuioObject classes.
	 *
	 * @author Martin Kaltenbrunner
	 * @version 1.4
	 */
	class TuioPoint {

	protected:
		/**
		 * X coordinate, representated as a floating point value in a range of 0..1
		 */
		float xpos;
		/**
		 * X coordinate, representated as a floating point value in a range of 0..1
		 */
		float ypos;
		/**
		 * The time stamp of the last update represented as TuioTime (time since session start)
		 */
		TuioTime currentTime;
		/**
		 * The creation time of this TuioPoint represented as TuioTime (time since session start)
		 */
		TuioTime startTime;

	public:
		/**
		 * The default constructor takes no arguments and sets
		 * its coordinate attributes to zero and its time stamp to the current session time.
		 */
		TuioPoint (float xp, float yp) {
			xpos = xp;
			ypos = yp;
			currentTime = TuioTime::getSessionTime();
			startTime = currentTime;
		};

		/**
		 * This constructor takes a TuioTime object and two floating point coordinate arguments and sets
		 * its coordinate attributes to these values and its time stamp to the provided TUIO time object.
		 *
		 * @param	ttime	the TuioTime to assign
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 */
		TuioPoint (TuioTime ttime, float xp, float yp) {
			xpos = xp;
			ypos = yp;
			currentTime = ttime;
			startTime = currentTime;
		};

		/**
		 * This constructor takes a TuioPoint argument and sets its coordinate attributes
		 * to the coordinates of the provided TuioPoint and its time stamp to the current session time.
		 *
		 * @param	tpoint	the TuioPoint to assign
		 */
		TuioPoint (TuioPoint *tpoint) {
			xpos = tpoint->getX();
			ypos = tpoint->getY();
			currentTime = TuioTime::getSessionTime();
			startTime = currentTime;
		};

		/**
		 * The destructor is doing nothing in particular.
		 */
		~TuioPoint(){};

		/**
		 * Takes a TuioPoint argument and updates its coordinate attributes
		 * to the coordinates of the provided TuioPoint and leaves its time stamp unchanged.
		 *
		 * @param	tpoint	the TuioPoint to assign
		 */
		void update (TuioPoint *tpoint) {
			xpos = tpoint->getX();
			ypos = tpoint->getY();
		};

		/**
		 * Takes two floating point coordinate arguments and updates its coordinate attributes
		 * to the coordinates of the provided TuioPoint and leaves its time stamp unchanged.
		 *
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 */
		void update (float xp, float yp) {
			xpos = xp;
			ypos = yp;
		};

		/**
		 * Takes a TuioTime object and two floating point coordinate arguments and updates its coordinate attributes
		 * to the coordinates of the provided TuioPoint and its time stamp to the provided TUIO time object.
		 *
		 * @param	ttime	the TuioTime to assign
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 */
		void update (TuioTime ttime, float xp, float yp) {
			xpos = xp;
			ypos = yp;
			currentTime = ttime;
		};


		/**
		 * Returns the X coordinate of this TuioPoint.
		 * @return	the X coordinate of this TuioPoint
		 */
		float getX() {
			return xpos;
		};

		/**
		 * Returns the Y coordinate of this TuioPoint.
		 * @return	the Y coordinate of this TuioPoint
		 */
		float getY() {
			return ypos;
		};

		/**
		 * Returns the distance to the provided coordinates
		 *
		 * @param	xp	the X coordinate of the distant point
		 * @param	yp	the Y coordinate of the distant point
		 * @return	the distance to the provided coordinates
		 */
		float getDistance(float xp, float yp) {
			float dx = xpos-xp;
			float dy = ypos-yp;
			return ::sqrtf(dx*dx+dy*dy);
		}

		/**
		 * Returns the distance to the provided TuioPoint
		 *
		 * @param	tpoint	the distant TuioPoint
		 * @return	the distance to the provided TuioPoint
		 */
		float getDistance(TuioPoint *tpoint) {
			return getDistance(tpoint->getX(),tpoint->getY());
		}

		/**
		 * Returns the angle to the provided coordinates
		 *
		 * @param	xp	the X coordinate of the distant point
		 * @param	yp	the Y coordinate of the distant point
		 * @return	the angle to the provided coordinates
		 */
		 float getAngle(float xp, float yp) {
			float side = xpos-xp;
			float height = ypos-yp;
			float distance = getDistance(xp,yp);

			float angle = (float)(asin(side/distance)+M_PI/2);
			if (height<0) angle = 2.0f*(float)M_PI-angle;

			return angle;
		}

		/**
		 * Returns the angle to the provided TuioPoint
		 *
		 * @param	tpoint	the distant TuioPoint
		 * @return	the angle to the provided TuioPoint
		 */
		float getAngle(TuioPoint *tpoint) {
			return getAngle(tpoint->getX(),tpoint->getY());
		}

		/**
		 * Returns the angle in degrees to the provided coordinates
		 *
		 * @param	xp	the X coordinate of the distant point
		 * @param	yp	the Y coordinate of the distant point
		 * @return	the angle in degrees to the provided TuioPoint
		 */
		float getAngleDegrees(float xp, float yp) {
			return ((getAngle(xp,yp)/(float)M_PI)*180.0f);
		}

		/**
		 * Returns the angle in degrees to the provided TuioPoint
		 *
		 * @param	tpoint	the distant TuioPoint
		 * @return	the angle in degrees to the provided TuioPoint
		 */
		float getAngleDegrees(TuioPoint *tpoint) {
			return ((getAngle(tpoint)/(float)M_PI)*180.0f);
		}

		/**
		 * Returns the X coordinate in pixels relative to the provided screen width.
		 *
		 * @param	width	the screen width
		 * @return	the X coordinate of this TuioPoint in pixels relative to the provided screen width
		 */
		int getScreenX(int width) {
			return (int)floor(xpos*width+0.5f);
		};

		/**
		 * Returns the Y coordinate in pixels relative to the provided screen height.
		 *
		 * @param	height	the screen height
		 * @return	the Y coordinate of this TuioPoint in pixels relative to the provided screen height
		 */
		int getScreenY(int height) {
			return (int)floor(ypos*height+0.5f);
		};

		/**
		 * Returns current time stamp of this TuioPoint as TuioTime
		 *
		 * @return	the  time stamp of this TuioPoint as TuioTime
		 */
		TuioTime getTuioTime() {
			return currentTime;
		};

		/**
		 * Returns the start time of this TuioPoint as TuioTime.
		 *
		 * @return	the start time of this TuioPoint as TuioTime
		 */
		TuioTime getStartTime() {
			return startTime;
		};
	};
};
#endif
