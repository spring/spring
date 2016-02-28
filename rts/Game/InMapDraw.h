/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IN_MAP_DRAW_H
#define IN_MAP_DRAW_H

#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>
#include <list>

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
	int GotNetMsg(boost::shared_ptr<const netcode::RawPacket>& packet);

	void SendPoint(const float3& pos, const std::string& label, bool fromLua);
	void SendLine(const float3& pos1, const float3& pos2, bool fromLua);
	void SendErase(const float3& pos);
	void SendWaitingInput(const std::string& label);

	void PromptLabel(const float3& pos);

	void GetPoints(std::vector<PointMarker>& points, int pointsSizeMax, const std::list<int>& teamIDs);
	void GetLines(std::vector<LineMarker>& lines, int linesSizeMax, const std::list<int>& teamIDs);

	void SetDrawMode(bool drawMode) { this->drawMode = drawMode; }
	bool IsDrawMode() const { return drawMode; }

	// TODO choose a better name for these or refactor this away if possible (even better)
	void SetWantLabel(bool wantLabel) { this->wantLabel = wantLabel; }
	bool IsWantLabel() const { return wantLabel; }

	void SetSpecMapDrawingAllowed(bool state);
	bool GetSpecMapDrawingAllowed() const { return allowSpecMapDrawing; }

	void SetLuaMapDrawingAllowed(bool state);
	bool GetLuaMapDrawingAllowed() const { return allowLuaMapDrawing; }

	float3 GetMouseMapPos();

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

	float lastLeftClickTime;
	float lastDrawTime;
	float3 lastPos;
	float3 waitingPoint;

	/// whether spectators can send out MAPDRAW net-messages (synced)
	bool allowSpecMapDrawing;
	/// whether client ignores incoming Lua MAPDRAW net-messages (unsynced)
	bool allowLuaMapDrawing;

	CNotificationPeeper* notificationPeeper;
};

extern CInMapDraw* inMapDrawer;

#endif /* IN_MAP_DRAW_H */
