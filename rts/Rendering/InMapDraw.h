#ifndef INMAPDRAW_H
#define INMAPDRAW_H

#include <string>
#include <list>
#include "Rendering/GL/myGL.h"


class CInMapDraw
{
public:
	CInMapDraw(void);
	~CInMapDraw(void);
	void Draw(void);

	void MousePress(int x, int y, int button);
	void MouseRelease(int x,int y,int button);
	void MouseMove(int x, int y, int dx,int dy, int button);
	void GotNetMsg(unsigned char* msg);

	float3 GetMouseMapPos(void);
	void ErasePos(const float3& pos);

	bool keyPressed;

	struct MapPoint {
		float3 pos;
		unsigned char* color;
		std::string label;
	};

	struct MapLine {
		float3 pos;
		float3 pos2;
		unsigned char* color;
	};

	struct DrawQuad {
		std::list<MapPoint> points;
		std::list<MapLine> lines;
	};

	DrawQuad* drawQuads;

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
