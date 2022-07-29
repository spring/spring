/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BITMAP_H
#define _BITMAP_H

#include <stdint.h>
#include <string>
#include <vector>
#ifndef HEADLESS
	#include "nv_dds.h"
#endif // !HEADLESS
#include "System/float3.h"
#include "System/Color.h"


struct SDL_Surface;


class CBitmap {
public:
	CBitmap();
	CBitmap(const uint8_t* data, int xsize, int ysize, int channels = 4, uint32_t reqDataType = 0);
	CBitmap(const CBitmap& bmp): CBitmap() { *this = bmp; }
	CBitmap(CBitmap&& bmp) noexcept : CBitmap() { *this = std::move(bmp); }
	CBitmap& operator=(const CBitmap& bmp);
	CBitmap& operator=(CBitmap&& bmp) noexcept;

	~CBitmap();

	CBitmap CanvasResize(const int newx, const int newy, const bool center = true) const;
	CBitmap CreateRescaled(int newx, int newy) const;

	static bool CanBeKilled();
	static void InitPool(size_t size);
	static void KillPool();

	void Alloc(int w, int h, int c, uint32_t glType);
	void Alloc(int w, int h, int c) { Alloc(w, h, c, 0x1401/*GL_UNSIGNED_BYTE*/); }
	void Alloc(int w, int h) { Alloc(w, h, channels); }
	void AllocDummy(const SColor fill = SColor(255, 0, 0, 255));

	int32_t GetIntFmt() const;
	int32_t GetExtFmt() const { return GetExtFmt(channels); }
	static int32_t GetExtFmt(uint32_t ch);
	static int32_t ExtFmtToChannels(int32_t extFmt);
	uint32_t GetDataTypeSize() const;

	/// Load data from a file on the VFS
	bool Load(std::string const& filename, float defaultAlpha = 1.0f, uint32_t reqChannel = 4, uint32_t reqDataType = 0x1401/*GL_UNSIGNED_BYTE*/);
	/// Load data from a gray-scale file on the VFS
	bool LoadGrayscale(std::string const& filename);

	bool Save(const std::string& filename, bool opaque, bool logged = false, unsigned quality = 80) const;
	bool SaveGrayScale(const std::string& filename) const;
	bool SaveFloat(const std::string& filename) const;

	bool Empty() const { return (memIdx == size_t(-1)); } // implies size=0

	unsigned int CreateTexture(float aniso = 0.0f, float lodBias = 0.0f, bool mipmaps = false, uint32_t texID = 0) const;
	unsigned int CreateMipMapTexture(float aniso = 0.0f, float lodBias = 0.0f) const { return (CreateTexture(aniso, lodBias, true)); }
	unsigned int CreateAnisoTexture(float aniso = 0.0f, float lodBias = 0.0f) const { return (CreateTexture(aniso, lodBias, false)); }
	unsigned int CreateDDSTexture(unsigned int texID = 0, float aniso = 0.0f, float lodBias = 0.0f, bool mipmaps = false) const;

	void CreateAlpha(uint8_t red, uint8_t green, uint8_t blue);
	void ReplaceAlpha(float a = 1.0f);
	void SetTransparent(const SColor& c, const SColor trans = SColor(0, 0, 0, 0));

	void Renormalize(const float3& newCol);
	void Blur(int iterations = 1, float weight = 1.0f);
	void Fill(const SColor& c);

	void CopySubImage(const CBitmap& src, int x, int y);

	void ReverseYAxis();
	void InvertColors();
	void InvertAlpha();
	void MakeGrayScale();
	void Tint(const float tint[3]);

	/**
	 * Allocates a new SDL_Surface, and feeds it with the data of this bitmap.
	 * Note:
	 * - You have to free the surface with SDL_FreeSurface(surface)
	 *   if you do not need it anymore!
	 */
	SDL_Surface* CreateSDLSurface();

	const uint8_t* GetRawMem() const;
	      uint8_t* GetRawMem()      ;

	size_t GetMemSize() const { return (xsize * ysize * channels * GetDataTypeSize()); }

private:
	// managed by pool
	size_t memIdx = size_t(-1);

public:
	int32_t xsize = 0;
	int32_t ysize = 0;
	int32_t channels = 4;
	uint32_t dataType = 0;

	#ifndef HEADLESS
	// GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP, ...
	// not set to anything until Load is called
	int32_t textype = 0;

	nv_dds::CDDSImage ddsimage;
	#endif

	bool compressed = false;
};

#endif // _BITMAP_H
