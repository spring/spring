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

#ifndef _SKIRMISHAI_H
#define _SKIRMISHAI_H

#include "ISkirmishAI.h"
#include "SkirmishAIKey.h"

class ISkirmishAILibrary;

/**
 * The default implementation of ISkirmishAI.
 */
class CSkirmishAI : public ISkirmishAI {
public:
	CSkirmishAI(int teamId, const SkirmishAIKey& skirmishAIKey);
	virtual ~CSkirmishAI();

	virtual int HandleEvent(int topic, const void* data) const;

private:
	int teamId;
	const SkirmishAIKey key;
	const ISkirmishAILibrary* library;
};

#endif // _SKIRMISHAI_H
