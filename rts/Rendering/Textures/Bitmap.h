/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <string>
#ifndef BITMAP_NO_OPENGL
	#include "nv_dds.h"
#endif // !BITMAP_NO_OPENGL
#include "System/float3.h"
#include "System/Color.h"


struct SDL_Surface;


class CBitmap  
{
public:
	CBitmap(const unsigned char* data,int xsize,int ysize);
	CBitmap();
	CBitmap(const CBitmap& old);
	CBitmap& operator=(const CBitmap& bm);

	virtual ~CBitmap();

	void Alloc(int w, int h);
	bool Load(std::string const& filename, unsigned char defaultAlpha = 255);
	bool LoadGrayscale(std::string const& filename);
	bool Save(std::string const& filename, bool opaque = true) const;

#ifndef BITMAP_NO_OPENGL
	const unsigned int CreateTexture(bool mipmaps = false) const;
	const unsigned int CreateDDSTexture(unsigned int texID = 0) const;
#else  // !BITMAP_NO_OPENGL
	const unsigned int CreateTexture(bool mipmaps = false) const;
	const unsigned int CreateDDSTexture(unsigned int texID = 0) const;
#endif // !BITMAP_NO_OPENGL

	void CreateAlpha(unsigned char red, unsigned char green, unsigned char blue);
	void SetTransparent(const SColor& c, const SColor& trans = SColor(0,0,0,0));

	void Renormalize(float3 newCol);
	void Blur(int iterations = 1, float weight = 1.0f);

	CBitmap GetRegion(int startx, int starty, int width, int height) const;
	void CopySubImage(const CBitmap& src, unsigned int x, unsigned int y);

	/**
	 * Allocates a new SDL_Surface, and feeds it with the data of this bitmap.
	 * @param newPixelData
	 *        if false, the returned struct will have a pointer
	 *        to the internal pixel data (mem), which means it is only valid
	 *        as long as mem is not freed, which should only happen
	 *        when the this bitmap is deleted.
	 *        If true, an array is allocated with new, and has to be deleted
	 *        after SDL_FreeSurface() is called.
	 * Note:
	 * - You have to free the surface with SDL_FreeSurface(surface)
	 *   if you do not need it anymore!
	 */
	SDL_Surface* CreateSDLSurface(bool newPixelData = false) const;

	unsigned char* mem;
	int xsize;
	int ysize;
	int channels;

	enum BitmapType
	{
		BitmapTypeStandardRGBA,
		BitmapTypeStandardAlpha,
		BitmapTypeDDS
	};

	int type;
#ifndef BITMAP_NO_OPENGL
	int textype; //! GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP, ...
	nv_dds::CDDSImage* ddsimage;
#endif // !BITMAP_NO_OPENGL

public:
	CBitmap CreateRescaled(int newx, int newy) const;
	void ReverseYAxis();
	void InvertColors();
	void GrayScale();
	void Tint(const float tint[3]);
};

#endif // __BITMAP_H__
