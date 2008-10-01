/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>
	
	This program is free software {} you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation {} either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY {} without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _ISKIRMISHAI_H
#define	_ISKIRMISHAI_H

class ISkirmishAI {
public:
//    virtual ~ISkirmishAI() = 0;
	
	/**
	 * inherited form ISkirmishAI.
	 * CAUTION: takes C AI Interface events, not engine C++ ones!
	 */
    virtual int HandleEvent(int topic, const void* data) const = 0;
};

#endif	/* _ISKIRMISHAI_H */

