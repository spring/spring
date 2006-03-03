// Bitmap.h: interface for the CBitmap class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <string>

#include "Platform/Win/win32.h"
#include "Rendering/Textures/nv_dds.h"

using std::string;

class CBitmap  
{
public:
	CBitmap(unsigned char* data,int xsize,int ysize);
	CBitmap(string const& filename);
	CBitmap();
	CBitmap(const CBitmap& old);
	CBitmap& operator=(const CBitmap& bm);

	virtual ~CBitmap();

	void Load(string const& filename, unsigned char defaultAlpha=255);
	void Save(string const& filename);

	unsigned int CreateTexture(bool mipmaps=false);
	unsigned int CreateDDSTexture();

	void CreateAlpha(unsigned char red,unsigned char green,unsigned char blue);
	void SetTransparent(unsigned char red, unsigned char green, unsigned char blue);

	void Renormalize(float3 newCol);

	CBitmap GetRegion(int startx, int starty, int width, int height);
	CBitmap CreateMipmapLevel(void);

	unsigned char* mem;
	int xsize;
	int ysize;

	enum BitmapType
	{
		BitmapTypeStandar,
		BitmapTypeDDS
	};

	int type;
	nv_dds::CDDSImage *ddsimage;

public:
	CBitmap CreateRescaled(int newx, int newy);
	void ReverseYAxis(void);
};

#endif // __BITMAP_H__
