/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "InMapDrawModel.h"

#include "Game/GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Net/Protocol/BaseNetProtocol.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/EventHandler.h"
#include "System/creg/STL_List.h"


CInMapDrawModel* inMapDrawerModel = nullptr;

const size_t CInMapDrawModel::DRAW_QUAD_SIZE = 32;

const float CInMapDrawModel::QUAD_SCALE = 1.0f / (DRAW_QUAD_SIZE * SQUARE_SIZE);



CInMapDrawModel::CInMapDrawModel()
	: drawQuadsX(mapDims.mapx / DRAW_QUAD_SIZE)
	, drawQuadsY(mapDims.mapy / DRAW_QUAD_SIZE)
	, drawAllMarks(false)
	, numPoints(0)
	, numLines(0)
{
	drawQuads.resize(drawQuadsX * drawQuadsY);

	for (int y = 0; y < drawQuadsY; y++) {
		for (int x = 0; x < drawQuadsX; x++) {
			drawQuads[y * drawQuadsX + x].points.reserve(16);
			drawQuads[y * drawQuadsX + x].lines.reserve(16);
		}
	}
}



bool CInMapDrawModel::MapDrawPrimitive::IsVisibleToPlayer(bool drawAllMarks) const
{
	const int allyTeam = teamHandler.AllyTeam(teamID);

	const bool alliedAB = teamHandler.Ally(allyTeam, gu->myAllyTeam);
	const bool alliedBA = teamHandler.Ally(gu->myAllyTeam, allyTeam);

	return (gu->spectating || drawAllMarks || (!spectator && alliedAB && alliedBA));
}


bool CInMapDrawModel::AllowedMsg(const CPlayer* sender) const
{
	const int  allyTeam  = teamHandler.AllyTeam(sender->team);

	const bool alliedAB = teamHandler.Ally(allyTeam, gu->myAllyTeam);
	const bool alliedBA = teamHandler.Ally(gu->myAllyTeam, allyTeam);
	const bool alliedMsg = alliedAB && alliedBA;

	// if we are playing and the guy sending the message is
	// a spectator (or not an ally), we can not just ignore
	// it due to drawAllMarks mode considerations
	return (gu->spectating || (!sender->spectator && alliedMsg));
}


bool CInMapDrawModel::AddPoint(const float3& constPos, const std::string& label, int playerID)
{
	if (!playerHandler.IsValidPlayer(playerID)) {
		return false;
	}

	// GotNetMsg() alreadys checks validity of playerID
	const CPlayer* sender = playerHandler.Player(playerID);
	const bool allowed = AllowedMsg(sender);

	float3 pos = constPos;
	pos.ClampInBounds();
	pos.y = CGround::GetHeightAboveWater(pos.x, pos.z, false) + 2.0f;

	// event clients may process the point
	// if their owner is allowed to see it
	if (allowed && eventHandler.MapDrawCmd(playerID, MAPDRAW_POINT, &pos, nullptr, &label))
		return false;


	// let the engine handle it (disallowed
	// points added here are filtered while
	// rendering the quads)
	MapPoint point(sender->spectator, sender->team, sender, pos, label);

	const int quad = int(pos.z * QUAD_SCALE) * drawQuadsX +
	                 int(pos.x * QUAD_SCALE);
	drawQuads[quad].points.push_back(point);

	numPoints++;

	return true;
}


bool CInMapDrawModel::AddLine(const float3& constPos1, const float3& constPos2, int playerID)
{
	if (!playerHandler.IsValidPlayer(playerID)) {
		return false;
	}

	const CPlayer* sender = playerHandler.Player(playerID);

	float3 pos1 = constPos1;
	float3 pos2 = constPos2;
	pos1.ClampInBounds();
	pos2.ClampInBounds();
	pos1.y = CGround::GetHeightAboveWater(pos1.x, pos1.z, false) + 2.0f;
	pos2.y = CGround::GetHeightAboveWater(pos2.x, pos2.z, false) + 2.0f;

	if (AllowedMsg(sender) && eventHandler.MapDrawCmd(playerID, MAPDRAW_LINE, &pos1, &pos2, nullptr)) {
		return false;
	}


	MapLine line(sender->spectator, sender->team, sender, pos1, pos2);

	const int quad = int(pos1.z * QUAD_SCALE) * drawQuadsX +
	                 int(pos1.x * QUAD_SCALE);
	drawQuads[quad].lines.push_back(line);

	numLines++;

	return true;
}


void CInMapDrawModel::EraseNear(const float3& constPos, int playerID)
{
	if (!playerHandler.IsValidPlayer(playerID))
		return;

	const CPlayer* sender = playerHandler.Player(playerID);

	float3 pos = constPos;
	pos.ClampInBounds();
	pos.y = CGround::GetHeightAboveWater(pos.x, pos.z, false) + 2.0f;

	if (AllowedMsg(sender) && eventHandler.MapDrawCmd(playerID, MAPDRAW_ERASE, &pos, nullptr, nullptr)) {
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

			for (auto pi = dq->points.begin(); pi != dq->points.end(); /* none */) {
				if (pi->GetPos().SqDistance2D(pos) < (radius*radius) && (pi->IsBySpectator() == sender->spectator)) {
					*pi = dq->points.back();
					dq->points.pop_back();
					numPoints--;
				} else {
					++pi;
				}
			}

			for (auto li = dq->lines.begin(); li != dq->lines.end(); /* none */) {
				// TODO maybe erase on pos2 too?
				if (li->GetPos1().SqDistance2D(pos) < (radius*radius) && (li->IsBySpectator() == sender->spectator)) {
					*li = dq->lines.back();
					dq->lines.pop_back();
					numLines--;
				} else {
					++li;
				}
			}
		}
	}
}


void CInMapDrawModel::EraseAll()
{
	for (auto& drawQuad: drawQuads) {
		drawQuad.points.clear();
		drawQuad.lines.clear();
	}
	numPoints = 0;
	numLines = 0;

	// TODO check if this is needed
	//visibleLabels.clear();
}


const CInMapDrawModel::DrawQuad* CInMapDrawModel::GetDrawQuad(int x, int y) const
{
	return &(drawQuads[(y * drawQuadsX) + x]);
}
