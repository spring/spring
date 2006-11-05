#ifndef MINIMAP_H
#define MINIMAP_H
// MiniMap.h: interface for the CMiniMap class.
//
//////////////////////////////////////////////////////////////////////

#include <vector>

#include "InputReceiver.h"
class CUnit;
class CIcon;

class CMiniMap : public CInputReceiver
{
public:
	void DrawUnit(CUnit* unit, float size);
	void DrawUnitHighlight(CUnit* unit, float size);
	CIcon* GetUnitIcon(CUnit* unit, float& scale) const;

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
	
	bool fullProxy;

	bool proxyMode;
	bool selecting;
	bool maximized;
	bool minimized;
	bool mouseMove;
	bool mouseResize;
	bool mouseLook;
	
	CUnit* pointedAt;
	
	float cursorScale;

	bool MousePress(int x, int y, int button);
	void MouseMove(int x, int y, int dx,int dy, int button);
	void MouseRelease(int x, int y, int button);
	void MoveView(int x, int y);
	bool IsAbove(int x, int y);
	std::string GetTooltip(int x, int y);

	void SelectUnit(CUnit* unit, int button) const;
	void SelectUnits(int x, int y) const;
	void ProxyMousePress(int x, int y, int button);
	void ProxyMouseRelease(int x, int y, int button);
	float3 GetMapPosition(int x, int y) const;
	CUnit* GetSelectUnit(const float3& pos) const;
	
	void AddNotification(float3 pos, float3 color, float alpha);

	struct Notification {
		float creationTime;
		float3 pos;
		float3 color;
		float alpha;
	};

	std::list<Notification> notes;
	void DrawNotes(void);

	bool useIcons;
	bool simpleColors;
	unsigned char myColor[4];
	unsigned char allyColor[4];
	unsigned char enemyColor[4];
};
extern CMiniMap* minimap;

#endif /* MINIMAP_H */
