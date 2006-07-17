/*---------------------------------------------------------------------
 Terrain Renderer using texture splatting and geomipmapping

 Copyright (2006) Jelmer Cnossen
 This code is released under GPL license (See LICENSE.html for info)
---------------------------------------------------------------------*/

#include "StdAfx.h"

#include "TdfParser.h"

#include <cassert>
#include <deque>

#include "TerrainBase.h"
#include "TerrainNode.h"
#include "Textures.h"
#include "Rendering/Textures/Bitmap.h"

using namespace std;

namespace terrain {

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
	{
		bumpMap = 0;
	}

	TiledTexture::~TiledTexture()
	{
		delete bumpMap;
	}

	void TiledTexture::Load(const std::string& name, const std::string& section, ILoadCallback *cb, TdfParser *tdf)
	{
		this->name = name;

		tilesize.x = tilesize.y = atof (tdf->SGetValueDef ("10", section + "\\Tilesize").c_str());
		bumpMapName = tdf->SGetValueDef (string(), section + "\\Bumpmap");

		string texture;
		if (!tdf->SGetValue (texture, section + "\\File"))
			throw content_error("Texture " + section + " has no image file.");

		if (cb) cb->PrintMsg ("    loading %s...", texture.c_str());
		id = LoadTexture (texture);
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

	void Blendmap::Load(const std::string& name, const std::string& section, Heightmap *heightmap, ILoadCallback *cb, TdfParser *tdf)
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
			GeneratorInfo *gi = generatorInfo = new GeneratorInfo;
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

			int generatorLOD = atof (tdf->SGetValueDef ("0", section + "\\GeneratorLOD").c_str());

			if (cb) cb->PrintMsg ("    generating %s...", name.c_str());
			Generate (heightmap, generatorLOD);
		}
		else { // Load blendmap from file
			CBitmap bitmap;
			if (!bitmap.LoadGrayscale (blendmapImg))
				throw std::runtime_error("Failed to load blendmap image " + blendmapImg);

			if (cb) cb->PrintMsg ("    loading blendmap %s from %s...", name.c_str(), blendmapImg.c_str());

			image = new AlphaImage;
			image->CopyFromBitmap (bitmap);
		}
	}

	static void BlendmapFilter(AlphaImage *img) 
	{
		float *nd = new float[img->w*img->h];

		//3.95*(x-.5)^3+.5

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
				float v = img->at(x,y);
				if (result > 0.5f) {
					v += 0.25f;
					if (v > 1.0f) v=1.0f;
				} else if(result < 0.5f) {
					v -= 0.25f;
					if (v < 0.0f) v=0.0f;
				}
				nd[y*img->w+x] = v;
			}
		}

		memcpy (img->data, nd, sizeof(float)*img->w*img->h);
		delete[] nd;
	}


	void Blendmap::Generate (Heightmap *rootHm, int lodLevel)
	{
		Heightmap *heightmap = rootHm->GetLevel(-lodLevel);

		// Allocate the blendmap image
		AlphaImage *bm = new AlphaImage;
		bm->Alloc (heightmap->w-1, heightmap->h-1); // texture dimensions have to be power-of-two

		Blendmap::GeneratorInfo *gi = generatorInfo;

		// Calculate blend factors using the function parameters and heightmap input:
		// blendFactor = max(0, (1 - abs(height - originHeight) / heightRange) * (1-slopePart) + slope * slopePart;
		for (int y=0;y<bm->h;y++)
		{
			for (int x=0;x<bm->w;x++)
			{
				const float heightScale = 1.0f / (1<<16);
				float h = heightmap->at (x,y) * heightScale;

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
					norm_y = tangent.z * binormal.x - binormal.z * tangent.x;
				}

				// flatness=dotproduct of surface normal with up vector
				float slope = 1.0f - fabsf (norm_y);

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

	uint GenSphereBumpmap ()
	{
		uint id;
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
					int sz = sqrtf(32 * 32 - sx*sx - sy*sy);
					n = Vector3(sx,sy,sz);
					n.Normalize ();
				}

				// compress it into a color
				*(p++) = 255 * (n.x * 0.5f + 0.5f);
				*(p++) = 255 * (n.y * 0.5f + 0.5f);
				*(p++) = 255 * (n.z * 0.5f + 0.5f);
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
		FILE *tf = fopen(fn, "rb");
		if (tf) {
			fclose(tf);
			remove(fn);
		}
		bool r=ilSaveImage((ILstring)fn);
		ilDeleteImages(1,&out);
#endif
	}

};

