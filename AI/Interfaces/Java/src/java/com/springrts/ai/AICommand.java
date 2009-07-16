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
import java.util.ArrayList;
import java.util.List;

/**
 * Serves as Interface for a Java Skirmish AIs for the Spring engine.
 *
 * @author	hoijui
 * @version	0.1
 */
//public interface AICommand {}
public abstract class AICommand extends Structure/* implements Structure.ByReference*/ {

	public static enum Option {
		DONT_REPEAT     (1 << 3), //   8
		RIGHT_MOUSE_KEY (1 << 4), //  16
		SHIFT_KEY       (1 << 5), //  32
		CONTROL_KEY     (1 << 6), //  64
		ALT_KEY         (1 << 7); // 128

		private int bitValue;

		private Option(int bitValue) {
			this.bitValue = bitValue;
		}

		public static int getBitField(List<Option> options) {

			int bitField = 0;

			for (Option option : options) {
				bitField = bitField | option.bitValue;
			}

			return bitField;
		}
		public static List<Option> getOptions(int bitField) {

			List<Option> options = new ArrayList<Option>();

			for (Option option : Option.values()) {
				if ((bitField & option.bitValue) == option.bitValue) {
					options.add(option);
				}
			}

			return options;
		}
	}

	public abstract int getTopic();

//	@Override
//	public boolean getAutoRead() { return true; }
//	@Override
//	public boolean getAutoWrite() { return true; }
}
