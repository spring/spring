/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring>
#include <assert.h>
#include "Rendering/GL/myGL.h"

#include "TerrainBase.h"
#include "TerrainNode.h"
#include "TerrainTexture.h"

namespace terrain {

	QuadRenderData::QuadRenderData()
		: normalMap(0)
		, normalMapW(0)
		, normalMapTexWidth(0)
		, vertexSize(0)
		, index(0)
		, used(false)
		, quad(0)
	{
	}

	QuadRenderData::~QuadRenderData()
	{
		// delete normal map
		if (normalMap) {
			glDeleteTextures(1, &normalMap);
			normalMap = 0;
		}
	}

	uint QuadRenderData::GetDataSize()
	{
		return normalMapW * normalMapW * 3 + vertexBuffer.GetSize();
	}

	RenderDataManager::RenderDataManager(Heightmap* rhm, QuadMap* rqm)
		: normalDataAllocates(0)
		, renderDataAllocates(0)
		, roothm(rhm)
		, rootQMap(rqm)
	{
	}

	RenderDataManager::~RenderDataManager()
	{
		for (size_t a = 0; a < freeRD.size(); a++)
			delete freeRD[a];
		for (size_t a = 0; a < qrd.size(); a++)
			delete qrd[a];
	}

	QuadRenderData* RenderDataManager::Allocate()
	{
		QuadRenderData* rd;
		if (!freeRD.empty()) {
			rd = freeRD.back();
			freeRD.pop_back();
		}
		else rd = new QuadRenderData;

		rd->index = qrd.size();
		qrd.push_back(rd);

		renderDataAllocates++;

		return rd;
	}

	void RenderDataManager::PruneFreeList(int maxFreeRD)
	{
		const size_t maxFreeRDUnsigned = (maxFreeRD < 0) ? 0 : maxFreeRD;
		while (freeRD.size() > maxFreeRDUnsigned) {
			delete freeRD.back();
			freeRD.pop_back();
		}
	}

	void RenderDataManager::Free(QuadRenderData* rd)
	{
		if (rd->index < qrd.size()-1) {
			qrd.back()->index = rd->index;
			std::swap(qrd.back(), qrd[rd->index]);
		}
		qrd.pop_back();

		freeRD.push_back(rd);
		rd->GetQuad()->renderData = 0;

		rd->GetQuad()->FreeCachedTexture();
	}

	void RenderDataManager::UpdateRect(int sx, int sy,int w,int h)
	{
		for (size_t a = 0; a < qrd.size(); a++)
		{
			QuadRenderData* rd = qrd[a];
			TQuad* q = rd->GetQuad();

			// rect vs rect collision:
			if (q->sqPos.x + q->width >= sx && q->sqPos.y + q->width >= sy &&
				q->sqPos.x <= sx + w && q->sqPos.y <= sy + h)
			{
				assert(q->renderData==qrd[a]);
				Free(q->renderData);
			}
		}
	}

	// delete all renderdata that is not used this frame and has maxlod < VBufMinDetail
	void RenderDataManager::FreeUnused()
	{
		for (int a = 0; a < qrd.size(); a++)
		{
			QuadRenderData* rd = qrd[a];

			if (rd->used) {
				rd->used = false;
				continue;
			}

			if (rd->GetQuad()->maxLodValue < VBufMinDetail) {
				Free(rd);
				a--;
			}
		}
	}

	void RenderDataManager::InitializeNode(TQuad* q)
	{
		assert(!q->renderData);

		QuadRenderData* rd = q->renderData = Allocate();

		// Allocate vertex data space
		const size_t vertexSize = q->GetVertexSize();
		if (static_cast<int>(vertexSize) != rd->vertexSize) {
			const size_t size = NUM_VERTICES * vertexSize;

			if (rd->vertexBuffer.GetSize() != size) {
				rd->vertexBuffer.Init(size);
			}
			rd->vertexSize = vertexSize;
		}

		// build the vertex buffer
		Vector3* v = (Vector3*)rd->vertexBuffer.LockData();

		uint vda = q->textureSetup->vertexDataReq; 		// vertex data requirements
		const Heightmap* hm = roothm->GetLevel(q->depth); // get the right heightmap level

		for (int y = q->hmPos.y; y <= (q->hmPos.y + QUAD_W); y++)
			for (int x = q->hmPos.x; x <= (q->hmPos.x + QUAD_W); x++)
			{
				*(v++) = Vector3(x * hm->squareSize, hm->atSynced(x, y), y * hm->squareSize);

				Vector3 tangent, binormal;
				CalculateTangents(hm, x,y, tangent, binormal);
				Vector3 normal = binormal.cross(tangent);
				normal.ANormalize();

				if (vda & VRT_Normal)
					*(v++) = normal;

				if (vda & VRT_TangentSpaceMatrix)
				{
					tangent.ANormalize();
					binormal.ANormalize();

					// orthonormal matrix, so inverse=transpose
					// Take the inverse of the tangent space -> world space transformation
					Vector3* tgs2ws = v;
					tgs2ws[0] = Vector3(tangent.x, binormal.x, normal.x);
					tgs2ws[1] = Vector3(tangent.y, binormal.y, normal.y);
					tgs2ws[2] = Vector3(tangent.z, binormal.z, normal.z);
					v += 3;
				}
			}
		rd->vertexBuffer.UnlockData();
		rd->SetQuad(q);
	}


	void RenderDataManager::InitializeNodeNormalMap(TQuad* q, int cfgNormalMapLevel)
	{
		QuadRenderData* rd = q->renderData;

		if (q->isLeaf()) {
			if (rd->normalMap) {
				glDeleteTextures(1, &rd->normalMap);
				rd->normalMap = 0;
				rd->normalMapW = 0;
			}
			return;
		}

		// find the right level heightmap to generate the normal map from
		Heightmap* hm = roothm;
		int level = 0;
		for (; hm->highDetail; hm = hm->highDetail, level++)
			if (level == q->depth + cfgNormalMapLevel) break;

		// calculate dimensions
		const int scale = 1 << (level - q->depth);
		size_t w = QUAD_W * scale + 1;
		const size_t h = w;
		const size_t texw = w;
		const int startx = q->hmPos.x * scale;

		// if not yet created, create a texture for it
		GLuint texture;

		if (rd->normalMap && (rd->normalMapW == w) && (rd->normalMapTexWidth == texw)) {
			texture = rd->normalMap;
			glBindTexture(GL_TEXTURE_2D, texture);
		} else {
			if (rd->normalMap)
				glDeleteTextures(1,&rd->normalMap);

			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

			rd->normalMap = texture;
			rd->normalMapW = w;
			rd->normalMapTexWidth = texw;
		}

		// allocate temporary storage for normals
		uchar* normals = new uchar[texw*texw*3];

		// calculate normals
		for (size_t y=0; y<h; y++) {
			const uchar* src = hm->GetNormal(startx, y + q->hmPos.y * scale);
			memcpy(&normals [3 * y * texw], src, 3 * w);
		}

		// fill texture
		glTexImage2D(GL_TEXTURE_2D, 0, 3, texw,texw,0, GL_RGB, GL_UNSIGNED_BYTE, normals);

		delete[] normals;
	}

};
