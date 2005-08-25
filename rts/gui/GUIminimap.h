// GUIminimap.h: interface for the GUIminimap class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(GUIMINIMAP_H)
#define GUIMINIMAP_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GUIframe.h"
#include <vector>
class CUnit;

class GUIminimap : public GUIframe
{
public:
	GUIminimap();
	virtual ~GUIminimap();

protected:

	bool MouseUpAction(int x, int y, int button);
	bool MouseDownAction(int x, int y, int button);
	bool MouseMoveAction(int x, int y, int xrel, int yrel, int button);

	void PrivateDraw();
	void SizeChanged();
	
	void MoveView(int x, int y, int button);
	
	GLuint unitBlip;
	
	void GetFrustumSide(float3& side);
	void DrawInMap(const float3& pos);
	void DrawUnit(CUnit* unit, const float size);
	
	void SelectCursor();
	
	struct fline 
	{
		float base;
		float dir;
		int left;
		float minz;
		float maxz;
	};
	std::vector<fline> left;

	void AddNotification(float3 pos, float3 color, float alpha);

	struct Notification {
		double creationTime;
		float3 pos;
		float3 color;
		float alpha;
	};

	std::list<Notification> notes;
	void DrawNotes(void);
	
	bool downInMinimap;
};

#endif // !defined(GUIMINIMAP_H)
