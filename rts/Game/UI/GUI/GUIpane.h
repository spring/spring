// GUIpane.h: interface for the GUIpane class.
//
//////////////////////////////////////////////////////////////////////

#ifndef GUIPANE_H
#define GUIPANE_H

#include "GUIframe.h"

class GUIpane : public GUIframe
{
public:
	GUIpane(int x, int y, int w, int h);
	virtual ~GUIpane();

	void SetResizeable(bool s) { canResize=s; }
	void SetDraggable(bool s) { canDrag=s; }
	void SetFrame(bool s) { drawFrame=s; }
protected:

	bool MouseDownAction(int x, int y, int button);
	bool MouseMoveAction(int x, int y, int xrel, int yrel, int buttons);
	bool MouseUpAction(int x, int y, int button);

	void PrivateDraw();
	void SizeChanged();

	void BuildList();

	GLuint displayList;

	bool dragging;
	bool resizing;
	bool canDrag;
	bool canResize;
	bool drawFrame;
	bool downInside;
	
	int minW, minH;
};

#endif // GUIPANE_H
