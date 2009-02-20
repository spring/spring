/*
	Copyright 2008  Nicolas Wu

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

	@author Nicolas Wu
	@author Robin Vobruba <hoijui.quaero@gmail.com>
*/

#ifndef _AIAI_H
#define _AIAI_H

class IGlobalAI;

class CAIAI {
public:
	CAIAI();
	CAIAI(int team, IGlobalAI* ai);

	/**
	 * Through this function, the AI receives events from the engine.
	 * For details about events that may arrive here, see file AISEvents.h.
	 *
	 * @param	topic	unique identifyer of a message
	 *					(see EVENT_* defines in AISEvents.h)
	 * @param	data	an topic specific struct, which contains the data
	 *					associatedwith the event
	 *					(see S*Event structs in AISEvents.h)
	 * @return	ok: 0, error: != 0
	 */
	virtual int handleEvent(int topic, const void* data);

private:
	int team;
	IGlobalAI* ai;
};

#endif // _AIAI_H
