// GUIscrollbar.h: interface for the GUIscrollbar class.
//
//////////////////////////////////////////////////////////////////////

#ifndef GUISCROLLBAR_H
#define GUISCROLLBAR_H

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

#endif // GUISCROLLBAR_H
