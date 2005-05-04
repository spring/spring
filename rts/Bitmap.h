// Bitmap.h: interface for the CBitmap class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BITMAP_H__BBA7EEE5_879F_4ABE_A878_51FE098C3A0D__INCLUDED_)
#define AFX_BITMAP_H__BBA7EEE5_879F_4ABE_A878_51FE098C3A0D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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

#endif // !defined(AFX_BITMAP_H__BBA7EEE5_879F_4ABE_A878_51FE098C3A0D__INCLUDED_)
