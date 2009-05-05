#ifndef INMAPDRAW_H
#define INMAPDRAW_H

#include <string>
#include <list>

#include "GL/myGL.h"

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
	void GotNetMsg(const unsigned char* msg);
	void ToggleAllVisible() { drawAll = !drawAll; }
	void SetAllVisible(bool b) { drawAll = b; }

	float3 GetMouseMapPos(void);

	void LocalPoint(const float3& pos, const std::string& label, int playerID);
	void LocalLine(const float3& pos1, const float3& pos2, int playerID);
	void LocalErase(const float3& pos, int playerID);

	void SendPoint(const float3& pos, const std::string& label);
	void SendLine(const float3& pos1, const float3& pos2);
	void SendErase(const float3& pos);

	bool keyPressed;

	struct MapPoint {
		CR_DECLARE_STRUCT(MapPoint);
		void Serialize(creg::ISerializer &s);
		bool MaySee(CInMapDraw*) const;

		float3 pos;
		unsigned char* color;
		std::string label;

		int senderAllyTeam;
		bool senderSpectator;
	};

	struct MapLine {
		CR_DECLARE_STRUCT(MapLine);
		void Serialize(creg::ISerializer &s);
		bool MaySee(CInMapDraw*) const;

		float3 pos;
		float3 pos2;
		unsigned char* color;

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

	enum NetTypes {
		NET_POINT,
		NET_ERASE,
		NET_LINE
	};

	GLuint texture;
	float lastLeftClickTime;
	float lastLineTime;
	float3 lastPos;
	bool wantLabel;
	float3 waitingPoint;
	int blippSound;
	bool drawAll;

	static void InMapDrawVisCallback(int x, int y, void* userData);

	void PromptLabel(const float3& pos);
};

extern CInMapDraw* inMapDrawer;

#endif /* INMAPDRAW_H */
