/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

package com.springrts.ai;


import com.sun.jna.Structure;

/**
 * An AI event is sent form the engine to Java Skirmish AIs.
 *
 * @author	hoijui
 * @version	0.1
 */
public abstract class AIEvent extends Structure/* implements Structure.ByReference*/ {

	protected AIEvent() {};
	protected AIEvent(int size) {
		super(size);
	}

	public abstract int getTopic();
}
