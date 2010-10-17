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
 * An exception of this type may be thrown while from an AI callback method.
 *
 * @author  hoijui
 */
public class CallbackAIException extends RuntimeException implements AIException {

	private String methodName;
	private int    errorNumber;

	public CallbackAIException(String methodName, int errorNumber) {
		super("Error calling method \"" + methodName + "\": " + errorNumber);

		this.methodName  = methodName;
		this.errorNumber = errorNumber;
	}
	public CallbackAIException(String methodName, int errorNumber, Throwable cause) {
		super("Error calling method \"" + methodName + "\": " + errorNumber, cause);

		this.methodName  = methodName;
		this.errorNumber = errorNumber;
	}

	/**
	 * Returns the name of the method in which the exception ocurred.
	 */
	public String getMethodName() {
		return methodName;
	}

	/**
	 * Returns the error number that will be sent to the engine,
	 * and consequently appear in the engines main log file.
	 * @return should be != 0, as this value is reserved for the no-error state
	 */
	 @Override
	public int getErrorNumber() {
		return errorNumber;
	}
}
