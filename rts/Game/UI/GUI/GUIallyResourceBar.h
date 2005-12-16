#if !defined(GUIALLYRESOURCEBAR_H)
#define GUIALLYRESOURCEBAR_H

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

