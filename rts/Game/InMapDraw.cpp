/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <SDL_mouse.h>
#include <SDL_keyboard.h>

#include "InMapDraw.h"

#include "InMapDrawModel.h"
#include "GlobalUnsynced.h"
#include "ExternalAI/AILegacySupport.h" // {Point, Line}Marker
#include "Game/GameControllerTextInput.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "UI/MiniMap.h"
#include "UI/MouseHandler.h"
#include "Net/Protocol/BaseNetProtocol.h"
#include "Net/Protocol/NetProtocol.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/Net/UnpackPacket.h"
#include "System/EventHandler.h"
#include "System/EventClient.h"
#include "System/Log/ILog.h"
#include "System/Sound/ISound.h"
#include "System/Sound/ISoundChannels.h"


CInMapDraw* inMapDrawer = nullptr;

/**
 * This simply makes a noice appear when a map point is placed.
 * We will only receive an even (and thus make a sound) when we are allwoed to
 * know about it.
 */
class CNotificationPeeper : public CEventClient
{
public:
	CNotificationPeeper(): CEventClient("NotificationPeeper", 99, false)
	{
		blipSoundID = sound->GetDefSoundId("MapPoint");
	}

	bool WantsEvent(const std::string& eventName) override {
		return (eventName == "MapDrawCmd");
	}

	bool MapDrawCmd(int playerID, int type, const float3* pos0, const float3* pos1, const std::string* label) override {
		if (type == MAPDRAW_POINT) {
			const CPlayer* sender = playerHandler.Player(playerID);

			// if we happen to be in drawAll mode, notify us now
			// even if this message is not intented for our ears
			LOG("%s added point: %s", sender->name.c_str(), label->c_str());
			eventHandler.LastMessagePosition(*pos0);
			Channels::UserInterface->PlaySample(blipSoundID, *pos0);
			minimap->AddNotification(*pos0, OnesVector, 1.0f);
		}

		return false;
	}

private:
	int blipSoundID;
};


CInMapDraw::CInMapDraw()
{
	static_assert(sizeof(CNotificationPeeper) < sizeof(notificationPeeperMem), "");
	eventHandler.AddClient(new (notificationPeeperMem) CNotificationPeeper());
}

CInMapDraw::~CInMapDraw()
{
	// EC destructor calls RemoveClient
	reinterpret_cast<CNotificationPeeper*>(notificationPeeperMem)->~CNotificationPeeper();
}


void CInMapDraw::MousePress(int x, int y, int button)
{
	const float3 pos = mouse->GetWorldMapPos();
	if (pos.x < 0.0f)
		return;

	switch (button) {
		case SDL_BUTTON_LEFT: {
			if (lastLeftClickTime > (gu->gameTime - 0.3f))
				PromptLabel(pos);

			lastLeftClickTime = gu->gameTime;
		} break;
		case SDL_BUTTON_MIDDLE: {
			SendPoint(pos, "", false);
		} break;
		case SDL_BUTTON_RIGHT: {
			SendErase(pos);
		} break;
		default: {
		} break;
	}

	lastPos = pos;
}


void CInMapDraw::MouseRelease(int x, int y, int button)
{
	// TODO implement CInMapDraw::MouseRelease
}


void CInMapDraw::MouseMove(int x, int y, int dx, int dy, int button)
{
	const float3 pos = mouse->GetWorldMapPos();
	if (pos.x < 0.0f)
		return;

	if (mouse->buttons[SDL_BUTTON_LEFT].pressed && (lastDrawTime < (gu->gameTime - 0.05f))) {
		SendLine(pos, lastPos, false);
		lastDrawTime = gu->gameTime;
		lastPos = pos;
	}
	if (mouse->buttons[SDL_BUTTON_RIGHT].pressed && (lastDrawTime < (gu->gameTime - 0.05f))) {
		SendErase(pos);
		lastDrawTime = gu->gameTime;
	}

}


int CInMapDraw::GotNetMsg(std::shared_ptr<const netcode::RawPacket>& packet)
{
	int playerID = -1;

	try {
		netcode::UnpackPacket pckt(packet, 2);

		unsigned char uPlayerID;
		pckt >> uPlayerID;
		if (!playerHandler.IsValidPlayer(uPlayerID))
			throw netcode::UnpackPacketException("Invalid player number");

		playerID = uPlayerID;

		unsigned char drawType;
		pckt >> drawType;

		switch (drawType) {
			case MAPDRAW_POINT: {
				short int x, z;
				pckt >> x;
				pckt >> z;
				const float3 pos(x, 0, z);
				unsigned char fromLua;
				pckt >> fromLua;
				string label;
				pckt >> label;

				if (!fromLua || allowLuaMapDrawing)
					inMapDrawerModel->AddPoint(pos, label, playerID);
			} break;
			case MAPDRAW_LINE: {
				short int x1, z1, x2, z2;
				pckt >> x1;
				pckt >> z1;
				pckt >> x2;
				pckt >> z2;
				const float3 pos1(x1, 0, z1);
				const float3 pos2(x2, 0, z2);
				unsigned char fromLua;
				pckt >> fromLua;

				if (!fromLua || allowLuaMapDrawing)
					inMapDrawerModel->AddLine(pos1, pos2, playerID);
			} break;
			case MAPDRAW_ERASE: {
				short int x, z;
				pckt >> x;
				pckt >> z;
				float3 pos(x, 0, z);
				inMapDrawerModel->EraseNear(pos, playerID);
			} break;
		}
	} catch (const netcode::UnpackPacketException& ex) {
		LOG_L(L_WARNING, "[InMapDraw::%s] exception \"%s\" unpacking packet", __func__, ex.what());
		playerID = -1;
	}

	return playerID;
}


void CInMapDraw::SetSpecMapDrawingAllowed(bool state)
{
	LOG("[%s] spectator map-drawing is %s", __func__, (allowSpecMapDrawing = state)? "enabled": "disabled");
}

void CInMapDraw::SetLuaMapDrawingAllowed(bool state)
{
	LOG("[%s] Lua map-drawing is %s", __func__, (allowLuaMapDrawing = state)? "enabled": "disabled");
}



void CInMapDraw::SendErase(const float3& pos)
{
	if (!gu->spectating || allowSpecMapDrawing)
		clientNet->Send(CBaseNetProtocol::Get().SendMapErase(gu->myPlayerNum, (short)pos.x, (short)pos.z));
}


void CInMapDraw::SendPoint(const float3& pos, const std::string& label, bool fromLua)
{
	if (!gu->spectating || allowSpecMapDrawing)
		clientNet->Send(CBaseNetProtocol::Get().SendMapDrawPoint(gu->myPlayerNum, (short)pos.x, (short)pos.z, label, fromLua));
}

void CInMapDraw::SendLine(const float3& pos, const float3& pos2, bool fromLua)
{
	if (!gu->spectating || allowSpecMapDrawing)
		clientNet->Send(CBaseNetProtocol::Get().SendMapDrawLine(gu->myPlayerNum, (short)pos.x, (short)pos.z, (short)pos2.x, (short)pos2.z, fromLua));
}

void CInMapDraw::SendWaitingInput(const std::string& label)
{
	SendPoint(waitingPoint, label, false);

	wantLabel = false;
	drawMode = false;
}


void CInMapDraw::PromptLabel(const float3& pos)
{
	waitingPoint = pos;
	wantLabel = true;

	gameTextInput.PromptLabel();
	SetDrawMode(false);
	SDL_StartTextInput();
}


void CInMapDraw::GetPoints(std::vector<PointMarker>& points, size_t maxPoints, const std::array<int, MAX_TEAMS>& teamIDs)
{
	maxPoints = std::min(maxPoints, inMapDrawerModel->GetNumPoints());

	points.clear();
	points.reserve(maxPoints);

	const auto teamIDsBeg = teamIDs.begin();
	const auto teamIDsEnd = std::find(teamIDsBeg, teamIDs.end(), -1);

	for (size_t y = 0; (y < inMapDrawerModel->GetDrawQuadY() && points.size() < maxPoints); y++) {
		for (size_t x = 0; (x < inMapDrawerModel->GetDrawQuadX() && points.size() < maxPoints); x++) {
			const std::vector<CInMapDrawModel::MapPoint>& quadPoints = (inMapDrawerModel->GetDrawQuad(x, y))->points;

			for (auto point = quadPoints.cbegin(); (point != quadPoints.cend() && points.size() < maxPoints); ++point) {
				if (std::find(teamIDsBeg, teamIDsEnd, point->GetTeamID()) == teamIDs.end())
					continue;

				points.emplace_back();

				PointMarker& pm = points.back();
				pm.pos   = point->GetPos();
				pm.color = teamHandler.Team(point->GetTeamID())->color;
				pm.label = point->GetLabel().c_str();
			}
		}
	}
}

void CInMapDraw::GetLines(std::vector<LineMarker>& lines, size_t maxLines, const std::array<int, MAX_TEAMS>& teamIDs)
{
	maxLines = std::min(maxLines, inMapDrawerModel->GetNumLines());

	lines.clear();
	lines.reserve(maxLines);

	const auto teamIDsBeg = teamIDs.begin();
	const auto teamIDsEnd = std::find(teamIDsBeg, teamIDs.end(), -1);

	for (size_t y = 0; (y < inMapDrawerModel->GetDrawQuadY() && lines.size() < maxLines); y++) {
		for (size_t x = 0; (x < inMapDrawerModel->GetDrawQuadX() && lines.size() < maxLines); x++) {
			const std::vector<CInMapDrawModel::MapLine>& quadLines = (inMapDrawerModel->GetDrawQuad(x, y))->lines;

			for (auto line = quadLines.cbegin(); (line != quadLines.cend()) && (lines.size() < maxLines); ++line) {
				if (std::find(teamIDsBeg, teamIDsEnd, line->GetTeamID()) == teamIDs.end())
					continue;

				lines.emplace_back();

				LineMarker& lm = lines.back();
				lm.pos   = line->GetPos1();
				lm.pos2  = line->GetPos2();
				lm.color = teamHandler.Team(line->GetTeamID())->color;
			}
		}
	}
}
