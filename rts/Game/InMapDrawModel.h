/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IN_MAP_DRAW_MODEL_H
#define IN_MAP_DRAW_MODEL_H

#include <string>
#include <vector>
#include <list>

#include "System/float3.h"
#include "System/creg/creg_cond.h"

class CPlayer;
class TeamController;

/**
 * The M in MVC for InMapDraw.
 * @see CInMapDrawView for V
 * @see CInMapDraw for C
 */
class CInMapDrawModel
{
	CR_DECLARE_STRUCT(CInMapDrawModel);
	CR_DECLARE_SUB(MapDrawPrimitive);
	CR_DECLARE_SUB(MapPoint);
	CR_DECLARE_SUB(MapLine);
	CR_DECLARE_SUB(DrawQuad);

public:
	static const size_t DRAW_QUAD_SIZE;
	static const float QUAD_SCALE;

	CInMapDrawModel();
	~CInMapDrawModel();

	void PostLoad();

	void SetAllMarksVisible(bool newState) { drawAllMarks = newState; }
	bool GetAllMarksVisible() const { return drawAllMarks; }

	bool AddPoint(const float3& pos, const std::string& label, int playerID);
	bool AddLine(const float3& pos1, const float3& pos2, int playerID);
	void EraseNear(const float3& pos, int playerID);
	void EraseAll();

	int GetNumPoints() const { return numPoints; }
	int GetNumLines() const { return numLines; }


	struct MapDrawPrimitive {
		CR_DECLARE(MapDrawPrimitive);

	public:
		MapDrawPrimitive(bool spectator, int teamID, const TeamController* teamController)
			: spectator(spectator)
			, teamID(teamID)
			, teamController(teamController)
		{}

		bool IsLocalPlayerAllowedToSee(const CInMapDrawModel* inMapDraw) const;

		/**
		 * Was the creator of this map-drawing spectator at the time it was
		 * created?
		 * @see #GetTeamController
		 */
		bool IsBySpectator() const { return spectator; }
		/**
		 * The team-id of the creator of this map-drawing at the time of
		 * creation.
		 * @see #GetTeamController
		 */
		int GetTeamID() const { return teamID; }
		/**
		 * The team-controller that created this map-drawing.
		 */
		const TeamController* GetTeamController() const { return teamController; }

	private:
		bool spectator;
		int teamID;
		const TeamController* teamController;
	};

	struct MapPoint : public MapDrawPrimitive {
		CR_DECLARE(MapPoint);

	public:
		MapPoint(bool spectator, int teamID, const TeamController* teamController, const float3& pos, const std::string& label)
			: MapDrawPrimitive(spectator, teamID, teamController)
			, pos(pos)
			, label(label)
		{}

		const float3& GetPos() const { return pos; }
		const std::string& GetLabel() const { return label; }

	private:
		float3 pos;
		std::string label;
	};

	struct MapLine : public MapDrawPrimitive {
		CR_DECLARE(MapLine);

	public:
		MapLine(bool spectator, int teamID, const TeamController* teamController, const float3& pos1, const float3& pos2)
			: MapDrawPrimitive(spectator, teamID, teamController)
			, pos1(pos1)
			, pos2(pos2)
		{}

		/**
		 * The start position of the line.
		 */
		const float3& GetPos1() const { return pos1; }
		/**
		 * The end position of the line.
		 */
		const float3& GetPos2() const { return pos2; }

	private:
		float3 pos1;
		float3 pos2;
	};

	/**
	 * This is meant to be a QuadTree implementation, but in reality it is a
	 * cell of a grid structure.
	 */
	struct DrawQuad {
		CR_DECLARE_STRUCT(DrawQuad);
		std::list<CInMapDrawModel::MapPoint> points;
		std::list<CInMapDrawModel::MapLine> lines;
	};

	int GetDrawQuadX() const { return drawQuadsX; }
	int GetDrawQuadY() const { return drawQuadsY; }
	const DrawQuad* GetDrawQuad(int x, int y) const;

private:
	bool AllowedMsg(const CPlayer* sender) const;

	int drawQuadsX;
	int drawQuadsY;
	std::vector<DrawQuad> drawQuads;

	bool drawAllMarks;

	/// total number of points
	int numPoints;
	/// total number of lines
	int numLines;
};

extern CInMapDrawModel* inMapDrawerModel;

#endif /* IN_MAP_DRAW_MODEL_H */
