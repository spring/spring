// GUIslider.h: interface for the GUIslider class.

//

//////////////////////////////////////////////////////////////////////



#if !defined(GUISLIDER_H)

#define GUISLIDER_H



#if _MSC_VER >= 1000

#pragma once

#endif // _MSC_VER >= 1000



#include "GUIframe.h"

#include "Functor.h"



class GUIslider : public GUIframe  

{

public:

	GUIslider(int x, int y, int width, int max, Functor2<GUIslider*, int>clicked);

	virtual ~GUIslider();

	

	void SetPosition(int p);

	void SetMaximum(int m);

	

	int Position() { return position; }



protected:

	Functor2<GUIslider*, int> click;

	int maximum;

	int position;

	void PrivateDraw();

	bool downInControl;



	bool MouseDownAction(int x, int y, int button);

	bool MouseMoveAction(int x1, int y1, int xrel, int yrel, int buttons);

	bool MouseUpAction(int x, int y, int button);

};



#endif // !defined(GUISLIDER_H)

