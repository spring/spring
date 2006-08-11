#ifndef MOUSECURSOR_H
#define MOUSECURSOR_H

#include <string>
#include <vector>

using namespace std;

class CBitmap;

class CMouseCursor
{
public:
	enum HotSpot {TopLeft, Center};
	static CMouseCursor* New(const string &name, HotSpot hs);
	CMouseCursor(const string &name, HotSpot hs);
	~CMouseCursor(void);
	void Update();
	void Draw(int x, int y);
	void DrawQuad(int x, int y);
	void BindTexture();
protected:	
	CBitmap* getAlignedBitmap(const CBitmap &orig);
	void setBitmapTransparency(CBitmap &bm, int r, int g, int b);
	struct FrameData {
		FrameData() : texture(0), xsize(0), ysize(0) {}
		FrameData(unsigned int t, int x, int y) : texture(t), xsize(x), ysize(y) {}
		unsigned int  texture;
		int xsize;
		int ysize;
	};
	vector<FrameData> frames;
	double lastFrameTime;
	int curFrame;
	int xofs, yofs;			//Describes where the center of the cursor is. Calculated after the largest cursor if animated
};


#endif /* MOUSECURSOR_H */
