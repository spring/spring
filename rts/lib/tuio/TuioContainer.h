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

#ifndef INCLUDED_TUIOCONTAINER_H
#define INCLUDED_TUIOCONTAINER_H

#include <list>
#include <math.h>
#include "TuioPoint.h"
#include <iostream>

#define TUIO_ADDED 0
#define TUIO_ACCELERATING 1
#define TUIO_DECELERATING 2
#define TUIO_STOPPED 3
#define TUIO_REMOVED 4

namespace TUIO {
	
	/**
	 * The abstract TuioContainer class defines common attributes that apply to both subclasses {@link TuioObject} and {@link TuioCursor}.
	 *
	 * @author Martin Kaltenbrunner
	 * @version 1.4
	 */ 
	class TuioContainer: public TuioPoint {
		
	protected:
		/**
		 * The unique session ID number that is assigned to each TUIO object or cursor.
		 */ 
		long session_id;
		/**
		 * The X-axis velocity value.
		 */ 
		float x_speed;
		/**
		 * The Y-axis velocity value.
		 */ 
		float y_speed;
		/**
		 * The motion speed value.
		 */ 
		float motion_speed;
		/**
		 * The motion acceleration value.
		 */ 
		float motion_accel;
		/**
		 * A List of TuioPoints containing all the previous positions of the TUIO component.
		 */ 
		std::list<TuioPoint> path;
		/**
		 * Reflects the current state of the TuioComponent
		 */ 
		int state;
		
	public:
		/**
		 * This constructor takes a TuioTime argument and assigns it along with the provided 
		 * Session ID, X and Y coordinate to the newly created TuioContainer.
		 *
		 * @param	ttime	the TuioTime to assign
		 * @param	si	the Session ID to assign
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 */
		TuioContainer (TuioTime ttime, long si, float xp, float yp):TuioPoint(ttime, xp,yp) {
			session_id = si;
			x_speed = 0.0f;
			y_speed = 0.0f;
			motion_speed = 0.0f;
			motion_accel = 0.0f;			
			TuioPoint p(currentTime,xpos,ypos);
			path.push_back(p);
			
			state = TUIO_ADDED;
		};

		/**
		 * This constructor takes the provided Session ID, X and Y coordinate 
		 * and assigs these values to the newly created TuioContainer.
		 *
		 * @param	si	the Session ID to assign
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 */
		TuioContainer (long si, float xp, float yp):TuioPoint(xp,yp) {
			session_id = si;
			x_speed = 0.0f;
			y_speed = 0.0f;
			motion_speed = 0.0f;
			motion_accel = 0.0f;			
			TuioPoint p(currentTime,xpos,ypos);
			path.push_back(p);
			
			state = TUIO_ADDED;
		};
		
		/**
		 * This constructor takes the atttibutes of the provided TuioContainer 
		 * and assigs these values to the newly created TuioContainer.
		 *
		 * @param	tcon	the TuioContainer to assign
		 */
		TuioContainer (TuioContainer *tcon):TuioPoint(tcon) {
			session_id = tcon->getSessionID();
			x_speed = 0.0f;
			y_speed = 0.0f;
			motion_speed = 0.0f;
			motion_accel = 0.0f;
			TuioPoint p(currentTime,xpos,ypos);
			path.push_back(p);
			
			state = TUIO_ADDED;
		};
		
		/**
		 * The destructor is doing nothing in particular. 
		 */
		virtual ~TuioContainer(){};
		
		/**
		 * Takes a TuioTime argument and assigns it along with the provided 
		 * X and Y coordinate to the private TuioContainer attributes.
		 * The speed and accleration values are calculated accordingly.
		 *
		 * @param	ttime	the TuioTime to assign
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 */
		virtual void update (TuioTime ttime, float xp, float yp) {
			TuioPoint lastPoint = path.back();
			TuioPoint::update(ttime,xp, yp);
			
			TuioTime diffTime = currentTime - lastPoint.getTuioTime();
			float dt = diffTime.getTotalMilliseconds()/1000.0f;
			float dx = xpos - lastPoint.getX();
			float dy = ypos - lastPoint.getY();
			float dist = sqrt(dx*dx+dy*dy);
			float last_motion_speed = motion_speed;
			
			x_speed = dx/dt;
			y_speed = dy/dt;
			motion_speed = dist/dt;
			motion_accel = (motion_speed - last_motion_speed)/dt;
			
			TuioPoint p(currentTime,xpos,ypos);
			path.push_back(p);
			
			if (motion_accel>0) state = TUIO_ACCELERATING;
			else if (motion_accel<0) state = TUIO_DECELERATING;
			else state = TUIO_STOPPED;
		};

		
		/**
		 * This method is used to calculate the speed and acceleration values of
		 * TuioContainers with unchanged positions.
		 */
		virtual void stop(TuioTime ttime) {
			update(ttime,xpos,ypos);
		};

		/**
		 * Takes a TuioTime argument and assigns it along with the provided 
		 * X and Y coordinate, X and Y velocity and acceleration
		 * to the private TuioContainer attributes.
		 *
		 * @param	ttime	the TuioTime to assign
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	xs	the X velocity to assign
		 * @param	ys	the Y velocity to assign
		 * @param	ma	the acceleration to assign
		 */
		virtual void update (TuioTime ttime, float xp, float yp, float xs, float ys, float ma) {
			TuioPoint::update(ttime,xp, yp);
			x_speed = xs;
			y_speed = ys;
			motion_speed = (float)sqrt(x_speed*x_speed+y_speed*y_speed);
			motion_accel = ma;
			
			TuioPoint p(currentTime,xpos,ypos);
			path.push_back(p);
			
			if (motion_accel>0) state = TUIO_ACCELERATING;
			else if (motion_accel<0) state = TUIO_DECELERATING;
			else state = TUIO_STOPPED;
		};
		
		/**
		 * Assigns the provided X and Y coordinate, X and Y velocity and acceleration
		 * to the private TuioContainer attributes. The TuioTime time stamp remains unchanged.
		 *
		 * @param	xp	the X coordinate to assign
		 * @param	yp	the Y coordinate to assign
		 * @param	xs	the X velocity to assign
		 * @param	ys	the Y velocity to assign
		 * @param	ma	the acceleration to assign
		 */
		virtual void update (float xp, float yp, float xs, float ys, float ma) {
			TuioPoint::update(xp,yp);
			x_speed = xs;
			y_speed = ys;
			motion_speed = (float)sqrt(x_speed*x_speed+y_speed*y_speed);
			motion_accel = ma;
			
			path.pop_back();
			TuioPoint p(currentTime,xpos,ypos);
			path.push_back(p);
			
			if (motion_accel>0) state = TUIO_ACCELERATING;
			else if (motion_accel<0) state = TUIO_DECELERATING;
			else state = TUIO_STOPPED;
		};
		
		/**
		 * Takes the atttibutes of the provided TuioContainer 
		 * and assigs these values to this TuioContainer.
		 * The TuioTime time stamp of this TuioContainer remains unchanged.
		 *
		 * @param	tcon	the TuioContainer to assign
		 */
		virtual void update (TuioContainer *tcon) {
			TuioPoint::update(tcon);
			x_speed = tcon->getXSpeed();
			y_speed =  tcon->getYSpeed();
			motion_speed =  tcon->getMotionSpeed();
			motion_accel = tcon->getMotionAccel();
			
			TuioPoint p(tcon->getTuioTime(),xpos,ypos);
			path.push_back(p);
			
			if (motion_accel>0) state = TUIO_ACCELERATING;
			else if (motion_accel<0) state = TUIO_DECELERATING;
			else state = TUIO_STOPPED;
		};
		
		/**
		 * Assigns the REMOVE state to this TuioContainer and sets
		 * its TuioTime time stamp to the provided TuioTime argument.
		 *
		 * @param	ttime	the TuioTime to assign
		 */
		virtual void remove(TuioTime ttime) {
			currentTime = ttime;
			state = TUIO_REMOVED;
		}

		/**
		 * Returns the Session ID of this TuioContainer.
		 * @return	the Session ID of this TuioContainer
		 */
		virtual long getSessionID() { 
			return session_id;
		};
		
		/**
		 * Returns the X velocity of this TuioContainer.
		 * @return	the X velocity of this TuioContainer
		 */
		virtual float getXSpeed() { 
			return x_speed;
		};

		/**
		 * Returns the Y velocity of this TuioContainer.
		 * @return	the Y velocity of this TuioContainer
		 */
		virtual float getYSpeed() { 
			return y_speed;
		};
		
		/**
		 * Returns the position of this TuioContainer.
		 * @return	the position of this TuioContainer
		 */
		virtual TuioPoint getPosition() {
			TuioPoint p(xpos,ypos);
			return p;
		};
		
		/**
		 * Returns the path of this TuioContainer.
		 * @return	the path of this TuioContainer
		 */
		virtual std::list<TuioPoint> getPath() {
			return path;
		};
		
		/**
		 * Returns the motion speed of this TuioContainer.
		 * @return	the motion speed of this TuioContainer
		 */
		virtual float getMotionSpeed() {
			return motion_speed;
		};
		
		/**
		 * Returns the motion acceleration of this TuioContainer.
		 * @return	the motion acceleration of this TuioContainer
		 */
		virtual float getMotionAccel() {
			return motion_accel;
		};
		
		/**
		 * Returns the TUIO state of this TuioContainer.
		 * @return	the TUIO state of this TuioContainer
		 */
		virtual int getTuioState() { 
			return state;
		};	
		
		/**
		 * Returns true of this TuioContainer is moving.
		 * @return	true of this TuioContainer is moving
		 */
		virtual bool isMoving() { 
			if ((state==TUIO_ACCELERATING) || (state==TUIO_DECELERATING)) return true;
			else return false;
		};
	};
};
#endif
