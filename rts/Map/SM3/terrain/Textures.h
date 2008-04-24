/*
---------------------------------------------------------------------
   Terrain Renderer using texture splatting and geomipmapping
   Copyright (c) 2006 Jelmer Cnossen

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any
   purpose, including commercial applications, and to alter it and
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
      distribution.

   Jelmer Cnossen
   j.cnossen at gmail dot com
---------------------------------------------------------------------
*/

#ifndef TERRAIN_TEXTURES_H
#define TERRAIN_TEXTURES_H

class TdfParser;
class CBitmap;

namespace terrain {

	struct ILoadCallback;
	struct Heightmap;
	class TQuad;

	struct float2
	{
		float2() { x=y=0.0f; }
		float2(float X,float Y) : x(X), y(Y) {}

		float x,y;
	};


//-----------------------------------------------------------------------
// AlphaImage - a single channel image class, used for storing blendmaps
//-----------------------------------------------------------------------

	struct AlphaImage {
		AlphaImage ();
		~AlphaImage ();

		enum AreaTestResult {
			AREA_ONE, AREA_ZERO, AREA_MIXED 
		};

		void Alloc (int W,int H);
		float& at(int x,int y) { return data[y*w+x]; }
		bool CopyFromBitmap(CBitmap& bm);
		AlphaImage* CreateMipmap ();
		void Blit (AlphaImage *dst, int x,int y,int dstx, int dsty, int w ,int h);
		bool Save (const char *fn);
		AreaTestResult TestArea (int xstart,int ystart,int xend,int yend, float epsilon);

		int w,h;
		float *data;

		AlphaImage *lowDetail, *highDetail; // geomipmap chain links
	};


//-----------------------------------------------------------------------
// BaseTexture
//-----------------------------------------------------------------------

	struct BaseTexture
	{
		BaseTexture ();
		virtual ~BaseTexture();

		virtual bool ShareTexCoordUnit () { return true; }

		std::string name;
		GLuint id;
		float2 tilesize;

		// the "texgen vector" is defined as:
		// { Xfactor, Zfactor, Xoffset, Zoffset }
		virtual void CalcTexGenVector (float *v4);
		virtual void SetupTexGen();

		virtual bool IsRect() {return false;}
	};

	struct TiledTexture : public BaseTexture
	{
		TiledTexture ();
		~TiledTexture ();

		void Load(const std::string& name, const std::string& section, ILoadCallback *cb, const TdfParser *tdf, bool isBumpmap);
		static TiledTexture* CreateFlatBumpmap();
	};

    struct Blendmap : public BaseTexture
	{
		Blendmap ();
		~Blendmap ();

		void Generate (Heightmap *rootHm, int lodLevel, float hmscale, float hmoffset);
		void Load(const std::string& name, const std::string& section, Heightmap *heightmap, ILoadCallback *cb, const TdfParser *tdf);

		struct GeneratorInfo {
			float coverage, noise;
			float minSlope, maxSlope;
			float minSlopeFuzzy, maxSlopeFuzzy;
			float minHeight, maxHeight;
			float minHeightFuzzy, maxHeightFuzzy;
		};
		GeneratorInfo *generatorInfo;
		AlphaImage *image;  // WIP image pointer

		AlphaImage::AreaTestResult curAreaResult;
	};

	// A bumpmap generated from a heightmap, used to increase detail on distant nodes
	struct DetailBumpmap : public BaseTexture
	{
		~DetailBumpmap();
		void CalcTexGenVector (float *v4);
		bool ShareTexCoordUnit () { return false; }

		TQuad *node;
	};

	GLuint LoadTexture (const std::string& fn, bool isBumpmap=false);
	void SaveImage(const char *fn, int components, GLenum type, int w,int h, void *data);
	void SetTexCoordGen(float *tgv);
};


#endif
