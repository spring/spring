/*
	Copyright (c) 2009 Robin Vobruba <hoijui.quaero@gmail.com>

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


import java.awt.Color;

/**
 * Contains util functions used within all of the Java AI Interface code.
 *
 * @author	hoijui.quaero@gmail.com
 * @version	0.1
 */
public final class Util {

	/** We need no instances of this class */
	private Util() {}

	public static short[] toShort3Array(final Color color) {

		short[] shortArr = new short[3];

		shortArr[0] = (short) color.getRed();
		shortArr[1] = (short) color.getGreen();
		shortArr[2] = (short) color.getBlue();

		return shortArr;
	}

	public static Color toColor(final short[] shortArr) {

		Color color = new Color(
				(int) shortArr[0],
				(int) shortArr[1],
				(int) shortArr[2]
				);

		return color;
	}
}
