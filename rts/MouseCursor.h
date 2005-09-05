#ifndef MOUSECURSOR_H
#define MOUSECURSOR_H
/*pragma once removed*/

#include <string>
#include <vector>

using namespace std;

class CBitmap;

class CMouseCursor
{
	vector<unsigned int> frames;
	vector<int> xsize;
	vector<int> ysize;
	double lastFrameTime;
	int curFrame;
	int xofs, yofs;			//Describes where the center of the cursor is. Calculated after the largest cursor if animated
public:
	enum HotSpot {TopLeft, Center};
	CMouseCursor(const string &name, HotSpot hs);
	~CMouseCursor(void);
	CBitmap* getAlignedBitmap(const CBitmap &orig);
	void setBitmapTransparency(CBitmap &bm, int r, int g, int b);
	void Draw(int x, int y);
};


#endif /* MOUSECURSOR_H */
