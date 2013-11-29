/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <utility>
#include <ostream>
#include <fstream>
#include <string.h>
#include <IL/il.h>
//#include <IL/ilu.h>
#include <SDL_video.h>
#include <boost/thread.hpp>

#ifndef BITMAP_NO_OPENGL
	#include "Rendering/GL/myGL.h"
	#include "System/TimeProfiler.h"
#endif // !BITMAP_NO_OPENGL

#include "Bitmap.h"
#include "Rendering/GlobalRendering.h"
#include "System/bitops.h"
#include "System/ScopedFPUSettings.h"
#include "System/Log/ILog.h"
#include "System/ThreadPool.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/FileSystem/FileHandler.h"


boost::mutex devilMutex; // devil functions, whilst expensive, aren't thread-save

static const float blurkernel[9] = {
	1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f,
	2.0f/16.0f, 4.0f/16.0f, 2.0f/16.0f,
	1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f
};
// this is a minimal list of file formats that (should) be available at all platforms
static const int formatList[] = {
	IL_PNG, IL_JPG, IL_TGA, IL_DDS, IL_BMP,
	IL_RGBA, IL_RGB, IL_BGRA, IL_BGR,
	IL_COLOUR_INDEX, IL_LUMINANCE, IL_LUMINANCE_ALPHA
};

static bool IsValidImageFormat(int format) {
	bool valid = false;

	// check if format is in the allowed list
	for (size_t i = 0; i < (sizeof(formatList) / sizeof(formatList[0])); i++) {
		if (format == formatList[i]) {
			valid = true;
			break;
		}
	}

	return valid;
}


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
	, compressed(false)
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
	, compressed(false)
#ifndef BITMAP_NO_OPENGL
	, textype(old.textype)
	, ddsimage(NULL)
#endif // !BITMAP_NO_OPENGL
{
	assert(!old.compressed);

	const int size = xsize*ysize * channels;
	mem = new unsigned char[size];
	memcpy(mem, old.mem, size);
}


CBitmap::CBitmap(CBitmap&& bm)
	: mem(std::move(bm.mem))
	, xsize(std::move(bm.xsize))
	, ysize(std::move(bm.ysize))
	, channels(std::move(bm.channels))
	, compressed(std::move(bm.compressed))
#ifndef BITMAP_NO_OPENGL
	, textype(std::move(bm.textype))
	, ddsimage(std::move(bm.ddsimage))
#endif
{
}


CBitmap::CBitmap(const unsigned char* data, int _xsize, int _ysize, int _channels)
	: mem(NULL)
	, xsize(_xsize)
	, ysize(_ysize)
	, channels(_channels)
	, compressed(false)
#ifndef BITMAP_NO_OPENGL
	, textype(GL_TEXTURE_2D)
	, ddsimage(NULL)
#endif
{
	const int size = xsize*ysize * channels;
	mem = new unsigned char[size];
	memcpy(mem, data, size);
}


CBitmap& CBitmap::operator=(const CBitmap& bm)
{
	if (this != &bm) {
		xsize = bm.xsize;
		ysize = bm.ysize;
		channels = bm.channels;
		compressed = bm.compressed;

		const int size = xsize*ysize * channels;
		delete[] mem;
		mem = new unsigned char[size];
		memcpy(mem, bm.mem, size);

#ifndef BITMAP_NO_OPENGL
		textype = bm.textype;
		delete ddsimage;
		if (bm.ddsimage == NULL) {
			ddsimage = NULL;
		} else {
			ddsimage = new nv_dds::CDDSImage();
			*ddsimage = *(bm.ddsimage);
		}
#endif // !BITMAP_NO_OPENGL
	}

	return *this;
}


CBitmap& CBitmap::operator=(CBitmap&& bm)
{
	xsize = bm.xsize;
	ysize = bm.ysize;
	channels = bm.channels;
	compressed = bm.compressed;
	mem = bm.mem;
	bm.mem = NULL;

#ifndef BITMAP_NO_OPENGL
	textype = bm.textype;
	ddsimage = bm.ddsimage;
	bm.ddsimage = NULL;
#endif // !BITMAP_NO_OPENGL

	return *this;
}


void CBitmap::Alloc(int w, int h)
{
	delete[] mem;
	xsize = w;
	ysize = h;

	const int size = w * h * channels;

	mem = new unsigned char[size];
	memset(mem, 0, size);
}

void CBitmap::AllocDummy()
{
	compressed = false;
	channels = 4;
	Alloc(1, 1);
	mem[0] = 255; // Red allows us to easily see textures that failed to load
	mem[1] = 0;
	mem[2] = 0;
	mem[3] = 255; // Non Transparent
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
#ifndef BITMAP_NO_OPENGL
		compressed = true;
		xsize = 0;
		ysize = 0;
		channels = 0;

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
#else // !BITMAP_NO_OPENGL
		AllocDummy(); //allocate a dummy texture, as dds aren't supported in headless
		return true;
#endif // !BITMAP_NO_OPENGL
	}

	compressed = false;
	channels = 4;

	CFileHandler file(filename);
	if (file.FileExists() == false) {
		AllocDummy();
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

	{
		// do not signal floating point exceptions in devil library
		ScopedDisableFpuExceptions fe;

		const bool success = !!ilLoadL(IL_TYPE_UNKNOWN, buffer, file.FileSize());
		ilDisable(IL_ORIGIN_SET);
		delete[] buffer;

		if (success == false) {
			AllocDummy();
			return false;
		}
	}

	{
		if (!IsValidImageFormat(ilGetInteger(IL_IMAGE_FORMAT))) {
			LOG_L(L_ERROR, "Invalid image format for %s: %d", filename.c_str(), ilGetInteger(IL_IMAGE_FORMAT));
			delete[] buffer;
			return false;
		}
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
	compressed = false;
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
	mem = NULL; // to prevent a dead-pointer in case of an out-of-memory exception on the next line
	mem = new unsigned char[xsize * ysize];
	memcpy(mem, ilGetData(), xsize * ysize);

	ilDeleteImages(1, &ImageName);

	return true;
}


bool CBitmap::Save(std::string const& filename, bool opaque) const
{
	if (compressed) {
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

	const std::string fullpath = dataDirsAccess.LocateFile(filename, FileQueryFlags::WRITE);
	const bool success = ilSaveImage((char*)fullpath.c_str());

	ilDeleteImages(1, &ImageName);
	ilDisable(IL_ORIGIN_SET);
	delete[] buf;

	return success;
}


#ifndef BITMAP_NO_OPENGL
const unsigned int CBitmap::CreateTexture(bool mipmaps) const
{
	if (compressed) {
		return CreateDDSTexture(0, mipmaps);
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


static void HandleDDSMipmap(GLenum target, bool mipmaps, int num_mipmaps)
{
	if (num_mipmaps > 0) {
		// dds included the MipMaps use them
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	} else {
		if (mipmaps && IS_GL_FUNCTION_AVAILABLE(glGenerateMipmap)) {
			// create the mipmaps at runtime
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glGenerateMipmap(target);
		} else {
			// no mipmaps
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
	}
}

const unsigned int CBitmap::CreateDDSTexture(unsigned int texID, bool mipmaps) const
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
		if (!ddsimage->upload_texture2D(0, GL_TEXTURE_2D)) {
			glDeleteTextures(1, &texID);
			texID = 0;
			break;
		}
		HandleDDSMipmap(GL_TEXTURE_2D, mipmaps, ddsimage->get_num_mipmaps());
		break;
	case nv_dds::Texture3D:
		glEnable(GL_TEXTURE_3D);
		glBindTexture(GL_TEXTURE_3D, texID);
		if (!ddsimage->upload_texture3D()) {
			glDeleteTextures(1, &texID);
			texID = 0;
			break;
		}
		HandleDDSMipmap(GL_TEXTURE_3D, mipmaps, ddsimage->get_num_mipmaps());
		break;
	case nv_dds::TextureCubemap:
		glEnable(GL_TEXTURE_CUBE_MAP);
		glBindTexture(GL_TEXTURE_CUBE_MAP, texID);
		if (!ddsimage->upload_textureCubemap()) {
			glDeleteTextures(1, &texID);
			texID = 0;
			break;
		}
		HandleDDSMipmap(GL_TEXTURE_CUBE_MAP, mipmaps, ddsimage->get_num_mipmaps());
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

const unsigned int CBitmap::CreateDDSTexture(unsigned int texID, bool mipmaps) const {
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
	if (compressed) {
		return;
	}

	static const uint32_t RGB = 0x00FFFFFF;

	uint32_t* mem_i = reinterpret_cast<uint32_t*>(mem);
	for (int y = 0; y < ysize; ++y) {
		for (int x = 0; x < xsize; ++x) {
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


inline static void kernelBlur(CBitmap* dst, const unsigned char* src, int x, int y, int channel, float weight)
{
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
		int offset = (yoffset * dst->xsize + xoffset) * dst->channels;
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
	if (compressed) {
		return;
	}

	CBitmap* src = this;
	CBitmap* dst = new CBitmap();
	dst->channels = src->channels;
	dst->Alloc(xsize,ysize);

	for (int i=0; i < iterations; ++i) {
		for_mt(0, ysize, [&](const int y) {
			for (int x=0; x < xsize; x++) {
				for (int j=0; j < channels; j++) {
					kernelBlur(dst, src->mem, x, y, j, weight);
				}
			}
		});
		std::swap(src, dst);
	}

	if (dst == this) {
		// make sure we don't delete `this`
		std::swap(src, dst);
	}

	delete dst;
}


void CBitmap::CopySubImage(const CBitmap& src, int xpos, int ypos)
{
	if (xpos + src.xsize > xsize || ypos + src.ysize > ysize) {
		LOG_L(L_WARNING, "CBitmap::CopySubImage src image does not fit into dst!");
		return;
	}

	if (compressed || src.compressed) {
		LOG_L(L_WARNING, "CBitmap::CopySubImage can't copy compressed textures!");
		return;
	}

	for (int y=0; y < src.ysize; ++y) {
		const int pixelDst = (((ypos + y) * xsize) + xpos) * channels;
		const int pixelSrc = ((y * src.xsize) + 0 ) * channels;

		// copy the whole line
		memcpy(mem + pixelDst, src.mem + pixelSrc, channels * src.xsize);
	}
}


CBitmap CBitmap::CanvasResize(const int newx, const int newy, const bool center) const
{
	CBitmap bm;

	if (xsize > newx || ysize > newy) {
		LOG_L(L_WARNING, "CBitmap::CanvasResize can only upscale (tried to resize %ix%i to %ix%i)!", xsize,ysize,newx,newy);
		bm.AllocDummy();
		return bm;
	}

	const int borderLeft = (center) ? (newx - xsize) / 2 : 0;
	const int borderTop  = (center) ? (newy - ysize) / 2 : 0;

	bm.channels = channels;
	bm.Alloc(newx, newy);
	bm.CopySubImage(*this, borderLeft, borderTop);

	return bm;
}


SDL_Surface* CBitmap::CreateSDLSurface(bool newPixelData) const
{
	SDL_Surface* surface = NULL;

	if (channels < 3) {
		LOG_L(L_WARNING, "CBitmap::CreateSDLSurface works only with 24bit RGB and 32bit RGBA pictures!");
		return surface;
	}

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
	newx = std::max(1, newx);
	newy = std::max(1, newy);

	CBitmap bm;

	if (compressed) {
		LOG_L(L_WARNING, "CBitmap::CreateRescaled doesn't work with compressed textures!");
		bm.AllocDummy();
		return bm;
	}

	if (channels != 4) {
		LOG_L(L_WARNING, "CBitmap::CreateRescaled only works with RGBA data!");
		bm.AllocDummy();
		return bm;
	}

	bm.Alloc(newx, newy);

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
	if (compressed) {
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


void CBitmap::InvertAlpha()
{
	if (compressed) return; // Don't try to invert DDS
	for (int y = 0; y < ysize; ++y) {
		for (int x = 0; x < xsize; ++x) {
			const int base = ((y * xsize) + x) * 4;
			mem[base + 3] = 0xFF - mem[base + 3];
		}
	}
}


void CBitmap::GrayScale()
{
	if (compressed) {
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
	if (compressed) {
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
	if (compressed) return; // don't try to flip DDS
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
