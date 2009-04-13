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

#include "CppTestAI.h"

#include "ExternalAI/Interface/AISEvents.h"

cpptestai::CCppTestAI::CCppTestAI(
		int teamId, const struct SSkirmishAICallback* innerCallback) :
		teamId(teamId), innerCallback(innerCallback) {}
		
cpptestai::CCppTestAI::~CCppTestAI() {}

int cpptestai::CCppTestAI::HandleEvent(int topic, const void* data) {

	switch (topic) {
		case EVENT_UNIT_CREATED: {
			struct SUnitCreatedEvent* evt = (struct SUnitCreatedEvent*) data;
			int unitId = evt->unit;
			innerCallback->Clb_Unit_0SINGLE1FETCH2UnitDef0getDef(teamId, unitId);
			break;
		}
		default: {
			break;
		}
	}

	// signal: everything went OK
	return 0;
}
