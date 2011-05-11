/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IN_MAP_DRAW_H
#define IN_MAP_DRAW_H

#include <string>
#include <vector>
#include <list>

#include "System/float3.h"
#include "System/creg/creg.h"
#include "System/Net/RawPacket.h"

class CPlayer;
struct PointMarker;
struct LineMarker;
struct TeamController;

/**
 * The M & C in MVC for InMapDraw.
 * @see CInMapDraw for V
 */
class CInMapDraw
{
	CR_DECLARE(CInMapDraw);
	CR_DECLARE_SUB(MapDrawPrimitive);
	CR_DECLARE_SUB(MapPoint);
	CR_DECLARE_SUB(MapLine);
	CR_DECLARE_SUB(DrawQuad);

public:
	static const size_t DRAW_QUAD_SIZE;
	static const float QUAD_SCALE;

	CInMapDraw();
	~CInMapDraw();

	void PostLoad();

	void MousePress(int x, int y, int button);
	void MouseRelease(int x, int y, int button);
	void MouseMove(int x, int y, int dx, int dy, int button);
	bool AllowedMsg(const CPlayer* sender) const;
	/** @return playerId */
	int GotNetMsg(boost::shared_ptr<const netcode::RawPacket>& packet);
	void ToggleAllMarksVisible() { drawAllMarks = !drawAllMarks; }
	void SetAllMarksVisible(bool b) { drawAllMarks = b; }
	void ClearMarks();

	void LocalPoint(const float3& pos, const std::string& label, int playerID);
	void LocalLine(const float3& pos1, const float3& pos2, int playerID);
	void LocalErase(const float3& pos, int playerID);

	void SendPoint(const float3& pos, const std::string& label, bool fromLua);
	void SendLine(const float3& pos1, const float3& pos2, bool fromLua);
	void SendErase(const float3& pos);
	void SendWaitingInput(const std::string& label);

	void PromptLabel(const float3& pos);

	unsigned int GetPoints(PointMarker* array, unsigned int maxPoints, const std::list<int>& teamIDs);
	unsigned int GetLines(LineMarker* array, unsigned int maxLines, const std::list<int>& teamIDs);

	void SetSpecMapDrawingAllowed(bool state);
	void SetLuaMapDrawingAllowed(bool state);
	bool GetSpecMapDrawingAllowed() const { return allowSpecMapDrawing; }
	bool GetLuaMapDrawingAllowed() const { return allowLuaMapDrawing; }

	void SetDrawMode(bool drawMode) { this->drawMode = drawMode; }
	bool IsDrawMode() const { return drawMode; }

	// TODO choose a better name for these or refactor this away if possible (even better)
	void SetWantLabel(bool wantLabel) { this->wantLabel = wantLabel; }
	bool IsWantLabel() const { return wantLabel; }

	float3 GetMouseMapPos();


	struct MapDrawPrimitive {
		CR_DECLARE_STRUCT(MapDrawPrimitive);

	public:
		MapDrawPrimitive(bool spectator, int teamID, const TeamController* teamController)
			: spectator(spectator)
			, teamID(teamID)
			, teamController(teamController)
		{}

		bool IsLocalPlayerAllowedToSee(const CInMapDraw* inMapDraw) const;

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
		CR_DECLARE_STRUCT(MapPoint);

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
		CR_DECLARE_STRUCT(MapLine);

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
		std::list<CInMapDraw::MapPoint> points;
		std::list<CInMapDraw::MapLine> lines;
	};

	const DrawQuad* GetDrawQuad(int x, int y) const;

private:
	/**
	 * Whether we are in draw mode.
	 * This is true for example, when the draw-mode-key is currently held down.
	 */
	bool drawMode;
	/**
	 * Whether the point currently beeing drawn is one with label or not??? TODO check this
	 */
	bool wantLabel;

	int drawQuadsX;
	int drawQuadsY;
	std::vector<DrawQuad> drawQuads;

	float lastLeftClickTime;
	float lastDrawTime;
	bool drawAllMarks;
	float3 lastPos;
	float3 waitingPoint;

	/// whether spectators can send out MAPDRAW net-messages (synced)
	bool allowSpecMapDrawing; 
	/// whether client ignores incoming Lua MAPDRAW net-messages (unsynced)
	bool allowLuaMapDrawing;

	int blippSound;
};

extern CInMapDraw* inMapDrawer;

#endif /* IN_MAP_DRAW_H */
