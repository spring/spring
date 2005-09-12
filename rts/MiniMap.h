#ifndef MINIMAP_H
#define MINIMAP_H
// MiniMap.h: interface for the CMiniMap class.
//
//////////////////////////////////////////////////////////////////////

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

	void AddNotification(float3 pos, float3 color, float alpha);

	struct Notification {
		double creationTime;
		float3 pos;
		float3 color;
		float alpha;
	};

	std::list<Notification> notes;
	void DrawNotes(void);
};
extern CMiniMap* minimap;

#endif /* MINIMAP_H */
