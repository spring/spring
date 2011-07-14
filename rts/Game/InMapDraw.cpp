/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
#include "SDL_mouse.h"
#include "SDL_keyboard.h"
#include "System/mmgr.h"

#include "InMapDraw.h"
#include "InMapDrawModel.h"
#include "ExternalAI/AILegacySupport.h" // {Point, Line}Marker
#include "Game/Camera.h"
#include "Game/Game.h"
#include "GlobalUnsynced.h"
#include "Game/PlayerHandler.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/MouseHandler.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "System/Net/UnpackPacket.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/EventHandler.h"
#include "System/EventClient.h"
#include "System/BaseNetProtocol.h"
#include "System/NetProtocol.h"
#include "System/LogOutput.h"
#include "System/Sound/ISound.h"
#include "System/Sound/SoundChannels.h"


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
		blippSound = sound->GetSoundId("MapPoint", false);
	}

	virtual bool WantsEvent(const std::string& eventName) {
		return (eventName == "MapDrawCmd");
	}

	virtual bool MapDrawCmd(int playerID, int type, const float3* pos0, const float3* pos1, const std::string* label) {

		if (type == MAPDRAW_POINT) {
			const CPlayer* sender = playerHandler->Player(playerID);

			// if we happen to be in drawAll mode, notify us now
			// even if this message is not intented for our ears
			logOutput.Print("%s added point: %s", sender->name.c_str(), label->c_str());
			logOutput.SetLastMsgPos(*pos0);
			Channels::UserInterface.PlaySample(blippSound, *pos0);
			minimap->AddNotification(*pos0, float3(1.0f, 1.0f, 1.0f), 1.0f);
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
	, lastPos(float3(1.0f, 1.0f, 1.0f))
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
			break;
		}
		case SDL_BUTTON_RIGHT: {
			SendErase(pos);
			break;
		}
		case SDL_BUTTON_MIDDLE:{
			SendPoint(pos, "", false);
			break;
		}
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
	const float dist = ground->LineGroundCol(camera->pos, camera->pos + (mouse->dir * globalRendering->viewRange * 1.4f), false);
	if (dist < 0) {
		return float3(-1.0f, 1.0f, -1.0f);
	}
	float3 pos = camera->pos + (mouse->dir * dist);
	pos.CheckInBounds();
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
	} catch (const netcode::UnpackPacketException& e) {
		logOutput.Print("Got invalid MapDraw: %s", e.err.c_str());
		playerID = -1;
	}

	return playerID;
}


void CInMapDraw::SetSpecMapDrawingAllowed(bool state)
{
	allowSpecMapDrawing = state;
	logOutput.Print("Spectator map drawing is %s", allowSpecMapDrawing? "disabled": "enabled");
}

void CInMapDraw::SetLuaMapDrawingAllowed(bool state)
{
	allowLuaMapDrawing = state;
	logOutput.Print("Lua map drawing is %s", allowLuaMapDrawing? "disabled": "enabled");
}



void CInMapDraw::SendErase(const float3& pos)
{
	if (!gu->spectating || allowSpecMapDrawing)
		net->Send(CBaseNetProtocol::Get().SendMapErase(gu->myPlayerNum, (short)pos.x, (short)pos.z));
}


void CInMapDraw::SendPoint(const float3& pos, const std::string& label, bool fromLua)
{
	if (!gu->spectating || allowSpecMapDrawing)
		net->Send(CBaseNetProtocol::Get().SendMapDrawPoint(gu->myPlayerNum, (short)pos.x, (short)pos.z, label, fromLua));
}


void CInMapDraw::SendLine(const float3& pos, const float3& pos2, bool fromLua)
{
	if (!gu->spectating || allowSpecMapDrawing)
		net->Send(CBaseNetProtocol::Get().SendMapDrawLine(gu->myPlayerNum, (short)pos.x, (short)pos.z, (short)pos2.x, (short)pos2.z, fromLua));
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
	game->ignoreChar = '\xA7'; // should do something better here
}



unsigned int CInMapDraw::GetPoints(PointMarker* array, unsigned int maxPoints, const std::list<int>& teamIDs)
{
	int numPoints = 0;

	const std::list<CInMapDrawModel::MapPoint>* points = NULL;
	std::list<CInMapDrawModel::MapPoint>::const_iterator point;
	std::list<int>::const_iterator it;

	for (size_t y = 0; (y < inMapDrawerModel->GetDrawQuadY()) && (numPoints < maxPoints); y++) {
		for (size_t x = 0; (x < inMapDrawerModel->GetDrawQuadX()) && (numPoints < maxPoints); x++) {
			points = &(inMapDrawerModel->GetDrawQuad(x, y)->points);

			for (point = points->begin(); (point != points->end()) && (numPoints < maxPoints); ++point) {
				for (it = teamIDs.begin(); it != teamIDs.end(); ++it) {
					if (point->GetTeamID() == *it) {
						array[numPoints].pos   = point->GetPos();
						array[numPoints].color = teamHandler->Team(point->GetTeamID())->color;
						array[numPoints].label = point->GetLabel().c_str();
						numPoints++;
						break;
					}
				}
			}
		}
	}

	return numPoints;
}

unsigned int CInMapDraw::GetLines(LineMarker* array, unsigned int maxLines, const std::list<int>& teamIDs)
{
	unsigned int numLines = 0;

	const std::list<CInMapDrawModel::MapLine>* lines = NULL;
	std::list<CInMapDrawModel::MapLine>::const_iterator line;
	std::list<int>::const_iterator it;

	for (size_t y = 0; (y < inMapDrawerModel->GetDrawQuadY()) && (numLines < maxLines); y++) {
		for (size_t x = 0; (x < inMapDrawerModel->GetDrawQuadX()) && (numLines < maxLines); x++) {
			lines = &(inMapDrawerModel->GetDrawQuad(x, y)->lines);

			for (line = lines->begin(); (line != lines->end()) && (numLines < maxLines); ++line) {
				for (it = teamIDs.begin(); it != teamIDs.end(); ++it) {
					if (line->GetTeamID() == *it) {
						array[numLines].pos   = line->GetPos1();
						array[numLines].pos2  = line->GetPos2();
						array[numLines].color = teamHandler->Team(line->GetTeamID())->color;
						numLines++;
						break;
					}
				}
			}
		}
	}

	return numLines;
}

