/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "InMapDrawModel.h"

#include "Player.h"
#include "Game/GlobalUnsynced.h"
#include "Game/PlayerHandler.h"
#include "Game/TeamController.h"
#include "Map/Ground.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/EventHandler.h"
#include "System/BaseNetProtocol.h"
#include "System/creg/STL_List.h"
#include "lib/gml/gmlmut.h"


CInMapDrawModel* inMapDrawerModel = NULL;

CR_BIND(CInMapDrawModel, );
CR_REG_METADATA(CInMapDrawModel, (
	CR_MEMBER(drawQuadsX),
	CR_MEMBER(drawQuadsY),
	CR_MEMBER(drawQuads),
	CR_IGNORED(drawAllMarks),
	CR_MEMBER(numPoints),
	CR_MEMBER(numLines),
	CR_POSTLOAD(PostLoad)
));

CR_BIND(CInMapDrawModel::MapDrawPrimitive, (false, -1, NULL));

CR_REG_METADATA_SUB(CInMapDrawModel, MapDrawPrimitive, (
	CR_MEMBER(spectator),
	CR_MEMBER(teamID)//,
	//CR_MEMBER(teamController)
));

CR_BIND_DERIVED(CInMapDrawModel::MapPoint, CInMapDrawModel::MapDrawPrimitive, (false, -1, NULL, ZeroVector, ""));
CR_REG_METADATA_SUB(CInMapDrawModel, MapPoint, (
	CR_MEMBER(pos),
	CR_MEMBER(label)
));

CR_BIND_DERIVED(CInMapDrawModel::MapLine, CInMapDrawModel::MapDrawPrimitive, (false, -1, NULL, ZeroVector, ZeroVector));
CR_REG_METADATA_SUB(CInMapDrawModel, MapLine, (
	CR_MEMBER(pos1),
	CR_MEMBER(pos2)
));

CR_BIND(CInMapDrawModel::DrawQuad, );
CR_REG_METADATA_SUB(CInMapDrawModel, DrawQuad, /*(
	CR_MEMBER(points),
	CR_MEMBER(lines)
)*/);



const size_t CInMapDrawModel::DRAW_QUAD_SIZE = 32;

const float CInMapDrawModel::QUAD_SCALE = 1.0f / (DRAW_QUAD_SIZE * SQUARE_SIZE);



CInMapDrawModel::CInMapDrawModel()
	: drawQuadsX(gs->mapx / DRAW_QUAD_SIZE)
	, drawQuadsY(gs->mapy / DRAW_QUAD_SIZE)
	, drawAllMarks(false)
	, numPoints(0)
	, numLines(0)
{
	drawQuads.resize(drawQuadsX * drawQuadsY);
}


CInMapDrawModel::~CInMapDrawModel()
{
}

void CInMapDrawModel::PostLoad()
{
	if (drawQuads.size() != (drawQuadsX * drawQuadsY)) {
		// For old savegames
		drawQuads.resize(drawQuadsX * drawQuadsY);
	}
}

bool CInMapDrawModel::MapDrawPrimitive::IsLocalPlayerAllowedToSee(const CInMapDrawModel* inMapDrawModel) const
{
	const int allyTeam = teamHandler->AllyTeam(teamID);
	const bool allied =
		(teamHandler->Ally(gu->myAllyTeam, allyTeam) &&
		teamHandler->Ally(allyTeam, gu->myAllyTeam));
	const bool maySee = (gu->spectating || (!spectator && allied) || inMapDrawModel->drawAllMarks);

	return maySee;
}


bool CInMapDrawModel::AllowedMsg(const CPlayer* sender) const
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


bool CInMapDrawModel::AddPoint(const float3& constPos, const std::string& label, int playerID)
{
	if (!playerHandler->IsValidPlayer(playerID)) {
		return false;
	}

	GML_STDMUTEX_LOCK(inmap); // LocalPoint

	// GotNetMsg() alreadys checks validity of playerID
	const CPlayer* sender = playerHandler->Player(playerID);
	const bool allowed = AllowedMsg(sender);

	float3 pos = constPos;
	pos.ClampInBounds();
	pos.y = ground->GetHeightAboveWater(pos.x, pos.z, false) + 2.0f;

	// event clients may process the point
	// if their owner is allowed to see it
	if (allowed && eventHandler.MapDrawCmd(playerID, MAPDRAW_POINT, &pos, NULL, &label)) {
		return false;
	}

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
	if (!playerHandler->IsValidPlayer(playerID)) {
		return false;
	}

	GML_STDMUTEX_LOCK(inmap); // LocalLine

	const CPlayer* sender = playerHandler->Player(playerID);

	float3 pos1 = constPos1;
	float3 pos2 = constPos2;
	pos1.ClampInBounds();
	pos2.ClampInBounds();
	pos1.y = ground->GetHeightAboveWater(pos1.x, pos1.z, false) + 2.0f;
	pos2.y = ground->GetHeightAboveWater(pos2.x, pos2.z, false) + 2.0f;

	if (AllowedMsg(sender) && eventHandler.MapDrawCmd(playerID, MAPDRAW_LINE, &pos1, &pos2, NULL)) {
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
	if (!playerHandler->IsValidPlayer(playerID))
		return;

	GML_STDMUTEX_LOCK(inmap); // LocalErase

	const CPlayer* sender = playerHandler->Player(playerID);

	float3 pos = constPos;
	pos.ClampInBounds();
	pos.y = ground->GetHeightAboveWater(pos.x, pos.z, false) + 2.0f;

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
					numPoints--;
				} else {
					++pi;
				}
			}
			std::list<MapLine>::iterator li;
			for (li = dq->lines.begin(); li != dq->lines.end(); /* none */) {
				// TODO maybe erase on pos2 too?
				if (li->GetPos1().SqDistance2D(pos) < (radius*radius) && (li->IsBySpectator() == sender->spectator)) {
					li = dq->lines.erase(li);
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
	for (size_t n = 0; n < drawQuads.size(); n++) {
		drawQuads[n].points.clear();
		drawQuads[n].lines.clear();
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
