#ifndef INMAPDRAW_H
#define INMAPDRAW_H

#include <string>
#include <list>
#include "Rendering/GL/myGL.h"


class CInMapDraw
{
	CR_DECLARE(CInMapDraw);
	CR_DECLARE_SUB(MapPoint);
	CR_DECLARE_SUB(MapLine);
	CR_DECLARE_SUB(DrawQuad);
public:
	CInMapDraw(void);
	~CInMapDraw(void);
	void Draw(void);
	void PostLoad();

	void MousePress(int x, int y, int button);
	void MouseRelease(int x,int y,int button);
	void MouseMove(int x, int y, int dx,int dy, int button);
	void GotNetMsg(const unsigned char* msg);

	float3 GetMouseMapPos(void);
	void ErasePos(const float3& pos);

	bool keyPressed;

	struct MapPoint {
		CR_DECLARE_STRUCT(MapPoint);
		void Serialize(creg::ISerializer &s);
		float3 pos;
		unsigned char* color;
		std::string label;
	};

	struct MapLine {
		CR_DECLARE_STRUCT(MapLine);
		void Serialize(creg::ISerializer &s);
		float3 pos;
		float3 pos2;
		unsigned char* color;
	};

	struct DrawQuad {
		CR_DECLARE_STRUCT(DrawQuad);
		std::list<MapPoint> points;
		std::list<MapLine> lines;
	};

	std::vector<DrawQuad> drawQuads;

	int drawQuadsX;
	int drawQuadsY;
	int numQuads;

	enum NetTypes{
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

	static void InMapDrawVisCallback (int x,int y,void *userData);
	void CreatePoint(const float3& pos, std::string label);
	void AddLine(const float3& pos, const float3& pos2);
	void PromptLabel (const float3& pos);;
};

extern CInMapDraw* inMapDrawer;

#endif /* INMAPDRAW_H */
