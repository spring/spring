/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "TerrainBase.h"
#include "TerrainVertexBuffer.h"
#include "TerrainNode.h"
#include "TerrainTexture.h"

#include "Rendering/Textures/Bitmap.h"

#include <cassert>

namespace terrain {

//-----------------------------------------------------------------------
// Heightmap class
//-----------------------------------------------------------------------

	Heightmap::Heightmap()
	{ 
		w = h = 0;
		lowDetail = highDetail = 0;
		normalData = 0;
		squareSize = 0.0f; 
	}
	Heightmap::~Heightmap()
	{ 
		delete[] normalData;
	}

	void Heightmap::Alloc(int W, int H)
	{
		w = W;
		h = H;
		dataSynced.resize(w * h);
		dataUnsynced.resize(w * h);
	}

	void Heightmap::LodScaleDown(Heightmap* dst)
	{
		// reduce 33x33 to 17x17
		dst->Alloc((w - 1) / 2 + 1, (h - 1) / 2 + 1);

		for (int y = 0; y < dst->h; y++) {
			const float* srcrowSynced = &dataSynced[y * 2 * w];
			const float* srcrowUnsynced = &dataUnsynced[y * 2 * w];

			for (int x = 0; x < dst->w; x++) {
				dst->dataSynced[y * dst->w + x] = srcrowSynced[x * 2];
				dst->dataUnsynced[y * dst->w + x] = srcrowUnsynced[x * 2];
			}
		}
	}

	Heightmap* Heightmap::CreateLowDetailHM()
	{
		lowDetail = new Heightmap;
		LodScaleDown(lowDetail);
		lowDetail->highDetail = this;
		return lowDetail;
	}


	void Heightmap::FindMinMax(int2 st, int2 size, float& minHgt, float& maxHgt, bool synced)
	{
		if (synced) {
			minHgt = maxHgt = atSynced(st.x, st.y);
		} else {
			minHgt = maxHgt = atUnsynced(st.x, st.y);
		}

		for (int y = st.y; y < st.y + size.y; y++)
			for (int x = st.x; x < st.x + size.x; x++) {
				if (synced) {
					minHgt = std::min(minHgt, atSynced(x, y));
					maxHgt = std::max(maxHgt, atSynced(x, y));
				} else {
					minHgt = std::min(minHgt, atUnsynced(x, y));
					maxHgt = std::max(maxHgt, atUnsynced(x, y));
				}
			}
	}

	// level > 0 returns a high detail HM
	// level < 0 returns a lower detail HM
	const Heightmap* Heightmap::GetLevel(int level) 
	{
		Heightmap* hm = this;
		if (level > 0)
			while (hm->highDetail && (level > 0)) {
				hm = hm->highDetail;
				level --;
			}
		else
			while (hm->lowDetail && (level < 0)) {
				hm = hm->lowDetail;
				level ++;
			}
		return hm;
	}

	void Heightmap::GenerateNormals()
	{
		normalData = new uchar[3 * w * h];

		uchar* cnorm = normalData;
		for (int y = 0; y < h;y++)
			for (int x = 0; x < w; x++) {
				Vector3 tangent, binormal;
				CalculateTangents(this, x, y, tangent, binormal);

				Vector3 normal = binormal.cross(tangent);
				normal.ANormalize();

				*(cnorm++) = (uchar)((normal.x * 0.5f + 0.5f) * 255);
				*(cnorm++) = (uchar)((normal.y * 0.5f + 0.5f) * 255);
				*(cnorm++) = (uchar)((normal.z * 0.5f + 0.5f) * 255);
			}
	}


	void Heightmap::UpdateLowerUnsynced(int sx, int sy, int w, int h)
	{
		if (!lowDetail)
			return;

		for (int x = sx / 2; x < (sx + w) / 2; x++)
			for (int y = sy / 2; y < (sy + h) / 2; y++) {
				lowDetail->dataSynced[y * lowDetail->w + x] = atSynced(x * 2, y * 2);
				lowDetail->dataUnsynced[y * lowDetail->w + x] = atUnsynced(x * 2, y * 2);
			}

		lowDetail->UpdateLowerUnsynced(sx / 2, sy / 2, w / 2, h / 2);
	}



//-----------------------------------------------------------------------
// Index calculater
//-----------------------------------------------------------------------

	IndexTable::IndexTable()
	{
		for (int a = 0; a < NUM_TABLES; a++)
			Calculate(a);
	}

#define TRI(a,b,c) {*(i++) = (a); *(i++) = b; *(i++) = c;}
#define VRT(x,y) ((y)*VERTC + (x))

	void IndexTable::Calculate(int sides)
	{
		IndexBuffer* buf = &buffers[sides];
		buf->Init(sizeof(index_t) * MAX_INDICES);
		index_t* begin = (index_t*)buf->LockData();
		index_t* i = begin;

		int ystart = 0, xstart = 0;
		int yend = QUAD_W, xend = QUAD_W;
		if (sides & left_bit) xstart++;
		if (sides & right_bit) xend--;
		if (sides & up_bit) ystart++;
		if (sides & down_bit) yend--;

		if (sides & up_bit) {
			for (int x = xstart; x < QUAD_W; x++) {
				if (x & 1) {
					TRI(x-1, x+1, VERTC+x);
					if (x < xend) TRI(x+1, VERTC+x+1, VERTC+x);
				} else {
					TRI(x, VERTC+x+1, VERTC+x);
				}
			}
		}

		for (int y = ystart; y < QUAD_W; y++)
		{
			if (sides & left_bit) {
				if (y & 1) {
					TRI(VRT(0,y-1),VRT(1,y),VRT(0,y+1));
					if (y<yend) TRI(VRT(1,y),VRT(1,y+1),VRT(0,y+1));
				} else {
					TRI(VRT(0,y),VRT(1,y),VRT(1,y+1));
				}
			}

			if ((y >= ystart) && (y < yend)) {
				for (int x = xstart; x < xend; x++)
				{
					TRI(y*VERTC + x, y*VERTC + x + 1, (y + 1)*VERTC + x + 1);
					TRI(y*VERTC + x, (y + 1)*VERTC + x + 1, (y + 1)*VERTC + x);
				}
			}

			if (sides & right_bit) {
				int x = QUAD_W - 1;
				if (y & 1) {
					TRI(VRT(x+1, y-1), VRT(x+1, y+1), VRT(x, y));
					if (y < yend) TRI(VRT(x, y), VRT(x+1, y+1), VRT(x, y+1));
				} else {
					TRI(VRT(x, y), VRT(x+1, y), VRT(x, y+1));
				}
			}
		}

		if (sides & down_bit) {
			int y = QUAD_W - 1;
			for (int x = xstart; x < QUAD_W; x++) {
				if (x & 1) {
					TRI(VRT(x, y), VRT(x+1, y+1), VRT(x-1, y+1));
					if (x < xend) TRI(VRT(x, y), VRT(x+1,y), VRT(x+1, y+1));
				} else {
					TRI(VRT(x, y), VRT(x+1, y), VRT(x, y+1));
				}
			}
		}

		size[sides] = i - begin;
		assert(size[sides] <= MAX_INDICES);
		buf->UnlockData();
	}

#undef VRT
#undef TRI


	
	void SetTexGen(float scale)
	{
		float tgv[4] = { scale, scale, 0.0f, 0.0f };
		SetTexCoordGen(tgv);
	}

//-----------------------------------------------------------------------
// AlphaImage
//-----------------------------------------------------------------------
	AlphaImage::AlphaImage()
	{
		w = h = 0;
		data = 0;
		lowDetail = highDetail = 0;
	}
	AlphaImage::~AlphaImage()
	{ delete[] data; }

	void AlphaImage::Alloc(int W, int H)
	{
		w = W;
		h = H;
		data = new float[w*h];
		std::fill(data, data + w*h, 0.0f);
	}

	bool AlphaImage::CopyFromBitmap(CBitmap& bm)
	{
		if (bm.channels != 1 || bitmap.compressed)
			return false;

		Alloc(bm.xsize, bm.ysize);
		for (int y = 0; y < h; y++)
			for (int x = 0; x < w; x++)
				at(x, y) = bm.mem[w*y + x] * (1.0f / 255.0f);

		return true;
	}

	AlphaImage* AlphaImage::CreateMipmap()
	{
		if (w == 1 && h == 1)
			return 0;

		int nw = w/2;
		int nh = h/2;
		if (!nw) { nw = 1; }
		if (!nh) { nh = 1; }

		AlphaImage* mipmap = new AlphaImage;
		mipmap->Alloc(nw, nh);

		// Scale X&Y
		if (w > 1 && h > 1) {
			for (int y = 0; y < nh; y++)
				for (int x = 0; x < nw; x++)
					mipmap->at(x,y) = (at(x*2, y*2) + at(x*2+1, y*2) + at(x*2, y*2+1) + at(x*2+1, y*2+1)) * 0.25f;
		} else if (w == 1 && h > 1) { // Scale Y only
			for (int y = 0; y < nh; y++)
				for (int x = 0; x < nw; x++)
					mipmap->at(x, y) = (at(x, y*2) + at(x, y*2+1)) * 0.5f;
		} else { // if (w > 1 && h == 1) {
			for (int y = 0; y < nh; y++)  // Scale X only
				for (int x = 0; x < nw; x++)
					mipmap->at(x, y) = (at(x*2, y) + at(x*2+1, y)) * 0.5f;
		}
		return mipmap;
	}

	// NO CLIPPING!
	void AlphaImage::Blit(AlphaImage* dst, int srcx,int srcy,int dstx,int dsty,int w,int h)
	{
		for (int y = 0; y < h; y++)
			for (int x = 0; x < w; x++)
				dst->at(x + dstx, y + dsty) = at(x + srcx, y + srcy);
	}

	bool AlphaImage::Save(const char* fn)
	{
		SaveImage(fn, 1, GL_FLOAT, w,h, data);
		return true;
	}

	AlphaImage::AreaTestResult AlphaImage::TestArea(int xstart, int ystart, int xend, int yend, float epsilon)
	{
		bool allOne = true;
		bool allZero = true;

		for (int y = ystart; y < yend; y++)
			for (int x = xstart; x < xend; x++)
			{
				float v = at(x, y);
				if (v > epsilon) allZero = false;
				if (v < 1.0f - epsilon) allOne = false;
			}

		if (allOne) return AREA_ONE;
		if (allZero) return AREA_ZERO;
		return AREA_MIXED;
	}

	int TextureUsage::AddTextureCoordRead(int maxCoords, BaseTexture* texture)
	{
		if (texture->ShareTexCoordUnit())
		{
			for (size_t a = 0; a < coordUnits.size(); a++) {
				BaseTexture* t = coordUnits[a];
				if (t == texture)
					return (int)a;
				if (t->ShareTexCoordUnit() && (t->tilesize.x == texture->tilesize.x) && (t->tilesize.y == texture->tilesize.y))
					return (int)a;
			}
		}
		else {
			for (size_t a = 0; a < coordUnits.size(); a++)
				if (coordUnits[a] == texture) return (int)a;
		}

		if ((maxCoords >= 0) && (maxCoords == (int)coordUnits.size()))
			return -1;
		
		coordUnits.push_back(texture);
		return (int)coordUnits.size() - 1;
	}

	int TextureUsage::AddTextureRead(int maxUnits, BaseTexture* texture)
	{
		if ((maxUnits >= 0) && (maxUnits == (int)texUnits.size()))
			return -1;

		for (size_t a = 0; a < texUnits.size(); a++)
			if (texUnits[a] == texture) return (int)a;
		texUnits.push_back(texture);
		return (int)texUnits.size() - 1;
	}
};
