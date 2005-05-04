// GUIresourceBar.h: interface for the GUIresourceBar class.

//

//////////////////////////////////////////////////////////////////////



#if !defined(GUIRESOURCEBAR_H)

#define GUIRESOURCEBAR_H



#if _MSC_VER >= 1000

#pragma once

#endif // _MSC_VER >= 1000



#include "GUIpane.h"



class GUIresourceBar : public GUIpane  

{

public:

	GUIresourceBar(int x, int y, int w, int h);

	virtual ~GUIresourceBar();



protected:

	void PrivateDraw();

	void SizeChanged();



	bool MouseDownAction(int x1, int y1, int buttons);

	bool MouseMoveAction(int x1, int y1, int xrel, int yrel, int buttons);

	bool MouseUpAction(int x1, int y1, int buttons);



	bool downInBar;	

};



#endif // !defined(GUIRESOURCEBAR_H)

