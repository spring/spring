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

#ifndef _CPPTESTAI_CPPTESTAI_H
#define _CPPTESTAI_CPPTESTAI_H

#include "Cpp/AICallback.h"

namespace cpptestai {

/**
 * This is the main C++ entry point of this AI.
 * 
 * @author	Robin Vobruba <hoijui.quaero@gmail.com>
 */
class CCppTestAI {

private:
	int teamId;
	const struct SSkirmishAICallback* innerCallback;

public:
	CCppTestAI(int teamId, const struct SSkirmishAICallback* innerCallback);
	~CCppTestAI();

	int HandleEvent(int topic, const void* data);
}; // class CCppTestAI

} // namespace cpptestai

#endif // _CPPTESTAI_CPPTESTAI_H

