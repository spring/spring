/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef INMAPDRAW_H
#define INMAPDRAW_H

#include <string>
#include <vector>
#include <list>

#include "System/float3.h"
#include "System/creg/creg.h"
#include "System/Net/RawPacket.h"

class CPlayer;
struct PointMarker;
struct LineMarker;

class CInMapDraw
{
	CR_DECLARE(CInMapDraw);
	CR_DECLARE_SUB(MapPoint);
	CR_DECLARE_SUB(MapLine);
	CR_DECLARE_SUB(DrawQuad);

public:
	CInMapDraw(void);
	~CInMapDraw(void);
	void PostLoad();

	void Draw(void);

	void MousePress(int x, int y, int button);
	void MouseRelease(int x,int y,int button);
	void MouseMove(int x, int y, int dx, int dy, int button);
	bool AllowedMsg(const CPlayer*) const;
	/** @return playerId */
	int GotNetMsg(boost::shared_ptr<const netcode::RawPacket> &packet);
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

	unsigned int GetPoints(PointMarker* array, unsigned int maxPoints, const std::list<const unsigned char*>& colors);
	unsigned int GetLines(LineMarker* array, unsigned int maxLines, const std::list<const unsigned char*>& colors);

	void SetSpecMapDrawingAllowed(bool state);
	void SetLuaMapDrawingAllowed(bool state);
	bool GetSpecMapDrawingAllowed() const { return allowSpecMapDrawing; }
	bool GetLuaMapDrawingAllowed() const { return allowLuaMapDrawing; }

	float3 GetMouseMapPos(void);

	struct MapPoint {
		CR_DECLARE_STRUCT(MapPoint);
		bool MaySee(CInMapDraw*) const;

		float3 pos;
		const unsigned char* color;
		std::string label;

		int senderAllyTeam;
		bool senderSpectator;
	};

	struct MapLine {
		CR_DECLARE_STRUCT(MapLine);
		bool MaySee(CInMapDraw*) const;

		float3 pos;
		float3 pos2;
		const unsigned char* color;

		int senderAllyTeam;
		bool senderSpectator;
	};

	struct DrawQuad {
		CR_DECLARE_STRUCT(DrawQuad);
		std::list<MapPoint> points;
		std::list<MapLine> lines;
	};

	std::vector<DrawQuad> drawQuads;

	int drawQuadsX;
	int drawQuadsY;

	bool keyPressed;
	bool wantLabel;

private:
	float lastLeftClickTime;
	float lastDrawTime;
	bool drawAllMarks;
	float3 lastPos;
	float3 waitingPoint;

	unsigned int texture;
	int blippSound;

	std::vector<MapPoint*> visibleLabels;

	bool allowSpecMapDrawing; //! if true, spectators can send out MAPDRAW net-messages (synced)
	bool allowLuaMapDrawing;  //! if true, client ignores incoming Lua MAPDRAW net-messages (unsynced)
};

extern CInMapDraw* inMapDrawer;

#endif /* INMAPDRAW_H */
