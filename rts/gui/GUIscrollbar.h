// GUIscrollbar.h: interface for the GUIscrollbar class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GUISCROLLBAR_H__A9702A2C_FB58_4A2F_9FFE_C3D19696DA16__INCLUDED_)
#define AFX_GUISCROLLBAR_H__A9702A2C_FB58_4A2F_9FFE_C3D19696DA16__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GUIframe.h"
#include "Functor.h"

class GUIscrollbar : public GUIframe  
{
public:
	GUIscrollbar(int x, int y, int height, int m, Functor1<int>clicked);
	virtual ~GUIscrollbar();
	
	void SetPosition(int p);

protected:
	Functor1<int> click;
	int maximum;
	int position;
	void PrivateDraw();
	bool downInControl;

	bool MouseUpAction(int x, int y, int button);
	bool MouseDownAction(int x, int y, int button);
	bool MouseMoveAction(int x1, int y1, int xrel, int yrel, int buttons);
	GLuint displayList;
};

#endif // !defined(AFX_GUISCROLLBAR_H__A9702A2C_FB58_4A2F_9FFE_C3D19696DA16__INCLUDED_)
