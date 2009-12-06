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

package com.springrts.ai.oo;

/**
 * Common base interface for AI related Exceptions.
 *
 * @author  hoijui
 * @version 0.1
 */
public interface AIException {

	/**
	 * Returns the error associated with this Exception.
	 * This is used to send over low-level language interfaces,
	 * for example C, where exceptions are not supported.
	 * @return should be != 0, as this value is reserved for the no-error state
	 */
	public int getErrorNumber();
}
