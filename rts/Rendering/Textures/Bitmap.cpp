/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

//#include <omp.h>
#include <ostream>
#include <fstream>
#include <string.h>
#include <IL/il.h>
//#include <IL/ilu.h>
#include <SDL_video.h>
#include <boost/thread.hpp>
#include "mmgr.h"

#include "Rendering/GlobalRendering.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/FileSystem.h"
#include "GlobalUnsynced.h"
#include "Bitmap.h"
#include "bitops.h"
#include "LogOutput.h"

#ifndef BITMAP_NO_OPENGL
	#include "Rendering/GL/myGL.h"
	#include "System/TimeProfiler.h"
#endif // !BITMAP_NO_OPENGL

boost::mutex devilMutex; // devil functions, whilst expensive, aren't thread-save

static const float blurkernel[9] = {
	1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f,
	2.0f/16.0f, 4.0f/16.0f, 2.0f/16.0f,
	1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f
};


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

struct InitializeOpenIL {
	InitializeOpenIL() {
		ilInit();
		//iluInit();
	}
	~InitializeOpenIL() {
		ilShutDown();
	}
} static initOpenIL;


CBitmap::CBitmap()
	: mem(NULL)
	, xsize(0)
	, ysize(0)
	, channels(4)
	, type(BitmapTypeStandardRGBA)
#ifndef BITMAP_NO_OPENGL
	, textype(GL_TEXTURE_2D)
	, ddsimage(NULL)
#endif // !BITMAP_NO_OPENGL
{
}


CBitmap::~CBitmap()
{
	delete[] mem;
#ifndef BITMAP_NO_OPENGL
	delete ddsimage;
#endif // !BITMAP_NO_OPENGL
}


CBitmap::CBitmap(const CBitmap& old)
	: mem(NULL)
	, xsize(old.xsize)
	, ysize(old.ysize)
	, channels(old.channels)
	, type(old.type)
#ifndef BITMAP_NO_OPENGL
	, textype(old.textype)
	, ddsimage(NULL)
#endif // !BITMAP_NO_OPENGL
{
	assert(old.type != BitmapTypeDDS);

	const int size = xsize*ysize * channels;
	mem = new unsigned char[size];
	memcpy(mem, old.mem, size);
}


CBitmap::CBitmap(const unsigned char* data, int xsize, int ysize)
	: mem(NULL)
	, xsize(xsize)
	, ysize(ysize)
	, channels(4)
	, type(BitmapTypeStandardRGBA)
#ifndef BITMAP_NO_OPENGL
	, textype(GL_TEXTURE_2D)
	, ddsimage(NULL)
#endif // !BITMAP_NO_OPENGL
{
	const int size = xsize*ysize * channels;
	mem = new unsigned char[size];
	memcpy(mem, data, size);
}


CBitmap& CBitmap::operator=(const CBitmap& bm)
{
	if (this != &bm) {
		type  = bm.type;
		xsize = bm.xsize;
		ysize = bm.ysize;
		channels = bm.channels;

		const int size = xsize*ysize * channels;
		delete[] mem;
		mem = new unsigned char[size];
		memcpy(mem, bm.mem, size);
	}

	return *this;
}


void CBitmap::Alloc(int w, int h)
{
	delete[] mem;
	xsize = w;
	ysize = h;
	const int size = w*h*channels;
	mem = new unsigned char[size];
	memset(mem, 0, size);
}


bool CBitmap::Load(std::string const& filename, unsigned char defaultAlpha)
{
#ifndef BITMAP_NO_OPENGL
	ScopedTimer timer("Textures::CBitmap::Load");
#endif

	bool noAlpha = true;

	delete[] mem;
	mem = NULL;

#ifndef BITMAP_NO_OPENGL
	textype = GL_TEXTURE_2D;
#endif // !BITMAP_NO_OPENGL

	if (filename.find(".dds") != std::string::npos) {
		type = BitmapTypeDDS;
		xsize = 0;
		ysize = 0;
		channels = 0;
#ifndef BITMAP_NO_OPENGL
		ddsimage = new nv_dds::CDDSImage();
		bool status = ddsimage->load(filename);
		if (status) {
			xsize = ddsimage->get_width();
			ysize = ddsimage->get_height();
			channels = ddsimage->get_components();
			switch (ddsimage->get_type()) {
				case nv_dds::TextureFlat :
					textype = GL_TEXTURE_2D;
					break;
				case nv_dds::Texture3D :
					textype = GL_TEXTURE_3D;
					break;
				case nv_dds::TextureCubemap :
					textype = GL_TEXTURE_CUBE_MAP;
					break;
				case nv_dds::TextureNone :
				default :
					break;
			}
		}
		return status;
#else
		return false;
#endif // !BITMAP_NO_OPENGL
	}
	type = BitmapTypeStandardRGBA;
	channels = 4;

	CFileHandler file(filename);
	if (file.FileExists() == false) {
		Alloc(1, 1);
		return false;
	}

	unsigned char* buffer = new unsigned char[file.FileSize() + 2];
	file.Read(buffer, file.FileSize());

	boost::mutex::scoped_lock lck(devilMutex);
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);

	ILuint ImageName = 0;
	ilGenImages(1, &ImageName);
	ilBindImage(ImageName);

	const bool success = !!ilLoadL(IL_TYPE_UNKNOWN, buffer, file.FileSize());
	ilDisable(IL_ORIGIN_SET);
	delete[] buffer;

	if (success == false) {
		xsize = 1;
		ysize = 1;
		mem = new unsigned char[4];
		mem[0] = 255; // Red allows us to easily see textures that failed to load
		mem[1] = 0;
		mem[2] = 0;
		mem[3] = 255; // Non Transparent
		return false;
	}

	noAlpha = (ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL) != 4);
	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
	xsize = ilGetInteger(IL_IMAGE_WIDTH);
	ysize = ilGetInteger(IL_IMAGE_HEIGHT);

	mem = new unsigned char[xsize * ysize * 4];
	//ilCopyPixels(0, 0, 0, xsize, ysize, 0, IL_RGBA, IL_UNSIGNED_BYTE, mem);
	memcpy(mem, ilGetData(), xsize * ysize * 4);

	ilDeleteImages(1, &ImageName);

	if (noAlpha) {
		for (int y=0; y < ysize; ++y) {
			for (int x=0; x < xsize; ++x) {
				mem[((y*xsize+x) * 4) + 3] = defaultAlpha;
			}
		}
	}

	return true;
}


bool CBitmap::LoadGrayscale(const std::string& filename)
{
	type = BitmapTypeStandardAlpha;
	channels = 1;

	CFileHandler file(filename);
	if (!file.FileExists()) {
		return false;
	}

	unsigned char* buffer = new unsigned char[file.FileSize() + 1];
	file.Read(buffer, file.FileSize());

	boost::mutex::scoped_lock lck(devilMutex);
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);

	ILuint ImageName = 0;
	ilGenImages(1, &ImageName);
	ilBindImage(ImageName);

	const bool success = !!ilLoadL(IL_TYPE_UNKNOWN, buffer, file.FileSize());
	ilDisable(IL_ORIGIN_SET);
	delete[] buffer;

	if (success == false) {
		return false;
	}

	ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
	xsize = ilGetInteger(IL_IMAGE_WIDTH);
	ysize = ilGetInteger(IL_IMAGE_HEIGHT);

	delete[] mem;
	mem = new unsigned char[xsize * ysize];
	memcpy(mem, ilGetData(), xsize * ysize);
	
	ilDeleteImages(1, &ImageName);
	
	return true;
}


bool CBitmap::Save(std::string const& filename, bool opaque) const
{
	if (type == BitmapTypeDDS) {
#ifndef BITMAP_NO_OPENGL
		return ddsimage->save(filename);
#else
		return false;
#endif // !BITMAP_NO_OPENGL
	}

	unsigned char* buf = new unsigned char[xsize * ysize * 4];
	const int ymax = (ysize - 1);
	/* HACK Flip the image so it saves the right way up.
		(Fiddling with ilOriginFunc didn't do anything?)
		Duplicated with ReverseYAxis. */
	for (int y = 0; y < ysize; ++y) {
		for (int x = 0; x < xsize; ++x) {
			const int bi = 4 * (x + (xsize * (ymax - y)));
			const int mi = 4 * (x + (xsize * (y)));
			buf[bi + 0] = mem[mi + 0];
			buf[bi + 1] = mem[mi + 1];
			buf[bi + 2] = mem[mi + 2];
			buf[bi + 3] = opaque ? 0xff : mem[mi + 3];
		}
	}

	boost::mutex::scoped_lock lck(devilMutex);
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);

	ilHint(IL_COMPRESSION_HINT, IL_USE_COMPRESSION);
	ilSetInteger(IL_JPG_QUALITY, 80);

	ILuint ImageName = 0;
	ilGenImages(1, &ImageName);
	ilBindImage(ImageName);

	ilTexImage(xsize, ysize, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, buf);

	const std::string fullpath = filesystem.LocateFile(filename, FileSystem::WRITE);
	const bool success = ilSaveImage((char*)fullpath.c_str());

	ilDeleteImages(1, &ImageName);
	ilDisable(IL_ORIGIN_SET);
	delete[] buf;

	return success;
}


#ifndef BITMAP_NO_OPENGL
const unsigned int CBitmap::CreateTexture(bool mipmaps) const
{
#ifndef BITMAP_NO_OPENGL
	ScopedTimer timer("Textures::CBitmap::CreateTexture");
#endif

	if (type == BitmapTypeDDS) {
		return CreateDDSTexture();
	}

	if (mem == NULL) {
		return 0;
	}

	// jcnossen: Some drivers return "2.0" as a version string,
	// but switch to software rendering for non-power-of-two textures.
	// GL_ARB_texture_non_power_of_two indicates that the hardware will actually support it.
	if (!globalRendering->supportNPOTs && (xsize != next_power_of_2(xsize) || ysize != next_power_of_2(ysize)))
	{
		CBitmap bm = CreateRescaled(next_power_of_2(xsize), next_power_of_2(ysize));
		return bm.CreateTexture(mipmaps);
	}

	unsigned int texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//FIXME add glPixelStorei(GL_UNPACK_ALIGNMENT, 1); for NPOTs

	if (mipmaps) {
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);

		glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, mem);
	} else {
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		//glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8 ,xsize, ysize, 0,GL_RGBA, GL_UNSIGNED_BYTE, mem);
		//gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, mem);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, xsize, ysize, 0, GL_RGBA, GL_UNSIGNED_BYTE, mem);
	}

	return texture;
}


const unsigned int CBitmap::CreateDDSTexture(unsigned int texID) const
{
	glPushAttrib(GL_TEXTURE_BIT);

	if (texID == 0) {
		glGenTextures(1, &texID);
	}

	switch (ddsimage->get_type())
	{
	case nv_dds::TextureNone:
		glDeleteTextures(1, &texID);
		texID = 0;
		break;
	case nv_dds::TextureFlat:    // 1D, 2D, and rectangle textures
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		if (!ddsimage->upload_texture2D(0, GL_TEXTURE_2D)) {
			glDeleteTextures(1, &texID);
			texID = 0;
		}
		break;
	case nv_dds::Texture3D:
		glEnable(GL_TEXTURE_3D);
		glBindTexture(GL_TEXTURE_3D, texID);
		if (!ddsimage->upload_texture3D()) {
			glDeleteTextures(1, &texID);
			texID = 0;
		}
		break;
	case nv_dds::TextureCubemap:
		glEnable(GL_TEXTURE_CUBE_MAP_ARB);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, texID);
		if (!ddsimage->upload_textureCubemap()) {
			glDeleteTextures(1, &texID);
			texID = 0;
		}
		break;
	default:
		assert(false);
		break;
	}

	glPopAttrib();
	return texID;
}
#else  // !BITMAP_NO_OPENGL

const unsigned int CBitmap::CreateTexture(bool mipmaps) const {
	return 0;
}

const unsigned int CBitmap::CreateDDSTexture(unsigned int texID) const {
	return 0;
}
#endif // !BITMAP_NO_OPENGL


void CBitmap::CreateAlpha(unsigned char red, unsigned char green, unsigned char blue)
{
	float3 aCol;
	for(int a=0; a < 3; ++a) {
		int cCol = 0;
		int numCounted = 0;
		for (int y=0; y < ysize; ++y) {
			for (int x=0; x < xsize; ++x) {
				const int index = (y*xsize + x) * 4;
				if ((mem[index + 3] != 0) &&
					!(
						(mem[index + 0] == red) &&
						(mem[index + 1] == green) &&
						(mem[index + 2] == blue)
					))
				{
					cCol += mem[index + a];
					++numCounted;
				}
			}
		}
		if (numCounted != 0) {
			aCol[a] = cCol / 255.0f / numCounted;
		}
	}

	SColor c(red, green, blue);
	SColor a(aCol.x, aCol.y, aCol.z, 0.0f);
	SetTransparent(c, a);
}


void CBitmap::SetTransparent(const SColor& c, const SColor& trans)
{
	if (type != BitmapTypeStandardRGBA) {
		return;
	}

	static const uint32_t RGB = 0x00FFFFFF;

	uint32_t* mem_i = reinterpret_cast<uint32_t*>(mem);
	for (unsigned int y = 0; y < ysize; ++y) {
		for (unsigned int x = 0; x < xsize; ++x) {
			if ((*mem_i & RGB) == (c.i & RGB))
				*mem_i = trans.i;
			mem_i++;
		}
	}
}


void CBitmap::Renormalize(float3 newCol)
{
	float3 aCol;
	float3 colorDif;
	for (int a=0; a < 3; ++a) {
		int cCol = 0;
		int numCounted = 0;
		for (int y=0; y < ysize; ++y) {
			for (int x=0; x < xsize;++x) {
				const unsigned int index = (y*xsize + x) * 4;
				if (mem[index + 3] != 0) {
					cCol += mem[index + a];
					++numCounted;
				}
			}
		}
		aCol[a] = cCol / 255.0f / numCounted;
		cCol /= xsize*ysize;
		colorDif[a] = newCol[a] - aCol[a];
	}
	for (int a=0; a < 3; ++a) {
		for (int y=0; y < ysize; ++y) {
			for (int x=0; x < xsize; ++x) {
				const unsigned int index = (y*xsize + x) * 4;
				float nc = float(mem[index + a]) / 255.0f + colorDif[a];
				mem[index + a] = (unsigned char) (std::min(255.f, std::max(0.0f, nc*255)));
			}
		}
	}
}


inline void kernelBlur(CBitmap* dst, const unsigned char* src, int x, int y, int channel, float weight) {
	float fragment = 0.0f;

	const int pos = (x + y * dst->xsize) * dst->channels + channel;

	for (int i=0; i < 9; ++i) {
		int yoffset = (i / 3) - 1;
		int xoffset = (i - (yoffset + 1) * 3) - 1;

		const int tx = x + xoffset;
		const int ty = y + yoffset;
		if ((tx < 0) || (tx >= dst->xsize)) {
			xoffset = 0;
		}
		if ((ty < 0) || (ty >= dst->ysize)) {
			yoffset = 0;
		}
		int offset = (xoffset + yoffset * dst->xsize) * dst->channels;
		if (i == 4) {
			fragment += weight * blurkernel[i] * src[pos + offset];
		} else {
			fragment += blurkernel[i] * src[pos + offset];
		}
	}

	dst->mem[pos] = (unsigned char)std::min(255.0f,std::max(0.0f, fragment ));
}


void CBitmap::Blur(int iterations, float weight)
{
	if (type == BitmapTypeDDS) {
		return;
	}

	CBitmap* src = this;
	CBitmap* dst = new CBitmap();
	dst->channels = src->channels;
	dst->Alloc(xsize,ysize);

	for (int i=0; i < iterations; ++i){
		#pragma omp parallel private(y,x,i)
		{
			#pragma omp for
			for (int y=0; y < ysize; ++y) {
				for (int x=0; x < xsize; ++x) {
					for (int i=0; i < channels; ++i) {
						kernelBlur(dst, src->mem, x, y, i, weight);
					}
				}
			}
		}
		CBitmap* buf = dst;
		dst = src;
		src = buf;
	}

	if (dst == this) {
		CBitmap* buf = dst;
		dst = src;
		src = buf;
	}

	delete dst;
}


// Unused
CBitmap CBitmap::GetRegion(int startx, int starty, int width, int height) const
{
	CBitmap bm;

	delete[] bm.mem;
	bm.mem = new unsigned char[width*height * channels];
	bm.channels = channels;
	bm.type = type;
	bm.xsize = width;
	bm.ysize = height;

	for (int y=0; y < height; ++y) {
		for (int x=0; x < width; ++x) {
			for (int i=0; i < channels; ++i) {
				bm.mem[(y*width + x)*channels + i] = mem[((starty + y)*xsize + startx + x)*channels + i];
			}
		}
	}

	return bm;
}


void CBitmap::CopySubImage(const CBitmap& src, unsigned int xpos, unsigned int ypos)
{
	if (xpos + src.xsize >= xsize || ypos + src.ysize >= ysize) {
		logOutput.Print("CBitmap::CopySubImage src image doesn't fit into dst");
		return;
	}

	if (src.type != BitmapTypeStandardRGBA || type != BitmapTypeStandardRGBA) {
		return;
	}

	for (int y=0; y < src.ysize; ++y) {
		const int pixelDst = (((ypos + y) * xsize) + xpos) * channels;
		const int pixelSrc = ((y * src.xsize) + 0 ) * channels;

		// copy the whole line
		memcpy(mem + pixelDst, src.mem + pixelSrc, channels * src.xsize);
	}
}


SDL_Surface* CBitmap::CreateSDLSurface(bool newPixelData) const
{
	SDL_Surface* surface = NULL;

	unsigned char* surfData = NULL;
	if (newPixelData) {
		// copy pixel data
		surfData = new unsigned char[xsize * ysize * channels];
		memcpy(surfData, mem, xsize * ysize * channels);
	} else {
		surfData = mem;
	}

	// This will only work with 24bit RGB and 32bit RGBA pictures
	surface = SDL_CreateRGBSurfaceFrom(surfData, xsize, ysize, 8 * channels, xsize * channels, 0x000000FF, 0x0000FF00, 0x00FF0000, (channels == 4) ? 0xFF000000 : 0);
	if ((surface == NULL) && newPixelData) {
		// cleanup when we failed to the create surface
		delete[] surfData;
	}

	return surface;
}


CBitmap CBitmap::CreateRescaled(int newx, int newy) const
{
	CBitmap bm;

	delete[] bm.mem;
	bm.xsize = newx;
	bm.ysize = newy;
	bm.mem   = new unsigned char[bm.xsize*bm.ysize * 4];

	const float dx = (float) xsize / newx;
	const float dy = (float) ysize / newy;

	float cy = 0;
	for (int y=0; y < newy; ++y) {
		const int sy = (int) cy;
		cy += dy;
		int ey = (int) cy;
		if (ey == sy) {
			ey = sy+1;
		}

		float cx = 0;
		for (int x=0; x < newx; ++x) {
			const int sx = (int) cx;
			cx += dx;
			int ex = (int) cx;
			if (ex == sx) {
				ex = sx + 1;
			}

			int r=0, g=0, b=0, a=0;
			for (int y2 = sy; y2 < ey; ++y2) {
				for (int x2 = sx; x2 < ex; ++x2) {
					const int index = (y2*xsize + x2) * 4;
					r += mem[index + 0];
					g += mem[index + 1];
					b += mem[index + 2];
					a += mem[index + 3];
				}
			}
			const int index = (y*bm.xsize + x) * 4;
			const int denom = (ex - sx) * (ey - sy);
			bm.mem[index + 0] = r / denom;
			bm.mem[index + 1] = g / denom;
			bm.mem[index + 2] = b / denom;
			bm.mem[index + 3] = a / denom;
		}
	}

	return bm;
}


void CBitmap::InvertColors()
{
	if (type != BitmapTypeStandardRGBA) {
		return;
	}
	for (int y = 0; y < ysize; ++y) {
		for (int x = 0; x < xsize; ++x) {
			const int base = ((y * xsize) + x) * 4;
			mem[base + 0] = 0xFF - mem[base + 0];
			mem[base + 1] = 0xFF - mem[base + 1];
			mem[base + 2] = 0xFF - mem[base + 2];
			// do not invert alpha
		}
	}
}


void CBitmap::GrayScale()
{
	if (type != BitmapTypeStandardRGBA) {
		return;
	}
	for (int y = 0; y < ysize; ++y) {
		for (int x = 0; x < xsize; ++x) {
			const int base = ((y * xsize) + x) * 4;
			const float illum =
				(mem[base + 0] * 0.299f) +
				(mem[base + 1] * 0.587f) +
				(mem[base + 2] * 0.114f);
			const unsigned int  ival = (unsigned int)(illum * (256.0f / 255.0f));
			const unsigned char cval = (ival <= 0xFF) ? ival : 0xFF;
			mem[base + 0] = cval;
			mem[base + 1] = cval;
			mem[base + 2] = cval;
		}
	}
}

static ILubyte TintByte(ILubyte value, float tint)
{
	float f = (float)value;
	f = std::max(0.0f, std::min(255.0f, f * tint));
	return (unsigned char)f;
}


void CBitmap::Tint(const float tint[3])
{
	if (type != BitmapTypeStandardRGBA) {
		return;
	}
	for (int y = 0; y < ysize; y++) {
		for (int x = 0; x < xsize; x++) {
			const int base = ((y * xsize) + x) * 4;
			mem[base + 0] = TintByte(mem[base + 0], tint[0]);
			mem[base + 1] = TintByte(mem[base + 1], tint[1]);
			mem[base + 2] = TintByte(mem[base + 2], tint[2]);
			// don't touch the alpha channel
		}
	}
}


void CBitmap::ReverseYAxis()
{
	unsigned char* tmpLine = new unsigned char[channels * xsize];
	for (int y=0; y < (ysize / 2); ++y) {
		const int pixelLow  = (((y            ) * xsize) + 0) * channels;
		const int pixelHigh = (((ysize - 1 - y) * xsize) + 0) * channels;

		// copy the whole line
		memcpy(tmpLine,         mem + pixelHigh, channels * xsize);
		memcpy(mem + pixelHigh, mem + pixelLow,  channels * xsize);
		memcpy(mem + pixelLow,  tmpLine,         channels * xsize);
	}
	delete[] tmpLine;
}
