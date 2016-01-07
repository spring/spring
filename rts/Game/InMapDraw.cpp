/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SDL_mouse.h"
#include "SDL_keyboard.h"

#include "InMapDraw.h"

#include "InMapDrawModel.h"
#include "Camera.h"
#include "Game.h"
#include "GlobalUnsynced.h"
#include "ExternalAI/AILegacySupport.h" // {Point, Line}Marker
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "UI/MiniMap.h"
#include "UI/MouseHandler.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Net/Protocol/BaseNetProtocol.h"
#include "Net/Protocol/NetProtocol.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/Net/UnpackPacket.h"
#include "System/EventHandler.h"
#include "System/EventClient.h"
#include "System/Log/ILog.h"
#include "System/Sound/ISound.h"
#include "System/Sound/ISoundChannels.h"


CInMapDraw* inMapDrawer = NULL;

/**
 * This simply makes a noice appear when a map point is placed.
 * We will only receive an even (and thus make a sound) when we are allwoed to
 * know about it.
 */
class CNotificationPeeper : public CEventClient
{
public:
	CNotificationPeeper()
		: CEventClient("NotificationPeeper", 99, false)
	{
		blippSound = sound->GetSoundId("MapPoint");
	}

	virtual bool WantsEvent(const std::string& eventName) {
		return (eventName == "MapDrawCmd");
	}

	virtual bool MapDrawCmd(int playerID, int type, const float3* pos0, const float3* pos1, const std::string* label) {

		if (type == MAPDRAW_POINT) {
			const CPlayer* sender = playerHandler->Player(playerID);

			// if we happen to be in drawAll mode, notify us now
			// even if this message is not intented for our ears
			LOG("%s added point: %s", sender->name.c_str(), label->c_str());
			eventHandler.LastMessagePosition(*pos0);
			Channels::UserInterface->PlaySample(blippSound, *pos0);
			minimap->AddNotification(*pos0, OnesVector, 1.0f);
		}

		return false;
	}

private:
	int blippSound;
};


CInMapDraw::CInMapDraw()
	: drawMode(false)
	, wantLabel(false)
	, lastLeftClickTime(0.0f)
	, lastDrawTime(0.0f)
	, lastPos(OnesVector)
	, allowSpecMapDrawing(true)
	, allowLuaMapDrawing(true)
	, notificationPeeper(NULL)
{
	notificationPeeper = new CNotificationPeeper();
	eventHandler.AddClient(notificationPeeper);
}


CInMapDraw::~CInMapDraw()
{
	eventHandler.RemoveClient(notificationPeeper);
	delete notificationPeeper;
	notificationPeeper = NULL;
}


void CInMapDraw::MousePress(int x, int y, int button)
{
	float3 pos = GetMouseMapPos();
	if (pos.x < 0)
		return;

	switch (button) {
		case SDL_BUTTON_LEFT: {
			if (lastLeftClickTime > gu->gameTime - 0.3f) {
				PromptLabel(pos);
			}
			lastLeftClickTime = gu->gameTime;
		} break;
		case SDL_BUTTON_MIDDLE:{
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
	float3 pos = GetMouseMapPos();
	if (pos.x < 0) {
		return;
	}
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


float3 CInMapDraw::GetMouseMapPos() // TODO move to some more global place?
{
	const float dist = CGround::LineGroundCol(camera->GetPos(), camera->GetPos() + (mouse->dir * globalRendering->viewRange * 1.4f), false);
	if (dist < 0) {
		return float3(-1.0f, 1.0f, -1.0f);
	}
	float3 pos = camera->GetPos() + (mouse->dir * dist);
	pos.ClampInBounds();
	return pos;
}

int CInMapDraw::GotNetMsg(boost::shared_ptr<const netcode::RawPacket>& packet)
{
	int playerID = -1;

	try {
		netcode::UnpackPacket pckt(packet, 2);

		unsigned char uPlayerID;
		pckt >> uPlayerID;
		if (!playerHandler->IsValidPlayer(uPlayerID)) {
			throw netcode::UnpackPacketException("Invalid player number");
		}
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
				if (!fromLua || allowLuaMapDrawing) {
					inMapDrawerModel->AddPoint(pos, label, playerID);
				}
				break;
			}
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
				if (!fromLua || allowLuaMapDrawing) {
					inMapDrawerModel->AddLine(pos1, pos2, playerID);
				}
				break;
			}
			case MAPDRAW_ERASE: {
				short int x, z;
				pckt >> x;
				pckt >> z;
				float3 pos(x, 0, z);
				inMapDrawerModel->EraseNear(pos, playerID);
				break;
			}
		}
	} catch (const netcode::UnpackPacketException& ex) {
		LOG_L(L_WARNING, "Got invalid MapDraw: %s", ex.what());
		playerID = -1;
	}

	return playerID;
}


void CInMapDraw::SetSpecMapDrawingAllowed(bool state)
{
	allowSpecMapDrawing = state;
	LOG("[%s] spectator map-drawing is %s", __FUNCTION__, allowSpecMapDrawing? "enabled": "disabled");
}

void CInMapDraw::SetLuaMapDrawingAllowed(bool state)
{
	allowLuaMapDrawing = state;
	LOG("[%s] Lua map-drawing is %s", __FUNCTION__, allowLuaMapDrawing? "enabled": "disabled");
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
	game->userWriting = true;
	wantLabel = true;
	game->userPrompt = "Label: ";
	game->ignoreNextChar = true;
	inMapDrawer->SetDrawMode(false);
}


void CInMapDraw::GetPoints(std::vector<PointMarker>& points, int pointsSizeMax, const std::list<int>& teamIDs)
{
	pointsSizeMax = std::min(pointsSizeMax, inMapDrawerModel->GetNumPoints());
	points.clear();
	points.reserve(pointsSizeMax);

	const std::list<CInMapDrawModel::MapPoint>* pointsInt = NULL;

	for (size_t y = 0; (y < inMapDrawerModel->GetDrawQuadY()) && ((int)points.size() < pointsSizeMax); y++) {
		for (size_t x = 0; (x < inMapDrawerModel->GetDrawQuadX()) && ((int)points.size() < pointsSizeMax); x++) {
			pointsInt = &(inMapDrawerModel->GetDrawQuad(x, y)->points);

			for (auto point = pointsInt->cbegin(); (point != pointsInt->cend()) && ((int)points.size()  < pointsSizeMax); ++point) {
				for (auto it = teamIDs.cbegin(); it != teamIDs.cend(); ++it) {
					if (point->GetTeamID() != *it)
						continue;

					PointMarker pm;
					pm.pos   = point->GetPos();
					pm.color = teamHandler->Team(point->GetTeamID())->color;
					pm.label = point->GetLabel().c_str();
					points.push_back(pm);
					break;
				}
			}
		}
	}
}

void CInMapDraw::GetLines(std::vector<LineMarker>& lines, int linesSizeMax, const std::list<int>& teamIDs)
{
	linesSizeMax = std::min(linesSizeMax, inMapDrawerModel->GetNumLines());
	lines.clear();
	lines.reserve(linesSizeMax);

	const std::list<CInMapDrawModel::MapLine>* linesInt = NULL;

	for (size_t y = 0; (y < inMapDrawerModel->GetDrawQuadY()) && ((int)lines.size() < linesSizeMax); y++) {
		for (size_t x = 0; (x < inMapDrawerModel->GetDrawQuadX()) && ((int)lines.size() < linesSizeMax); x++) {
			linesInt = &(inMapDrawerModel->GetDrawQuad(x, y)->lines);

			for (auto line = linesInt->cbegin(); (line != linesInt->cend()) && ((int)lines.size() < linesSizeMax); ++line) {
				for (auto it = teamIDs.cbegin(); it != teamIDs.cend(); ++it) {
					if (line->GetTeamID() != *it)
						continue;

					LineMarker lm;
					lm.pos   = line->GetPos1();
					lm.pos2  = line->GetPos2();
					lm.color = teamHandler->Team(line->GetTeamID())->color;
					lines.push_back(lm);
					break;
				}
			}
		}
	}
}

