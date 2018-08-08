/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IN_MAP_DRAW_H
#define IN_MAP_DRAW_H

#include <string>
#include <array>
#include <vector>

#include "Sim/Misc/GlobalConstants.h"
#include "System/float3.h"
#include "System/Net/RawPacket.h"

class CPlayer;
class CNotificationPeeper;
struct PointMarker;
struct LineMarker;

/**
 * The C in MVC for InMapDraw.
 * @see CInMapDrawModel for M
 * @see CInMapDrawView for V
 */
class CInMapDraw
{
public:
	CInMapDraw();
	~CInMapDraw();

	void MousePress(int x, int y, int button);
	void MouseRelease(int x, int y, int button);
	void MouseMove(int x, int y, int dx, int dy, int button);
	/** @return playerId */
	int GotNetMsg(std::shared_ptr<const netcode::RawPacket>& packet);

	void SendPoint(const float3& pos, const std::string& label, bool fromLua);
	void SendLine(const float3& pos1, const float3& pos2, bool fromLua);
	void SendErase(const float3& pos);
	void SendWaitingInput(const std::string& label);

	void PromptLabel(const float3& pos);

	void GetPoints(std::vector<PointMarker>& points, size_t maxPoints, const std::array<int, MAX_TEAMS>& teamIDs);
	void GetLines(std::vector<LineMarker>& lines, size_t maxLines, const std::array<int, MAX_TEAMS>& teamIDs);

	void SetDrawMode(bool drawMode) { this->drawMode = drawMode; }
	bool IsDrawMode() const { return drawMode; }

	// TODO choose a better name for these or refactor this away if possible (even better)
	void SetWantLabel(bool wantLabel) { this->wantLabel = wantLabel; }
	bool IsWantLabel() const { return wantLabel; }

	void SetSpecMapDrawingAllowed(bool state);
	bool GetSpecMapDrawingAllowed() const { return allowSpecMapDrawing; }

	void SetLuaMapDrawingAllowed(bool state);
	bool GetLuaMapDrawingAllowed() const { return allowLuaMapDrawing; }

private:
	float lastLeftClickTime = 0.0f;
	float lastDrawTime = 0.0f;

	float3 lastPos = OnesVector;
	float3 waitingPoint;

	/**
	 * Whether we are in draw mode.
	 * This is true for example, when the draw-mode-key is currently held down.
	 */
	bool drawMode = false;
	/**
	 * Whether the point currently beeing drawn is one with label or not??? TODO check this
	 */
	bool wantLabel = false;

	/// whether spectators can send out MAPDRAW net-messages (synced)
	bool allowSpecMapDrawing = true;
	/// whether client ignores incoming Lua MAPDRAW net-messages (unsynced)
	bool allowLuaMapDrawing = true;

	uint8_t notificationPeeperMem[96];
};

extern CInMapDraw* inMapDrawer;

#endif /* IN_MAP_DRAW_H */
