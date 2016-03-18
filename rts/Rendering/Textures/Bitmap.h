/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BITMAP_H
#define _BITMAP_H

#include <string>
#include <vector>
#ifndef BITMAP_NO_OPENGL
	#include "nv_dds.h"
#endif // !BITMAP_NO_OPENGL
#include "System/float3.h"
#include "System/Color.h"


struct SDL_Surface;


class CBitmap
{
public:
	CBitmap(const unsigned char* data, int xsize, int ysize, int channels = 4);
	CBitmap();
	CBitmap(const CBitmap& old);
	CBitmap& operator=(const CBitmap& bm);
	CBitmap(CBitmap&& bm);
	CBitmap& operator=(CBitmap&& bm);

	virtual ~CBitmap();

	void Alloc(int w, int h, int c);
	void Alloc(int w, int h);
	void AllocDummy(const SColor fill = SColor(255,0,0,255));

	/// Load data from a file on the VFS
	bool Load(std::string const& filename, unsigned char defaultAlpha = 255);
	/// Load data from a gray-scale file on the VFS
	bool LoadGrayscale(std::string const& filename);
	bool Save(std::string const& filename, bool opaque = true) const;
	bool SaveFloat(std::string const& filename) const;

	unsigned int CreateTexture(float aniso = 0.0f, bool mipmaps = false) const;
	unsigned int CreateMipMapTexture(float aniso = 0.0f) const { return (CreateTexture(aniso, true)); }
	unsigned int CreateAnisoTexture(float aniso = 0.0f) const { return (CreateTexture(aniso, false)); }
	unsigned int CreateDDSTexture(unsigned int texID = 0, float aniso = 0.0f, bool mipmaps = false) const;

	void CreateAlpha(unsigned char red, unsigned char green, unsigned char blue);
	void SetTransparent(const SColor& c, const SColor trans = SColor(0,0,0,0));

	void Renormalize(float3 newCol);
	void Blur(int iterations = 1, float weight = 1.0f);

	void CopySubImage(const CBitmap& src, int x, int y);

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
	SDL_Surface* CreateSDLSurface() const;

	std::vector<unsigned char> mem;
	int xsize;
	int ysize;
	int channels;
	bool compressed;

#ifndef BITMAP_NO_OPENGL
	int textype; //! GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP, ...
	nv_dds::CDDSImage* ddsimage;
#endif // !BITMAP_NO_OPENGL

public:
	CBitmap CanvasResize(const int newx, const int newy, const bool center = true) const;
	CBitmap CreateRescaled(int newx, int newy) const;
	void ReverseYAxis();
	void InvertColors();
	void InvertAlpha();
	void GrayScale();
	void Tint(const float tint[3]);
};

#endif // _BITMAP_H
