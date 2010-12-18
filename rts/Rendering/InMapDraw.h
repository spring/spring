/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef INMAPDRAW_H
#define INMAPDRAW_H

#include <string>
#include <vector>
#include <list>

#include "GL/myGL.h"

#include "Net/RawPacket.h"

class CPlayer;

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
	void ToggleAllVisible() { drawAll = !drawAll; }
	void SetAllVisible(bool b) { drawAll = b; }

	void LocalPoint(const float3& pos, const std::string& label, int playerID);
	void LocalLine(const float3& pos1, const float3& pos2, int playerID);
	void LocalErase(const float3& pos, int playerID);

	void SendPoint(const float3& pos, const std::string& label, bool fromLua);
	void SendLine(const float3& pos1, const float3& pos2, bool fromLua);
	void SendErase(const float3& pos);

	bool keyPressed;
	float lastLeftClickTime;
	float lastDrawTime;
	float3 lastPos;
	bool wantLabel;
	float3 waitingPoint;

	bool drawAll;

	void PromptLabel(const float3& pos);
	
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
	size_t numQuads;

private:
	GLuint texture;
	int blippSound;

	std::vector<MapPoint*> visibleLabels;

protected:
	bool allowSpecMapDrawing; //! if true, spectators can send out MAPDRAW net-messages (synced)
	bool allowLuaMapDrawing;  //! if true, client ignores incoming Lua MAPDRAW net-messages (unsynced)
};

extern CInMapDraw* inMapDrawer;

#endif /* INMAPDRAW_H */
