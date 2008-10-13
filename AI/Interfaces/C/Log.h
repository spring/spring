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

#ifndef _LOG_H
#define	_LOG_H

#ifdef	__cplusplus
extern "C" {
#endif

#define EXTERNAL_LOGGER(msg)	log(msg);

/**
 * Initializes the log.
 */
void simpleLog_init(const char* logFileName, bool useTimeStamps);

/**
 * Logs a text message. Works like printf(fmt, ...)
 */
void simpleLog_log(const char* fmt, ...);

/**
 * Logs a text message and exits. Works like printf(fmt, ...)
 */
void simpleLog_error(int error, const char* msg, ...);

#ifdef	__cplusplus
}
#endif

#endif	/* _LOG_H */

