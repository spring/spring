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

#include "SkirmishAI.h"

#include "IAILibraryManager.h"
#include "ISkirmishAILibrary.h"
#include "TimeProfiler.h"
#include "Util.h"

CSkirmishAI::CSkirmishAI(int teamId, const SkirmishAIKey& key,
		const SSkirmishAICallback* c_callback) :
		teamId(teamId),
		key(key),
		timerName((std::string("SkirmishAI:") +
		          key.GetShortName() + "-" + key.GetVersion() +
		          ":" + IntToString(teamId)).c_str())
{
	SCOPED_TIMER(timerName);
	library = IAILibraryManager::GetInstance()->FetchSkirmishAILibrary(key);
	library->Init(teamId, c_callback);
}

CSkirmishAI::~CSkirmishAI() {

	SCOPED_TIMER(timerName);
	library->Release(teamId);
	IAILibraryManager::GetInstance()->ReleaseSkirmishAILibrary(key);
}

int CSkirmishAI::HandleEvent(int topic, const void* data) const {

	SCOPED_TIMER(timerName);
	return library->HandleEvent(teamId, topic, data);
}
