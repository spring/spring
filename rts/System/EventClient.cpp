/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include "EventClient.h"

/******************************************************************************/
/******************************************************************************/

CEventClient::CEventClient(const std::string& _name, int _order, bool _synced)
	: name(_name)
	, order(_order)
	, synced(_synced)
{
}

/******************************************************************************/
/******************************************************************************/
//
//  Unsynced
//

void CEventClient::Save(zipFile archive) {}

void CEventClient::Update() {}

void CEventClient::ViewResize() {}

bool CEventClient::DefaultCommand(const CUnit* unit, const CFeature* feature, int& cmd) { return false; }

void CEventClient::DrawGenesis() {}
void CEventClient::DrawWorld() {}
void CEventClient::DrawWorldPreUnit() {}
void CEventClient::DrawWorldShadow() {}
void CEventClient::DrawWorldReflection() {}
void CEventClient::DrawWorldRefraction() {}
void CEventClient::DrawScreenEffects() {}
void CEventClient::DrawScreen() {}
void CEventClient::DrawInMiniMap() {}

// from LuaUI
bool CEventClient::KeyPress(unsigned short key, bool isRepeat) { return false; }
bool CEventClient::KeyRelease(unsigned short key) { return false; }
bool CEventClient::MouseMove(int x, int y, int dx, int dy, int button) { return false; }
bool CEventClient::MousePress(int x, int y, int button) { return false; }
int  CEventClient::MouseRelease(int x, int y, int button) { return -1; } // FIXME - bool / void?
bool CEventClient::MouseWheel(bool up, float value) { return false; }
bool CEventClient::JoystickEvent(const std::string& event, int val1, int val2) { return false; }
bool CEventClient::IsAbove(int x, int y) { return false; }
std::string CEventClient::GetTooltip(int x, int y) { return ""; }

bool CEventClient::CommandNotify(const Command& cmd) { return false; }

bool CEventClient::AddConsoleLine(const std::string& msg, const CLogSubsystem& subsystem) { return false; }

bool CEventClient::GroupChanged(int groupID) { return false; }

bool CEventClient::GameSetup(const std::string& state, bool& ready,
                             const map<int, std::string>& playerStates) { return false; }

std::string CEventClient::WorldTooltip(const CUnit* unit,
                                 const CFeature* feature,
                                 const float3* groundPos) { return ""; }

bool CEventClient::MapDrawCmd(int playerID, int type,
                        const float3* pos0,
                        const float3* pos1,
                        const std::string* label) { return false; }

/******************************************************************************/
/******************************************************************************/
