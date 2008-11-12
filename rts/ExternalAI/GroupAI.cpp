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

#include "GroupAI.h"

#include "IAILibraryManager.h"
#include "IGroupAILibrary.h"

CGroupAI::CGroupAI(int teamId, int groupId, const SGAIKey& key)
		: teamId(teamId), groupId(groupId), key(key) {

	library = IAILibraryManager::GetInstance()->FetchGroupAILibrary(key);
	library->Init(teamId, groupId);
}

CGroupAI::~CGroupAI() {

	library->Release(teamId, groupId);
	IAILibraryManager::GetInstance()->ReleaseGroupAILibrary(key);
}

int CGroupAI::HandleEvent(int topic, const void* data) const {
	return library->HandleEvent(teamId, groupId, topic, data);
}
