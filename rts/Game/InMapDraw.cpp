/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "SDL_mouse.h"
#include "SDL_keyboard.h"
#include "mmgr.h"

#include "InMapDraw.h"
#include "ExternalAI/AILegacySupport.h" // {Point, Line}Marker
#include "Game/Camera.h"
#include "Game/Game.h"
#include "Game/PlayerHandler.h"
#include "Game/TeamController.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/MouseHandler.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Net/UnpackPacket.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/EventHandler.h"
#include "System/BaseNetProtocol.h"
#include "System/NetProtocol.h"
#include "System/LogOutput.h"
#include "System/Sound/ISound.h"
#include "System/Sound/SoundChannels.h"
#include "System/creg/STL_List.h"


CInMapDraw* inMapDrawer = NULL;

CR_BIND(CInMapDraw, );

CR_REG_METADATA(CInMapDraw, (
	CR_MEMBER(drawQuads),
	CR_RESERVED(4)
));

/**/CR_BIND(CInMapDraw::MapDrawPrimitive, (false, -1, NULL));

CR_REG_METADATA_SUB(CInMapDraw, MapDrawPrimitive, (
	CR_MEMBER(spectator),
	CR_MEMBER(teamID),
//	CR_MEMBER(teamController), // TODO this is only left out due to lazyness to creg-ify TeamController
	CR_RESERVED(4)
));

CR_BIND(CInMapDraw::MapPoint, (false, -1, NULL, ZeroVector, ""));

CR_REG_METADATA_SUB(CInMapDraw, MapPoint, (
	CR_MEMBER(pos),
	CR_MEMBER(label),
	CR_RESERVED(4)
));

CR_BIND(CInMapDraw::MapLine, (false, -1, NULL, ZeroVector, ZeroVector));

CR_REG_METADATA_SUB(CInMapDraw, MapLine, (
	CR_MEMBER(pos1),
	CR_MEMBER(pos2),
	CR_RESERVED(4)
));

CR_BIND(CInMapDraw::DrawQuad, );

CR_REG_METADATA_SUB(CInMapDraw, DrawQuad, (
//	CR_MEMBER(points), // TODO this is only left out due to lazyness
//	CR_MEMBER(lines), // TODO this is only left out due to lazyness
	CR_RESERVED(4)
));



const size_t CInMapDraw::DRAW_QUAD_SIZE = 32;

const float CInMapDraw::QUAD_SCALE = 1.0f / (DRAW_QUAD_SIZE * SQUARE_SIZE);



CInMapDraw::CInMapDraw()
	: drawMode(false)
	, wantLabel(false)
	, drawQuadsX(gs->mapx / DRAW_QUAD_SIZE)
	, drawQuadsY(gs->mapy / DRAW_QUAD_SIZE)
	, lastLeftClickTime(0.0f)
	, lastDrawTime(0.0f)
	, drawAllMarks(false)
	, lastPos(float3(1.0f, 1.0f, 1.0f))
	, allowSpecMapDrawing(true)
	, allowLuaMapDrawing(true)
{
	drawQuads.resize(drawQuadsX * drawQuadsY);

	blippSound = sound->GetSoundId("MapPoint", false);
}


CInMapDraw::~CInMapDraw()
{
}

void CInMapDraw::PostLoad()
{
	if (drawQuads.size() != (drawQuadsX * drawQuadsY)) {
		// For old savegames
		drawQuads.resize(drawQuadsX * drawQuadsY);
	}
}

bool CInMapDraw::MapDrawPrimitive::IsLocalPlayerAllowedToSee(const CInMapDraw* inMapDraw) const
{
	const int allyTeam = teamHandler->AllyTeam(teamID);
	const bool allied =
		(teamHandler->Ally(gu->myAllyTeam, allyTeam) &&
		teamHandler->Ally(allyTeam, gu->myAllyTeam));
	const bool maySee = (gu->spectating || (!spectator && allied) || inMapDraw->drawAllMarks);

	return maySee;
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
	//TODO
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


float3 CInMapDraw::GetMouseMapPos()
{
	const float dist = ground->LineGroundCol(camera->pos, camera->pos + (mouse->dir * globalRendering->viewRange * 1.4f));
	if (dist < 0) {
		return float3(-1.0f, 1.0f, -1.0f);
	}
	float3 pos = camera->pos + (mouse->dir * dist);
	pos.CheckInBounds();
	return pos;
}



bool CInMapDraw::AllowedMsg(const CPlayer* sender) const
{
	const int  team      = sender->team;
	const int  allyteam  = teamHandler->AllyTeam(team);
	const bool alliedMsg = (teamHandler->Ally(gu->myAllyTeam, allyteam) &&
	                        teamHandler->Ally(allyteam, gu->myAllyTeam));

	if (!gu->spectating && (sender->spectator || !alliedMsg)) {
		// we are playing and the guy sending the
		// net-msg is a spectator or not an ally;
		// cannot just ignore the message due to
		// drawAllMarks mode considerations
		return false;
	}

	return true;
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
					LocalPoint(pos, label, playerID);
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
					LocalLine(pos1, pos2, playerID);
				}
				break;
			}
			case MAPDRAW_ERASE: {
				short int x, z;
				pckt >> x;
				pckt >> z;
				float3 pos(x, 0, z);
				LocalErase(pos, playerID);
				break;
			}
		}
	} catch (const netcode::UnpackPacketException& e) {
		logOutput.Print("Got invalid MapDraw: %s", e.err.c_str());
		playerID = -1;
	}

	return playerID;
}

void CInMapDraw::LocalPoint(const float3& constPos, const std::string& label, int playerID)
{
	if (!playerHandler->IsValidPlayer(playerID))
		return;

	GML_STDMUTEX_LOCK(inmap); // LocalPoint

	// GotNetMsg() alreadys checks validity of playerID
	const CPlayer* sender = playerHandler->Player(playerID);
	const bool allowed = AllowedMsg(sender);

	float3 pos = constPos;
	pos.CheckInBounds();
	pos.y = ground->GetHeightAboveWater(pos.x, pos.z) + 2.0f;

	// event clients may process the point
	// if their owner is allowed to see it
	if (allowed && eventHandler.MapDrawCmd(playerID, MAPDRAW_POINT, &pos, NULL, &label)) {
		return;
	}

	// let the engine handle it (disallowed
	// points added here are filtered while
	// rendering the quads)
	MapPoint point(sender->spectator, sender->team, sender, pos, label);

	const int quad = int(pos.z * QUAD_SCALE) * drawQuadsX +
	                 int(pos.x * QUAD_SCALE);
	drawQuads[quad].points.push_back(point);


	if (allowed || drawAllMarks) {
		// if we happen to be in drawAll mode, notify us now
		// even if this message is not intented for our ears
		logOutput.Print("%s added point: %s",
		                sender->name.c_str(), point.GetLabel().c_str());
		logOutput.SetLastMsgPos(pos);
		Channels::UserInterface.PlaySample(blippSound, pos);
		minimap->AddNotification(pos, float3(1.0f, 1.0f, 1.0f), 1.0f);
	}
}


void CInMapDraw::LocalLine(const float3& constPos1, const float3& constPos2, int playerID)
{
	if (!playerHandler->IsValidPlayer(playerID))
		return;

	GML_STDMUTEX_LOCK(inmap); // LocalLine

	const CPlayer* sender = playerHandler->Player(playerID);

	float3 pos1 = constPos1;
	float3 pos2 = constPos2;
	pos1.CheckInBounds();
	pos2.CheckInBounds();
	pos1.y = ground->GetHeightAboveWater(pos1.x, pos1.z) + 2.0f;
	pos2.y = ground->GetHeightAboveWater(pos2.x, pos2.z) + 2.0f;

	if (AllowedMsg(sender) && eventHandler.MapDrawCmd(playerID, MAPDRAW_LINE, &pos1, &pos2, NULL)) {
		return;
	}

	MapLine line(sender->spectator, sender->team, sender, pos1, pos2);

	const int quad = int(pos1.z * QUAD_SCALE) * drawQuadsX +
	                 int(pos1.x * QUAD_SCALE);
	drawQuads[quad].lines.push_back(line);
}


void CInMapDraw::LocalErase(const float3& constPos, int playerID)
{
	if (!playerHandler->IsValidPlayer(playerID))
		return;

	GML_STDMUTEX_LOCK(inmap); // LocalErase

	const CPlayer* sender = playerHandler->Player(playerID);

	float3 pos = constPos;
	pos.CheckInBounds();
	pos.y = ground->GetHeightAboveWater(pos.x, pos.z) + 2.0f;

	if (AllowedMsg(sender) && eventHandler.MapDrawCmd(playerID, MAPDRAW_ERASE, &pos, NULL, NULL)) {
		return;
	}

	const float radius = 100.0f;
	const int maxY = drawQuadsY - 1;
	const int maxX = drawQuadsX - 1;
	const int yStart = (int) std::max(0,    int((pos.z - radius) * QUAD_SCALE));
	const int xStart = (int) std::max(0,    int((pos.x - radius) * QUAD_SCALE));
	const int yEnd   = (int) std::min(maxY, int((pos.z + radius) * QUAD_SCALE));
	const int xEnd   = (int) std::min(maxX, int((pos.x + radius) * QUAD_SCALE));

	for (int y = yStart; y <= yEnd; ++y) {
		for (int x = xStart; x <= xEnd; ++x) {
			DrawQuad* dq = &drawQuads[(y * drawQuadsX) + x];

			std::list<MapPoint>::iterator pi;
			for (pi = dq->points.begin(); pi != dq->points.end(); /* none */) {
				if (pi->GetPos().SqDistance2D(pos) < (radius*radius) && (pi->IsBySpectator() == sender->spectator)) {
					pi = dq->points.erase(pi);
				} else {
					++pi;
				}
			}
			std::list<MapLine>::iterator li;
			for (li = dq->lines.begin(); li != dq->lines.end(); /* none */) {
				// TODO maybe erase on pos2 too?
				if (li->GetPos1().SqDistance2D(pos) < (radius*radius) && (li->IsBySpectator() == sender->spectator)) {
					li = dq->lines.erase(li);
				} else {
					++li;
				}
			}
		}
	}
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



unsigned int CInMapDraw::GetPoints(PointMarker* array, unsigned int maxPoints, const std::list<int>& teamIDs)
{
	int numPoints = 0;

	std::list<MapPoint>* points = NULL;
	std::list<MapPoint>::const_iterator point;
	std::list<int>::const_iterator it;

	for (size_t i = 0; (i < (drawQuadsX * drawQuadsY)) && (numPoints < maxPoints); i++) {
		points = &(drawQuads[i].points);

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

	return numPoints;
}

unsigned int CInMapDraw::GetLines(LineMarker* array, unsigned int maxLines, const std::list<int>& teamIDs)
{
	unsigned int numLines = 0;

	std::list<MapLine>* lines = NULL;
	std::list<MapLine>::const_iterator line;
	std::list<int>::const_iterator it;

	for (size_t i = 0; (i < (drawQuadsX * drawQuadsY)) && (numLines < maxLines); i++) {
		lines = &(drawQuads[i].lines);

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

	return numLines;
}


void CInMapDraw::ClearMarks()
{
	for (unsigned int n = 0; n < drawQuads.size(); n++) {
		drawQuads[n].points.clear();
		drawQuads[n].lines.clear();
	}

	// TODO check if this is needed
	//visibleLabels.clear();
}


const CInMapDraw::DrawQuad* CInMapDraw::GetDrawQuad(int x, int y) const
{
	return &(drawQuads[(y * drawQuadsX) + x]);
}
