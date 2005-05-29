#ifndef GUIPANE_H
#define GUIPANE_H
// GUIpane.h: interface for the GUIpane class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GUIPANE_H__C4F238EC_3007_4684_B81B_A89330F3F9B6__INCLUDED_)
#define AFX_GUIPANE_H__C4F238EC_3007_4684_B81B_A89330F3F9B6__INCLUDED_

#if _MSC_VER >= 1000
/*pragma once removed*/
#endif // _MSC_VER >= 1000

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

#endif // !defined(AFX_GUIPANE_H__C4F238EC_3007_4684_B81B_A89330F3F9B6__INCLUDED_)

#endif /* GUIPANE_H */
