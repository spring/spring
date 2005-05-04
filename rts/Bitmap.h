// Bitmap.h: interface for the CBitmap class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __BITMAP_H__
#define __BITMAP_H__

#include "archdef.h"

#include <string>

using std::string;

class CBitmap  
{
public:
	CBitmap(unsigned char* data,int xsize,int ysize);
	CBitmap(string filename);
	CBitmap();
	CBitmap(const CBitmap& old);
	void operator=(const CBitmap& bm);

	virtual ~CBitmap();

	void Load(string filename);
	void Save(string filename);

	unsigned int CreateTexture(bool mipmaps);

	void CreateAlpha(unsigned char red,unsigned char green,unsigned char blue);
	void Renormalize(float3 newCol);

	CBitmap GetRegion(int startx, int starty, int width, int height);
	CBitmap CreateMipmapLevel(void);

	unsigned char* mem;
	int xsize;
	int ysize;
protected:
	void LoadJPG(string filename);
	void LoadBMP(string filename);
	void LoadPCX(string filename);

	void SaveBMP(string filename);
	void SaveJPG(string filename,int quality=85);
public:
	CBitmap CreateRescaled(int newx, int newy);
	void ReverseYAxis(void);
};

#endif // __BITMAP_H__
