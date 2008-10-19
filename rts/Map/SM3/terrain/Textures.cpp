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

#include "StdAfx.h"

#include "TdfParser.h"

#include <cassert>
#include <deque>

#include "TerrainBase.h"
#include "TerrainNode.h"
#include "Textures.h"
#include "Rendering/Textures/Bitmap.h"
#include "Platform/FileSystem.h"
#include "System/float3.h"


namespace terrain {

	void SetTexCoordGen (float *tgv)
	{
		glTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		glTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

		Plane s(tgv[0],0,0,tgv[2]);
		glTexGenfv (GL_S, GL_OBJECT_PLANE, (float*)&s);
		Plane t(0,0,tgv[1],tgv[3]);
		glTexGenfv (GL_T, GL_OBJECT_PLANE, (float*)&t);

		glEnable (GL_TEXTURE_GEN_S);
		glEnable (GL_TEXTURE_GEN_T);
	}

//-----------------------------------------------------------------------
// Texturing objects
//-----------------------------------------------------------------------

	BaseTexture::BaseTexture ()
	{
		id = 0;
		tilesize.x = tilesize.y = 10.0f;
	}

	BaseTexture::~BaseTexture()
	{
		if (id) {
			glDeleteTextures(1, &id);
			id = 0;
		}
	}

	void BaseTexture::CalcTexGenVector (float *v)
	{
		v[0] = 1.0f / (SquareSize * tilesize.x);
		v[1] = 1.0f / (SquareSize * tilesize.y);
		v[2] = v[3] = 0.0f;
	}

	void BaseTexture::SetupTexGen ()
	{
		float tgv[4];
		CalcTexGenVector (tgv);

		Plane s(tgv[0],0,0,tgv[2]);
		glTexGenfv (GL_S, GL_OBJECT_PLANE, (float*)&s);
		Plane t(0,0,tgv[1],tgv[3]);
		glTexGenfv (GL_T, GL_OBJECT_PLANE, (float*)&t);
	}

	TiledTexture::TiledTexture ()
	{}

	TiledTexture::~TiledTexture()
	{}

	void TiledTexture::Load(const std::string& name, const std::string& section, ILoadCallback *cb, const TdfParser *tdf, bool isBumpmap)
	{
		this->name = name;

		tilesize.x = tilesize.y = atof (tdf->SGetValueDef ("10", section + "\\Tilesize").c_str());

		string texture;
		if (isBumpmap) {
			if (!tdf->SGetValue (texture, section + "\\Bumpmap"))
				return;

			this->name += "_BM";
		} else {
			if (!tdf->SGetValue (texture, section + "\\File"))
				throw content_error("Texture " + section + " has no image file.");
		}

		if (cb) cb->PrintMsg ("    loading %s %s from %s...", isBumpmap ? "bumpmap" : "texture", name.c_str(), texture.c_str());
		id = LoadTexture (texture);
	}

	TiledTexture* TiledTexture::CreateFlatBumpmap()
	{
		TiledTexture *tt = SAFE_NEW TiledTexture;
		glGenTextures(1, &tt->id);
		glBindTexture(GL_TEXTURE_2D, tt->id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		uchar img[3] = { 127, 127, 255 };
		glTexImage2D (GL_TEXTURE_2D,0, 3, 1, 1, 0,GL_RGB, GL_UNSIGNED_BYTE, img);
		tt->name = "_flatbm";
		return tt;
	}

	Blendmap::Blendmap ()
	{
		generatorInfo = 0;
		image = 0;
	}

	Blendmap::~Blendmap ()
	{
		if (generatorInfo)
			delete generatorInfo;
	}

	void Blendmap::Load(const std::string& name, const std::string& section, Heightmap *heightmap, ILoadCallback *cb, const TdfParser *tdf)
	{
		// Create blendmap
		this->name = name;
		tilesize.x = heightmap->w;
		tilesize.y = heightmap->h;

		// Is the blendmap generated or loaded from a file?
		string blendmapImg;
		tdf->GetDef (blendmapImg, string(), section + "\\File");
		bool generateBlendmap = blendmapImg.empty();
		if (generateBlendmap) {
			// Load blendfactor function parameters
			GeneratorInfo *gi = generatorInfo = SAFE_NEW GeneratorInfo;
			gi->coverage = atof (tdf->SGetValueDef ("1", section + "\\Coverage").c_str());
			gi->noise = atof (tdf->SGetValueDef ("0", section + "\\Noise").c_str());
			gi->minSlope = atof (tdf->SGetValueDef ("0", section + "\\MinSlope").c_str());
			gi->maxSlope = atof (tdf->SGetValueDef ("1", section + "\\MaxSlope").c_str());
			gi->minSlopeFuzzy = atof (tdf->SGetValueDef ("0", section + "\\MinSlopeFuzzy").c_str());
			gi->maxSlopeFuzzy = atof (tdf->SGetValueDef ("0", section + "\\MaxSlopeFuzzy").c_str());
			gi->maxHeight = atof (tdf->SGetValueDef ("1.01", section + "\\MaxHeight").c_str());
			gi->minHeight = atof (tdf->SGetValueDef ("-0.01", section + "\\MinHeight").c_str());
			gi->maxHeightFuzzy = atof (tdf->SGetValueDef ("0", section + "\\MaxHeightFuzzy").c_str());
			gi->minHeightFuzzy = atof (tdf->SGetValueDef ("0", section + "\\MinHeightFuzzy").c_str());

			int generatorLOD = atoi (tdf->SGetValueDef ("0", section + "\\GeneratorLOD").c_str());

			float hmScale = atof (tdf->SGetValueDef ("1000", "map\\terrain\\HeightScale").c_str());
			float hmOffset = atof (tdf->SGetValueDef ("0", "map\\terrain\\HeightOffset").c_str());

			if (cb) cb->PrintMsg ("    generating %s...", name.c_str());
			Generate (heightmap, generatorLOD, hmScale, hmOffset);
		}
		else { // Load blendmap from file
			CBitmap bitmap;
			if (!bitmap.LoadGrayscale (blendmapImg))
				throw std::runtime_error("Failed to load blendmap image " + blendmapImg);

			if (cb) cb->PrintMsg ("    loading blendmap %s from %s...", name.c_str(), blendmapImg.c_str());

			image = SAFE_NEW AlphaImage;
			image->CopyFromBitmap (bitmap);
		}
	}

	static void BlendmapFilter(AlphaImage *img)
	{
		float *nd = SAFE_NEW float[img->w*img->h];

		//3.95f*(x-.5f)^3+.5

		for (int y=0;y<img->h;y++) {
			for (int x=0;x<img->w;x++) {
				int l=x?x-1:x;
				int r=x<img->w-1?x+1:x;
				int t=y?y-1:y;
				int b=y<img->h-1?y+1:y;

				float result = 0.0f;

				result += img->at (l,t);
				result += img->at (x,t);
				result += img->at (r,t);

				result += img->at (l,y);
				//result += img->at (x,y);
				result += img->at (r,y);

				result += img->at (l,b);
				result += img->at (x,b);
				result += img->at (r,b);

				result *= (1.0f/8.0f);
				nd[y*img->w+x] = result;
			}
		}

		memcpy (img->data, nd, sizeof(float)*img->w*img->h);
		delete[] nd;
	}


	void Blendmap::Generate (Heightmap *rootHm, int lodLevel, float hmScale, float hmOffset)
	{
		Heightmap *heightmap = rootHm->GetLevel(-lodLevel);

		// Allocate the blendmap image
		AlphaImage *bm = SAFE_NEW AlphaImage;
		bm->Alloc (heightmap->w-1, heightmap->h-1); // texture dimensions have to be power-of-two

		Blendmap::GeneratorInfo *gi = generatorInfo;

		// Calculate blend factors using the function parameters and heightmap input:
		for (int y=0;y<bm->h;y++)
		{
			for (int x=0;x<bm->w;x++)
			{
				float h = (heightmap->at (x,y) - hmOffset) / hmScale;
				float factor=1.0f;

				if(h < gi->minHeight - gi->minHeightFuzzy) {
					bm->at (x,y) = 0.0f;
					continue;
				} else if (gi->minHeightFuzzy > 0.0f && h < gi->minHeight + gi->minHeightFuzzy)
					factor = (h - (gi->minHeight - gi->minHeightFuzzy)) / (2.0f * gi->minHeightFuzzy);

				if(h > gi->maxHeight + gi->maxHeightFuzzy) {
					bm->at (x,y) = 0.0f;
					continue;
				} else if (gi->maxHeightFuzzy > 0.0f && h > gi->maxHeight - gi->maxHeightFuzzy)
					factor *= ((gi->maxHeight + gi->maxHeightFuzzy) - h) / (2.0f * gi->maxHeightFuzzy);

                float norm_y = 0.0f;
				if (heightmap->normalData) {
					uchar *cnorm = heightmap->GetNormal (x,y);
					norm_y = cnorm[1] / 255.0f * 2.0f - 1.0f;
					if (norm_y > 1.0f) norm_y = 1.0f;
				} else {
					Vector3 tangent, binormal;
					CalculateTangents(heightmap, x,y,tangent,binormal);
					Vector3 normal = tangent.cross(binormal);
					normal.ANormalize();
					norm_y = normal.y;
				}

				// flatness=dotproduct of surface normal with up vector
				float slope = 1.0f - fabs(norm_y);

				if (slope < gi->minSlope - gi->minSlopeFuzzy) {
					bm->at (x,y) = 0.0f;
					continue;
				} else if (gi->minSlopeFuzzy > 0.0f && slope < gi->minSlope + gi->minSlopeFuzzy)
					factor *= (h - (gi->minSlope - gi->minSlopeFuzzy)) / ( 2.0f * gi->minSlopeFuzzy);

				if (slope > gi->maxSlope + gi->maxSlopeFuzzy) {
					bm->at (x,y) = 0.0f;
					continue;
				} else if (gi->maxSlopeFuzzy > 0.0f && slope > gi->maxSlope - gi->maxSlopeFuzzy)
					factor *= ((gi->maxSlope + gi->maxSlopeFuzzy) - h) / (2.0f * gi->maxSlopeFuzzy);

				factor *= gi->coverage;
				factor *= (rand () < gi->noise * RAND_MAX) ? 0.0f : 1.0f;

				bm->at (x,y) = factor;
			}
		}

		BlendmapFilter(bm);
		image = bm;
	}


	void DetailBumpmap::CalcTexGenVector (float *v4)
	{
		QuadRenderData *rd = node->renderData;
		float pw = 1.0f / rd->normalMapTexWidth;

		v4[0] = pw * (rd->normalMapW - 1) / (node->end.x - node->start.x);
		v4[2] = 0.5f * pw - v4[0] * node->start.x;

		v4[1] = pw * (rd->normalMapW - 1) / (node->end.z - node->start.z);
		v4[3] = 0.5f * pw - v4[1] * node->start.z;
	}

	DetailBumpmap::~DetailBumpmap() {}


	// Util functions

	GLuint GenSphereBumpmap ()
	{
		GLuint id;
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);

		uchar img[3 * 64 * 64], *p = img;

		for (int y=0;y<64;y++) {
			for (int x=0;x<64;x++)
			{
				int sx = x-32;
				int sy = y-32;
				Vector3 n;

				if (sx*sx + sy*sy < 32*32) {
					int sz = (int)sqrt(static_cast<float>(32 * 32 - sx*sx - sy*sy));
					n = Vector3(sx,sy,sz);
					n.ANormalize ();
				}

				// compress it into a color
				*(p++) = (uchar)(255 * (n.x * 0.5f + 0.5f));
				*(p++) = (uchar)(255 * (n.y * 0.5f + 0.5f));
				*(p++) = (uchar)(255 * (n.z * 0.5f + 0.5f));
			}
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D (GL_TEXTURE_2D,0, 3, 64, 64, 0,GL_RGB, GL_UNSIGNED_BYTE, img);

		SaveImage("sphere_bm.jpg", 3, GL_UNSIGNED_BYTE, 64,64,img);
		return id;
	}

	GLuint LoadTexture (const string& fn, bool isBumpmap)
	{
		CBitmap bmp;

		if (!bmp.Load (fn))
			throw content_error ("Failed to load texture: " + fn);

		GLuint tex = bmp.CreateTexture (true);
		return tex;
	}

	void SaveImage(const char *fn, int components, GLenum type, int w,int h, void *data)
	{
		// We use the fact that DevIL has the same constants for component type as OpenGL
		/// ( GL_UNSIGNED_BYTE = IL_UNSIGNED_BYTE for example )
#ifdef TERRAIN_USE_IL
		// Save image
		ILuint out;
		ilGenImages(1,&out);
		ilBindImage(out);
		ILenum fmt=IL_RGB;
		if (components==4) fmt = IL_RGBA;
		if (components==1) fmt = IL_LUMINANCE;
		ilTexImage(w,h,1,components,fmt,type,data);
		filesystem.Remove(fn);
		ilSaveImage((ILstring)fn);
		ilDeleteImages(1,&out);
#endif
	}

};

