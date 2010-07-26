/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <string>
// We can not include gl.h here, cause sometimes glew.h has to be included
// before gl.h, so we have to do this in before including this header.
//#include <GL/gl.h> // for GLuint, GLenum

#ifndef BITMAP_NO_OPENGL
#include "nv_dds.h"
#endif // !BITMAP_NO_OPENGL
#include "float3.h"

class CBitmap  
{
public:
	CBitmap(const unsigned char* data,int xsize,int ysize);
	CBitmap();
	CBitmap(const CBitmap& old);
	CBitmap& operator=(const CBitmap& bm);

	virtual ~CBitmap();

	void Alloc(int w,int h);
	bool Load(std::string const& filename, unsigned char defaultAlpha=255);
	bool LoadGrayscale(std::string const& filename);
	bool Save(std::string const& filename, bool opaque = true);

#ifndef BITMAP_NO_OPENGL
	const GLuint CreateTexture(bool mipmaps=false);
	const GLuint CreateDDSTexture(GLuint texID = 0);
#endif // !BITMAP_NO_OPENGL

	void CreateAlpha(unsigned char red,unsigned char green,unsigned char blue);
	void SetTransparent(unsigned char red, unsigned char green, unsigned char blue);

	void Renormalize(float3 newCol);
	void Blur(int iterations = 1, float weight = 1.0f);

	CBitmap GetRegion(int startx, int starty, int width, int height);
	CBitmap CreateMipmapLevel(void);

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
	GLenum textype; //! GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP, ...
	nv_dds::CDDSImage* ddsimage;
#endif // !BITMAP_NO_OPENGL

public:
	CBitmap CreateRescaled(int newx, int newy);
	void ReverseYAxis();
	void InvertColors();
	void GrayScale();
	void Tint(const float tint[3]);
};

#endif // __BITMAP_H__
