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
 * An exception of this type may be thrown while handling an AI event.
 *
 * @author  hoijui
 * @version 0.1
 */
public class EventAIException extends Exception implements AIException {

	public static final int DEFAULT_ERROR_NUMBER = 10;

	private int errorNumber;

	public EventAIException() {
		super();

		this.errorNumber = DEFAULT_ERROR_NUMBER;
	}
	public EventAIException(int errorNumber) {
		super();

		this.errorNumber = errorNumber;
	}

	public EventAIException(String message) {
		super(message);

		this.errorNumber = DEFAULT_ERROR_NUMBER;
	}
	public EventAIException(String message, int errorNumber) {
		super(message);

		this.errorNumber = errorNumber;
	}

	public EventAIException(String message, Throwable cause) {
		super(message, cause);

		this.errorNumber = DEFAULT_ERROR_NUMBER;
	}
	public EventAIException(String message, Throwable cause, int errorNumber) {
		super(message, cause);

		this.errorNumber = errorNumber;
	}

	public EventAIException(Throwable cause) {
		super(cause);

		this.errorNumber = DEFAULT_ERROR_NUMBER;
	}
	public EventAIException(Throwable cause, int errorNumber) {
		super(cause);

		this.errorNumber = errorNumber;
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
