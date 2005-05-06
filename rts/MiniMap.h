// MiniMap.h: interface for the CMiniMap class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MINIMAP_H__031E512D_66AB_4071_8769_EBAFAB24BBEA__INCLUDED_)
#define AFX_MINIMAP_H__031E512D_66AB_4071_8769_EBAFAB24BBEA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>

#include "InputReceiver.h"
class CUnit;

class CMiniMap : public CInputReceiver
{
public:
	void DrawUnit(CUnit* unit,float size);
	struct fline {
		float base;
		float dir;
		int left;
		float minz;
		float maxz;
	};
	std::vector<fline> left;
	void GetFrustumSide(float3& side);
	void DrawInMap(const float3& pos);
	void Draw();
	CMiniMap();
	virtual ~CMiniMap();

	int xpos,ypos;
	int height,width;
	int oldxpos,oldypos;
	int oldheight,oldwidth;

	bool maximized;
	bool minimized;
	bool mouseMove,mouseResize,mouseLook;

	unsigned int unitBlip;
	bool MousePress(int x, int y, int button);
	void MouseMove(int x, int y, int dx,int dy, int button);
	void MouseRelease(int x, int y, int button);
	void MoveView(int x, int y);
	void FakeMousePress(float3 pos, int button);
	bool IsAbove(int x, int y);
	std::string GetTooltip(int x, int y);
};
extern CMiniMap* minimap;

#endif // !defined(AFX_MINIMAP_H__031E512D_66AB_4071_8769_EBAFAB24BBEA__INCLUDED_)

