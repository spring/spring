/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TERRAIN_TEXTURES_H
#define TERRAIN_TEXTURES_H

#include <string>
#include "Rendering/GL/myGL.h"

class TdfParser;
class CBitmap;

namespace terrain {

	struct ILoadCallback;
	struct Heightmap;
	class TQuad;

	struct float2
	{
		float2() : x(0.0f), y(0.0f) {}
		float2(float x, float y) : x(x), y(y) {}

		float x;
		float y;
	};


//-----------------------------------------------------------------------
// AlphaImage - a single channel image class, used for storing blendmaps
//-----------------------------------------------------------------------

	struct AlphaImage {
		AlphaImage();
		~AlphaImage();

		enum AreaTestResult {
			AREA_ONE, AREA_ZERO, AREA_MIXED 
		};

		void Alloc(int w, int h);
		float& at(int x, int y) { return data[y*w + x]; }
		bool CopyFromBitmap(CBitmap& bm);
		AlphaImage* CreateMipmap();
		void Blit(AlphaImage* dst, int x, int y, int dstx, int dsty, int w, int h);
		bool Save(const char* fn);
		AreaTestResult TestArea(int xstart, int ystart, int xend, int yend, float epsilon);

		int w;
		int h;
		float* data;

		// geo-mip-map chain links
		AlphaImage* lowDetail;
		AlphaImage* highDetail;
	};


//-----------------------------------------------------------------------
// BaseTexture
//-----------------------------------------------------------------------

	struct BaseTexture
	{
		BaseTexture();
		virtual ~BaseTexture();

		virtual bool ShareTexCoordUnit() { return true; }

		// the "texgen vector" is defined as:
		// { Xfactor, Zfactor, Xoffset, Zoffset }
		virtual void CalcTexGenVector(float* v4);
		virtual void SetupTexGen();

		virtual bool IsRect() { return false; }

		std::string name;
		GLuint id;
		float2 tilesize;
	};

	struct TiledTexture : public BaseTexture
	{
		TiledTexture();
		~TiledTexture();

		void Load(const std::string& name, const std::string& section, ILoadCallback* cb, const TdfParser* tdf, bool isBumpmap);
		static TiledTexture* CreateFlatBumpmap();
	};

    struct Blendmap : public BaseTexture
	{
		Blendmap();
		~Blendmap();

		void Generate(Heightmap* rootHm, int lodLevel, float hmscale, float hmoffset);
		void Load(const std::string& name, const std::string& section, Heightmap* heightmap, ILoadCallback* cb, const TdfParser* tdf);

		struct GeneratorInfo {
			float coverage, noise;
			float minSlope, maxSlope;
			float minSlopeFuzzy, maxSlopeFuzzy;
			float minHeight, maxHeight;
			float minHeightFuzzy, maxHeightFuzzy;
		};

		GeneratorInfo* generatorInfo;
		AlphaImage* image; // WIP image pointer

		AlphaImage::AreaTestResult curAreaResult;
	};

	/**
	 * A bumpmap generated from a heightmap, used to increase detail on distant
	 * nodes.
	 */
	struct DetailBumpmap : public BaseTexture
	{
		~DetailBumpmap();
		void CalcTexGenVector(float* v4);
		bool ShareTexCoordUnit() { return false; }

		TQuad* node;
	};

	GLuint LoadTexture(const std::string& fn, bool isBumpmap = false);
	void SaveImage(const char* fn, int components, GLenum type, int w, int h, void* data);
	void SetTexCoordGen(float* tgv);
};

#endif

