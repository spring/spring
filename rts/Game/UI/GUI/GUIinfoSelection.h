// GUIinfoSelection.h: interface for the GUIinfoSelection class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(GUIINFOSELECTION_H)
#define GUIINFOSELECTION_H

#include "GUIpane.h"

struct GroupUnitsInfo {
	int number;
	int health;
	int maxHealth;
	int numStockpiled;
	int numStockpileQued;
	int id;
};

class GUIinfoSelection : public GUIpane  
{
public:
	GUIinfoSelection(int x, int y, int w, int h);
	virtual ~GUIinfoSelection();
	void SetBuildPicSize(int size) { buildPicSize=size; }

protected:
	void PrivateDraw();
	void SizeChanged();

	bool MouseDownAction(int x1, int y1, int buttons);
	bool MouseMoveAction(int x1, int y1, int xrel, int yrel, int buttons);
	bool MouseUpAction(int x1, int y1, int buttons);

	int buildPicSize;
};

#endif // !defined(GUIINFOSELECTION_H)
