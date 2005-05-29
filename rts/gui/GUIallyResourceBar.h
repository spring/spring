#if !defined(GUIALLYRESOURCEBAR_H)

#define GUIALLYRESOURCEBAR_H



#if _MSC_VER >= 1000

#pragma once

#endif // _MSC_VER >= 1000



#include "GUIpane.h"



class GUIallyResourceBar : public GUIpane  

{

public:

	GUIallyResourceBar(int x, int y, int w, int h);

	virtual ~GUIallyResourceBar();



protected:

	void PrivateDraw();

	void SizeChanged();



	bool downInBar;	

};



#endif // !defined(GUIRESOURCEBAR_H)

