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
#include <cstdarg>
#include <cstring>

#include "TerrainBase.h"
#include "Terrain.h"
#include "TerrainVertexBuffer.h"

#include "TdfParser.h"

#include <deque>
#include <fstream>
#include <IL/il.h>
#include "TerrainTexture.h"
#include "TerrainNode.h"
#include "FileSystem/FileHandler.h"
#include "Util.h"
#include <assert.h>

// define this for big endian machines
//#define SWAP_SHORT

namespace terrain {

	using namespace std;

	Config::Config()
	{
		cacheTextures=false;
		useBumpMaps=false;
		terrainNormalMaps=false;
		detailMod=2.0f;
		normalMapLevel=2;
		cacheTextureSize = 128;
		useShadowMaps = false;
		anisotropicFiltering = 0;
		forceFallbackTexturing = false;
		maxLodLevel = 4;
	}


	void ILoadCallback::PrintMsg (const char *fmt, ...)
	{
		char buf[256];

		va_list l;
		va_start (l, fmt);
		VSNPRINTF(buf, sizeof(buf), fmt, l);
		va_end (l);

		Write (buf);
	}

//-----------------------------------------------------------------------
// Quad map - stores a 2D map of the quad nodes,
//            for quick access of nabours
//-----------------------------------------------------------------------

	void QuadMap::Alloc (int W)
	{
		w=W;
		map = new TQuad*[w*w];
		memset (map, 0, sizeof(TQuad*) * w*w);
	}

	void QuadMap::Fill (TQuad *q)
	{
		At (q->qmPos.x, q->qmPos.y) = q;

		if (!q->isLeaf ()) {
			assert (highDetail);

			for (int a=0;a<4;a++)
				highDetail->Fill (q->childs[a]);
		}
	}


//-----------------------------------------------------------------------
// Terrain Quadtree Node
//-----------------------------------------------------------------------

	TQuad::TQuad ()
	{
		parent=0;
		for (int a=0;a<4;a++)
			childs[a]=0;
		depth=width=0;
		drawState=NoDraw;
		renderData=0;
		textureSetup=0;
		cacheTexture=0;
		normalData=0;
	}

	TQuad::~TQuad ()
	{
		if (!isLeaf()) {
			for (int a=0;a<4;a++)
				delete childs[a];
		}

		// delete cached texture
		if (cacheTexture) {
			glDeleteTextures(1,&cacheTexture);
			cacheTexture = 0;
		}
	}

	void TQuad::Build (Heightmap *hm, int2 sqStart, int2 hmStart, int2 quadPos, int w, int d)
	{
		// create child nodes if necessary
		if (hm->highDetail) {
			for (int a=0;a<4;a++) {
				childs[a] = new TQuad;
				childs[a]->parent = this;

				int2 sqPos (sqStart.x + (a&1)*w/2, sqStart.y + (a&2)*w/4); // square pos
				int2 hmPos (hmStart.x*2 + (a&1)*QUAD_W, hmStart.y*2 + (a&2)*QUAD_W/2); // heightmap pos
				int2 cqPos (quadPos.x*2 + (a&1), quadPos.y*2 + (a&2)/2);// child quad pos

				childs[a]->Build (hm->highDetail, sqPos, hmPos, cqPos, w/2, d + 1);
			}
		}

		float minH, maxH;
		hm->FindMinMax (hmStart, int2(QUAD_W,QUAD_W), minH, maxH);

		start = Vector3 (sqStart.x, 0.0f, sqStart.y) * SquareSize;
		end = Vector3 (sqStart.x+w, 0.0f, sqStart.y+w) * SquareSize;

		start.y = minH;
		end.y = maxH;

		hmPos = hmStart;
		qmPos = quadPos;
		sqPos = sqStart;
		depth = d;
		width = w;
	}

	int TQuad::GetVertexSize ()
	{
		// compile-time assert
		typedef int vector_should_be_12_bytes [(sizeof(Vector3) == 12) ? 1 : -1];
		int vertexSize = 12;

		uint vda = textureSetup->vertexDataReq;
		if (vda & VRT_Normal)
			vertexSize += 12;
		if (vda & VRT_TangentSpaceMatrix)
			vertexSize += 12 * 3;
		return vertexSize;
	}

	void TQuad::Draw (IndexTable *indexTable, bool onlyPositions, int lodState)
	{
		uint vda = textureSetup->vertexDataReq;
		int vertexSize = GetVertexSize();

		// Bind the vertex buffer components
		Vector3 *vbuf = (Vector3 *)renderData->vertexBuffer.Bind();
		glEnableClientState (GL_VERTEX_ARRAY);
		glVertexPointer (3, GL_FLOAT, vertexSize, vbuf ++);
		if (vda & VRT_Normal)
		{
			if (!onlyPositions) {
				glEnableClientState (GL_NORMAL_ARRAY);
				glNormalPointer (GL_FLOAT, vertexSize, vbuf);
			}
			vbuf ++;
		}
		if (vda & VRT_TangentSpaceMatrix)
		{
			if (!onlyPositions) {
				assert (textureSetup->currentShaderSetup);
				textureSetup->currentShaderSetup->BindTSM (vbuf, vertexSize);
			}
			vbuf += 3;
		}

		// Bind the index buffer and render
		IndexBuffer& ibuf = indexTable->buffers [lodState];
		glDrawElements (GL_TRIANGLES, indexTable->size [lodState], indexTable->IndexType(), ibuf.Bind());
		ibuf.Unbind ();

		// Unbind the vertex buffer
		if (!onlyPositions) {
			if (vda&VRT_Normal) glDisableClientState (GL_NORMAL_ARRAY);
			if (vda&VRT_TangentSpaceMatrix) textureSetup->currentShaderSetup->UnbindTSM();
		}
		glDisableClientState (GL_VERTEX_ARRAY);
		renderData->vertexBuffer.Unbind ();
	}

	bool TQuad::InFrustum (Frustum *f)
	{
		Vector3 boxSt=start, boxEnd=end;
		return f->IsBoxVisible (boxSt, boxEnd) != Frustum::Outside;
	}

	// Calculates the exact nearest point, not just one of the box'es vertices
	void NearestBoxPoint(const Vector3 *min, const Vector3 *max, const Vector3 *pos, Vector3 *out)
	{
		Vector3 mid = (*max + *min) * 0.5f;
		if(pos->x < min->x) out->x = min->x;
		else if(pos->x > max->x) out->x = max->x;
		else out->x = pos->x;
		if(pos->y < min->y) out->y = min->y;
		else if(pos->y > max->y) out->y = max->y;
		else out->y = pos->y;
		if(pos->z < min->z) out->z = min->z;
		else if(pos->z > max->z) out->z = max->z;
		else out->z = pos->z;
	}

	float TQuad::CalcLod (const Vector3& campos)
	{
		Vector3 nearest;
		NearestBoxPoint (&start, &end, &campos, &nearest);

		float nodesize = end.x - start.x;
	//	float sloped = 0.01f * (1.0f + end.y - start.y);
		nearest -= campos;
		//return sqrtf(sloped) * nodesize / (nearest.length () + 0.1f);
		return nodesize / (nearest.Length () + 0.1f);
	}

	void TQuad::CollectNodes(std::vector<TQuad*>& quads)
	{
		if (!isLeaf()) {
			for (int a=0;a<4;a++)
				childs[a]->CollectNodes(quads);
		}
		quads.push_back(this);
	}

	void TQuad::FreeCachedTexture()
	{
		if (cacheTexture) {
			glDeleteTextures(1,&cacheTexture);
			cacheTexture = 0;
		}
	}

	TQuad *TQuad::FindSmallestContainingQuad2D(const Vector3& pos, float range, int maxdepth)
	{
		if (depth < maxdepth)
		{
			for (int a=0;a<4;a++) {
				TQuad *r = childs[a]->FindSmallestContainingQuad2D(pos,range,maxdepth);
				if (r) return r;
			}
		}
		if (start.x <= pos.x - range && end.x >= pos.x + range &&
			start.z <= pos.z - range && end.z >= pos.z + range)
			return this;
		return 0;
	}

//-----------------------------------------------------------------------
// Terrain Main class
//-----------------------------------------------------------------------

	Terrain::Terrain()
	{
		heightmap = 0;
		quadtree = 0;
		debugQuad = 0;
		indexTable = 0;
		texturing = 0;
		lowdetailhm = 0;
		shadowMap = 0;
		renderDataManager = 0;
		logUpdates=false;
		nodeUpdateCount=0;
		curRC = 0;
		activeRC = 0;
		quadTreeDepth = 0;
	}
	Terrain::~Terrain()
	{
		delete renderDataManager;
        delete texturing;
		while (heightmap) {
			Heightmap *p = heightmap;
			heightmap = heightmap->lowDetail;
			delete p;
		}
		for (size_t a=0;a<qmaps.size();a++)
			delete qmaps[a];
		qmaps.clear();
		delete quadtree;
		delete indexTable;
	}

	void Terrain::SetShadowMap (uint shadowTex)
	{
		shadowMap = shadowTex;
	}

	// used by ForceQueue to queue quads
	void Terrain::QueueLodFixQuad (TQuad *q)
	{
		if (q->drawState == TQuad::Queued)
			return;

		if (q->drawState == TQuad::NoDraw) {
			// make sure the parent is drawn
			assert (q->parent);
			QueueLodFixQuad (q->parent);

			// change the queued quad to a parent quad
			q->parent->drawState = TQuad::Parent;
			for (int a=0;a<4;a++) {
				TQuad *ch = q->parent->childs [a];

				updatequads.push_back(ch);
				ch->drawState = TQuad::Queued;
				UpdateLodFix (ch);
			}
		}
	}

	void Terrain::ForceQueue (TQuad *q)
	{
		// See if the quad is culled against the view frustum
		TQuad *p = q->parent;
		while (p) {
			if (p->drawState == TQuad::Culled)
				return;
			if (p->drawState == TQuad::Queued)
				break;
			p = p->parent;
		}
		// Quad is not culled, so make sure it is drawn
		QueueLodFixQuad (q);
	}

	inline void Terrain::CheckNabourLod (TQuad *q, int xOfs, int yOfs)
	{
		QuadMap *qm = qmaps[q->depth];
		TQuad *nb = qm->At (q->qmPos.x+xOfs, q->qmPos.y+yOfs);

		// Check the state of the nabour parent (q is already the parent),
		if (nb->drawState == TQuad::NoDraw) {
			// a parent of the node is either culled or drawn itself
			ForceQueue (nb);
			return;
		}
	    // Parent: a child node of the nabour is drawn, which will take care of LOD gaps fixing
		// Queued: drawn itself, so the index buffer can fix gaps for that
		// Culled: no gap fixing required
	}

	// Check nabour parent nodes to see if they need to be drawn to fix LOD gaps of this node.
	void Terrain::UpdateLodFix (TQuad *q)
	{
		if (q->drawState==TQuad::Culled)
			return;

		if (q->drawState==TQuad::Parent) {
			for(int a=0;a<4;a++)
				UpdateLodFix(q->childs [a]);
		}
		else if (q->parent) {
			// find the nabours, and make sure at least them or their parents are drawn
			TQuad *parent = q->parent;
			QuadMap *qm = qmaps[parent->depth];
			if (parent->qmPos.x>0) CheckNabourLod (parent, -1, 0);
			if (parent->qmPos.y>0) CheckNabourLod (parent, 0, -1);
			if (parent->qmPos.x<qm->w-1) CheckNabourLod (parent, 1, 0);
			if (parent->qmPos.y<qm->w-1) CheckNabourLod (parent, 0, 1);
		}
	}

	// Handle visibility with the frustum, and select LOD based on distance to camera and LOD setting
	void Terrain::QuadVisLod (TQuad *q)
	{
		if (q->InFrustum (&frustum)) {
			// Node is visible, now determine if this LOD is suitable, or that a higher LOD should be used.
			float lod = config.detailMod * q->CalcLod (curRC->cam->pos);
			if (q->depth < quadTreeDepth-config.maxLodLevel || (lod > 1.0f && !q->isLeaf())) {
				q->drawState=TQuad::Parent;
				for (int a=0;a<4;a++)
					QuadVisLod (q->childs[a]);
			} else q->drawState=TQuad::Queued;
			updatequads.push_back (q);

			// update max lod value
			if (q->maxLodValue < lod)
				q->maxLodValue = lod;
		} else {
			q->drawState=TQuad::Culled;
			culled.push_back (q);
		}
	}

	static inline bool QuadSortFunc (const QuadRenderInfo& q1, const QuadRenderInfo& q2)
	{
		return q1.quad->textureSetup->sortkey < q2.quad->textureSetup->sortkey;
	}


	// update quad node drawing list
	void Terrain::Update ()
	{
		nodeUpdateCount = 0;

		renderDataManager->ClearStat();

		// clear LOD values of previously used renderquads
		for (size_t a=0;a<contexts.size();a++)
		{
			RenderContext *rc = contexts[a];
			for (size_t n = 0; n < rc->quads.size();n++) {
				TQuad *q = rc->quads [n].quad;

				q->maxLodValue = 0.0f;
				if (q->renderData)
					q->renderData->used = true;
			}
		}

		for (size_t ctx=0;ctx<contexts.size();ctx++)
		{
			RenderContext *rc = curRC = contexts[ctx];

			// Update the frustum based on the camera, to cull away TQuad nodes
			//Camera *camera = rc->cam;
		//	frustum.CalcCameraPlanes (&camera->pos, &camera->right, &camera->up, &camera->front, camera->fov, camera->aspect);
			//assert (camera->pos.x == 0.0f || frustum.IsPointVisible (camera->pos + camera->front * 20)==Frustum::Inside);

			// determine the new set of quads to draw
			QuadVisLod (quadtree);

			// go through nabours to make sure there is a maximum of one LOD difference between nabours
			UpdateLodFix (quadtree);

			if (debugQuad)
				ForceQueue (debugQuad);

			// update lod state and copy the rendering list to the render context...
			rc->quads.clear();
			for (size_t a=0;a<updatequads.size();a++)
			{
				if (updatequads[a]->drawState == TQuad::Queued)
				{
					TQuad *q = updatequads[a];
					// calculate lod state
					QuadMap *qm = qmaps[q->depth];
					int ls=0;
					if (q->qmPos.x>0 && qm->At (q->qmPos.x-1,q->qmPos.y)->drawState == TQuad::NoDraw) ls |= left_bit;
					if (q->qmPos.y>0 && qm->At (q->qmPos.x,q->qmPos.y-1)->drawState == TQuad::NoDraw) ls |= up_bit;
					if (q->qmPos.x<qm->w-1 && qm->At (q->qmPos.x+1,q->qmPos.y)->drawState == TQuad::NoDraw) ls |= right_bit;
					if (q->qmPos.y<qm->w-1 && qm->At (q->qmPos.x,q->qmPos.y+1)->drawState == TQuad::NoDraw) ls |= down_bit;

					rc->quads.push_back (QuadRenderInfo());
					rc->quads.back().quad = q;
					rc->quads.back().lodState = ls;
					if (q->renderData) q->renderData->used = true;
				}
			}
			// sort rendering quads based on sorting key
			sort (rc->quads.begin(), rc->quads.end(), QuadSortFunc);

			// the active set of quads is determined, so their draw-states can be reset for the next context
			for (size_t a=0;a<culled.size();a++)
				culled[a]->drawState = TQuad::NoDraw;
			culled.clear();

			// clear the list of queued quads
			for (size_t a=0;a<updatequads.size();a++)
				updatequads[a]->drawState = TQuad::NoDraw;
			updatequads.clear ();
		}

		renderDataManager->FreeUnused ();

		for (size_t ctx=0;ctx<contexts.size();ctx++)
		{
			RenderContext *rc = curRC = contexts[ctx];

			// allocate required vertex buffers
			for (size_t a=0;a<rc->quads.size();a++)
			{
				TQuad *q = rc->quads[a].quad;

				if (!q->renderData) {
					renderDataManager->InitializeNode (q);

					if (config.terrainNormalMaps && rc->needsNormalMap)
						renderDataManager->InitializeNodeNormalMap(q, config.normalMapLevel);

					nodeUpdateCount ++;
				}
			}
		}

		if (logUpdates) {
			if (nodeUpdateCount) d_trace ("NodeUpdates: %d, NormalDataAllocs:%d, RenderDataAllocs:%d\n",
				nodeUpdateCount, renderDataManager->normalDataAllocates, renderDataManager->renderDataAllocates);
		}
		curRC = 0;
	}

	void Terrain::DrawSimple ()
	{
		glEnable(GL_CULL_FACE);

		for (size_t a=0;a<activeRC->quads.size();a++)
		{
			QuadRenderInfo *q = &activeRC->quads[a];
			q->quad->Draw (indexTable, true, q->lodState);
		}
	}

	void Terrain::DrawOverlayTexture (uint tex)
	{
		glEnable (GL_TEXTURE_2D);
		glBindTexture (GL_TEXTURE_2D, tex);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE, GL_MODULATE);
        SetTexGen (1.0f / (heightmap->w * SquareSize));

		DrawSimple ();

		glDisable (GL_TEXTURE_GEN_S);
		glDisable (GL_TEXTURE_GEN_T);
		glDisable (GL_TEXTURE_2D);
	}

	void DrawNormals (TQuad *q, Heightmap *hm)
	{
		glBegin (GL_LINES);
		for (int y=q->hmPos.y;y<q->hmPos.y+QUAD_W;y++)
			for (int x=q->hmPos.x;x<q->hmPos.x+QUAD_W;x++)
			{
				Vector3 origin (x * hm->squareSize, hm->HeightAt (x,y), y * hm->squareSize);

				Vector3 tangent, binormal;
				CalculateTangents(hm, x,y, tangent, binormal);

				Vector3 normal = binormal.cross(tangent);
				normal.ANormalize();
				binormal.ANormalize ();
				tangent.ANormalize ();

				glColor3f (1,1,0);
				glVertex3fv(&origin[0]);
				glVertex3fv(&(origin + normal * 7)[0]);
				glColor3f (1,0,0);
				glVertex3fv(&origin[0]);
				glVertex3fv(&(origin + binormal * 7)[0]);
				glColor3f (0,0,1);
				glVertex3fv(&origin[0]);
				glVertex3fv(&(origin + tangent * 7)[0]);
			}
		glEnd();
		glColor3f (1,1,1);

	}

	void Terrain::Draw ()
	{
		const float diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, diffuse);

		glEnable(GL_CULL_FACE);
		glColor3f(1.0f,1.0f,1.0f);

		if (config.cacheTextures)
		{
			// draw all terrain nodes using their cached textures
			glEnable (GL_TEXTURE_GEN_S);
			glEnable (GL_TEXTURE_GEN_T);
			glTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

			glEnable (GL_TEXTURE_2D);
			glDisable (GL_LIGHTING);

			for (size_t a=0;a<activeRC->quads.size();a++)
			{
				TQuad *q = activeRC->quads[a].quad;

				// set quad texture
				glBindTexture (GL_TEXTURE_2D, q->cacheTexture);

				// set texture gen
				float ht = (q->end.x-q->start.x) / ( 2 * config.cacheTextureSize );

				float v[4] = {0.0f, 0.0f, 0.0f, 0.0f};
				v[0] = 1.0f / (q->end.x - q->start.x + ht * 2);
				v[3] = -v[0] * (q->start.x - ht);
				glTexGenfv (GL_S, GL_OBJECT_PLANE, v);
				v[0] = 0.0f;
				v[2] = 1.0f / (q->end.z - q->start.z + ht * 2);
				v[3] = -v[2] * (q->start.z - ht);
				glTexGenfv (GL_T, GL_OBJECT_PLANE, v);

				// draw
				q->Draw (indexTable, false, activeRC->quads[a].lodState);
			}

			glDisable (GL_TEXTURE_2D);
			glDisable (GL_TEXTURE_GEN_S);
			glDisable (GL_TEXTURE_GEN_T);
		}
		else  // no texture caching, so use the texturing system directly
		{
			const int numPasses = texturing->NumPasses();

			glEnable(GL_DEPTH_TEST);

			texturing->BeginTexturing();

			for (int pass = 0; pass < numPasses; pass++) {
				bool skipNodes = false;

				texturing->BeginPass(pass);

				for (size_t a = 0; a < activeRC->quads.size(); a++) {
					TQuad *q = activeRC->quads[a].quad;

					// Setup node texturing
					skipNodes = !texturing->SetupNode (q, pass);

					assert (q->renderData);

					if (!skipNodes) {
						// Draw the node
						q->Draw(indexTable, false, activeRC->quads[a].lodState);
					}
				}

				texturing->EndPass();
			}

			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);

			texturing->EndTexturing();
		}

		if (debugQuad)
			DrawNormals (debugQuad, lowdetailhm->GetLevel (debugQuad->depth));
	}

	void Terrain::CalcRenderStats (RenderStats& stats, RenderContext *ctx)
	{
		if (!ctx)
			ctx = activeRC;

		stats.cacheTextureSize = 0;
		stats.renderDataSize = 0;
		stats.tris = 0;

		for (size_t a=0;a<ctx->quads.size();a++) {
			TQuad *q = ctx->quads[a].quad;
			if (q->cacheTexture)
				stats.cacheTextureSize += config.cacheTextureSize * config.cacheTextureSize * 3;
			stats.renderDataSize += q->renderData->GetDataSize();
			stats.tris += MAX_INDICES/3;
		}
		stats.passes = texturing->NumPasses ();
	}

#ifndef TERRAINRENDERERLIB_EXPORTS
	void Terrain::DebugPrint (IFontRenderer *fr)
	{
		const float s=16.0f;
		if (debugQuad) {
			fr->printf (0, 30, s, "Selected quad: (%d,%d) on depth %d. Lod=%3.3f",
				debugQuad->qmPos.x, debugQuad->qmPos.y, debugQuad->depth, config.detailMod * debugQuad->CalcLod (activeRC->cam->pos));
		}
		RenderStats stats;
		CalcRenderStats(stats);
		fr->printf (0, 46, s, "Rendered nodes: %d, tris: %d, VBufSize: %d(kb), TotalRenderData(kb): %d, DetailMod: %g, CacheTextureMemory: %d",
			activeRC->quads.size(), stats.tris, VertexBuffer::TotalSize()/1024, stats.renderDataSize/1024, config.detailMod, stats.cacheTextureSize);
		fr->printf (0, 60, s, "NodeUpdateCount: %d, RenderDataAlloc: %d, #RenderData: %d",
			nodeUpdateCount, renderDataManager->normalDataAllocates, renderDataManager->QuadRenderDataCount ());
		texturing->DebugPrint(fr);
	}
#endif

	TQuad* FindQuad (TQuad *q, const Vector3& cpos)
	{
		if (cpos.x >= q->start.x && cpos.z >= q->start.z &&
			cpos.x < q->end.x && cpos.z < q->end.z)
		{
			if (q->isLeaf ()) return q;

			for (int a=0;a<4;a++)  {
				TQuad *r = FindQuad (q->childs[a], cpos);
				if (r) return r;
			}
		}
		return 0;
	}

	void Terrain::RenderNodeFlat (int x,int y,int depth)
	{
		RenderNode (qmaps[depth]->At (x,y));
	}

	void Terrain::RenderNode (TQuad *q)
	{
		// setup projection matrix, so the quad is exactly mapped onto the viewport
		glMatrixMode (GL_PROJECTION);
		glPushMatrix ();
		glLoadIdentity ();
		glOrtho (q->start.x, q->end.x, q->start.z, q->end.z, -10000.0f, 100000.0f);
		glColor3f(1.f,1.f,1.f);

		if (!q->renderData)
			renderDataManager->InitializeNode (q);

		texturing->BeginTexturing();
		// render to the framebuffer
		for (size_t p=0; p < q->textureSetup->renderSetup[0]->passes.size(); p++)
		{
			texturing->BeginPass(p);
			if (texturing->SetupNode (q, p))
			{
				glBegin(GL_QUADS);
				glVertex3f (q->start.x, 0.0f, q->start.z);
				glVertex3f (q->end.x, 0.0f, q->start.z);
				glVertex3f (q->end.x, 0.0f, q->end.z);
				glVertex3f (q->start.x, 0.0f, q->end.z);
				glEnd();
			}
			texturing->EndPass();
		}
		texturing->EndTexturing();
		glMatrixMode (GL_PROJECTION);
		glPopMatrix ();
	}

	void Terrain::CacheTextures ()
	{
		if (!config.cacheTextures)
			return;

		glPushAttrib (GL_VIEWPORT_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport (0, 0, config.cacheTextureSize, config.cacheTextureSize);

		glMatrixMode (GL_MODELVIEW);
		glPushMatrix ();
		glLoadIdentity ();

		float m[16];
		std::fill(m,m+16,0.0f);
		m[0]=m[6]=m[9]=m[15]=1.0f;
		glLoadMatrixf (m);

		glDepthMask (GL_FALSE);
		glDisable (GL_DEPTH_TEST);
		glDisable (GL_CULL_FACE);

		glDisable(GL_LIGHTING);

		// Make sure every node that might be drawn, has a cached texture
		for (size_t ctx=0;ctx<contexts.size();ctx++)
		{
			RenderContext *rc = contexts[ctx];
			if (!rc->needsTexturing)
				continue;

			for (size_t a=0;a<rc->quads.size();a++)
			{
				TQuad *q = rc->quads[a].quad;

				if (q->cacheTexture)
					continue;

				RenderNode (q);

				// copy it to the texture
				glGenTextures(1, &q->cacheTexture);
				glBindTexture (GL_TEXTURE_2D, q->cacheTexture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
				if (config.anisotropicFiltering > 0.0f)
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, config.anisotropicFiltering);
				glCopyTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, 0,0, config.cacheTextureSize, config.cacheTextureSize, 0);
			}
		}

		glMatrixMode (GL_MODELVIEW);
		glPopMatrix ();

		// restore viewport and depth buffer states
		glPopAttrib ();
	}

	void Terrain::DebugEvent (const std::string& event)
	{
		if (event == "t_detail_inc")
			config.detailMod *= 1.2f;
		if (event == "t_detail_dec")
			config.detailMod /= 1.2f;
		if (event == "t_debugquad") {
			if (debugQuad)
				debugQuad=0;
			else
				debugQuad=FindQuad(quadtree, activeRC->cam->pos);
			return;
		}
//		if (key == 'l')
//			logUpdates=!logUpdates;
		texturing->DebugEvent (event);
	}

	void Terrain::Load(const TdfParser& tdf, LightingInfo *li, ILoadCallback *cb)
	{
		string basepath = "MAP\\TERRAIN\\";

		// validate configuration
		if (config.cacheTextures)
			config.useBumpMaps = false;

		if (config.anisotropicFiltering != 0) {
			if (!GLEW_EXT_texture_filter_anisotropic)
				config.anisotropicFiltering = 0.0f;
			else {
				float maxAnisotropy=0.0f;
				glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,&maxAnisotropy);
				if (config.anisotropicFiltering > maxAnisotropy)
					config.anisotropicFiltering = maxAnisotropy;
			}
		}

		// load heightmap
		string heightmapName;
		tdf.GetDef(heightmapName, string(), basepath + "Heightmap");
		if (heightmapName.empty ())
			throw content_error("No heightmap given");

		if (cb) cb->PrintMsg ("loading heightmap (%s)...", heightmapName.c_str());
		string extension = heightmapName.substr (heightmapName.length ()-3,heightmapName.length ());
		if (extension == "raw") heightmap = LoadHeightmapFromRAW (heightmapName, cb);
		else heightmap = LoadHeightmapFromImage (heightmapName,cb);

		if (!heightmap)
			throw content_error("Failed to load heightmap " + heightmapName);

		d_trace("heightmap size: %dx%d\n", heightmap->w, heightmap->h);

		if (heightmap->w-1 != atoi(tdf.SGetValueDef("","MAP\\GameAreaW").c_str()) ||
			heightmap->h-1 != atoi(tdf.SGetValueDef("","MAP\\GameAreaH").c_str()))
		{
			char hmdims[32];
			SNPRINTF(hmdims,32,"%dx%d", heightmap->w, heightmap->h);
			throw content_error("Map size (" + string(hmdims) + ") should be equal to GameAreaW and GameAreaH");
		}

		float hmScale = atof (tdf.SGetValueDef ("1000", basepath + "HeightScale").c_str());
		float hmOffset = atof (tdf.SGetValueDef ("0", basepath + "HeightOffset").c_str());

		// apply scaling and offset to the heightmap
		for (int y=0;y<heightmap->w*heightmap->h;y++)
			heightmap->data[y]=heightmap->data[y]*hmScale + hmOffset;

		if (cb) cb->PrintMsg ("initializing heightmap renderer...");

		// create a set of low resolution heightmaps
		int w=heightmap->w-1;
		Heightmap *prev = heightmap;
		quadTreeDepth = 0;
		while (w>QUAD_W) {
			prev = prev->CreateLowDetailHM();
			quadTreeDepth++;
			w/=2;
		}
		lowdetailhm = prev;
		assert((1<<quadTreeDepth)*(lowdetailhm->w-1) == heightmap->w-1);

		// set heightmap squareSize for all lod's
		heightmap->squareSize = SquareSize;
		for (Heightmap *lod = heightmap->lowDetail; lod; lod = lod->lowDetail)
			lod->squareSize = lod->highDetail->squareSize * 2.0f;

		// build a quadtree and a vertex buffer for each quad tree
		// prev is now the lowest detail heightmap, and will be used for the root quad tree node
		quadtree = new TQuad;
		quadtree->Build (lowdetailhm, int2(), int2(), int2(), heightmap->w-1, 0);

		// create a quad map for each LOD level
		Heightmap *cur = prev;
		QuadMap *qm = 0;
		while (cur) {
			if (qm) {
				qm->highDetail = new QuadMap;
				qm->highDetail->lowDetail = qm;
				qm = qm->highDetail;
			} else {
				qm = new QuadMap;
			}
			qm->Alloc ((cur->w-1)/QUAD_W);
			qmaps.push_back(qm);
			cur = cur->highDetail;
		}

		// fill the quad maps (qmaps.front() is now the lowest detail quadmap)
		qmaps.front()->Fill (quadtree);

		// generate heightmap normals
		if (cb) cb->PrintMsg ("  generating terrain normals for shading...");
		for (Heightmap *lod = heightmap; lod; lod = lod->lowDetail)
			lod->GenerateNormals ();

		if (cb) cb->PrintMsg ("initializing texturing system...");

		// load textures
		texturing = new TerrainTexture;
		texturing->Load (&tdf, heightmap, quadtree, qmaps, &config, cb, li);

		renderDataManager = new RenderDataManager (lowdetailhm, qmaps.front());

		// calculate index table
		indexTable=new IndexTable;
		d_trace ("Index buffer data size: %d\n", VertexBuffer::TotalSize ());
	}

	void FindRAWProps(int len, int& width, int& bytespp, ILoadCallback* cb)
	{
		// test for 16-bit
		int w = 3;
		while (w*w*2 < len)
			w = (w-1)*2+1;

		if (w*w*2 != len) {
			w = 3; // test for 8-bit
			while (w*w < len)
				w = (w-1)*2+1;

			if (w*w != len) {
				cb->PrintMsg ("Incorrect raw file size: %d, last comparing size: %d", len, w*w*2);
				return;
			} else bytespp=1;
		} else bytespp=2;

		width = w;
	}

	Heightmap* Terrain::LoadHeightmapFromRAW (const std::string& file, ILoadCallback *cb)
	{
		CFileHandler fh (file);
		if (!fh.FileExists ()) {
			cb->PrintMsg ("Failed to load %s", file.c_str());
			return 0;
		}

		int len = fh.FileSize();

		int w=-1, bits;
		FindRAWProps(len, w, bits, cb);
		bits*=8;
		if (w<0) return 0;

		Heightmap *hm = new Heightmap;
		hm->Alloc (w,w);

		if (bits==16) {
			ushort *tmp=new ushort[w*w];
			fh.Read (tmp, len);
			for (int y=0;y<w*w;y++) hm->data[y]=float(tmp[y]) / float((1<<16)-1);
#ifdef SWAP_SHORT
			char *p = (char*)hm->data;
			for (int x=0;x<len;x+=2, p+=2)
				std::swap (p[0],p[1]);
#endif
		} else {
			uchar *buf=new uchar[len], *p=buf;
			fh.Read(buf,len);
			for (w=w*w-1;w>=0;w--)
				hm->data[w]=*(p++)/255.0f;
			delete[] buf;
		}

		return hm;
	}

	Heightmap* Terrain::LoadHeightmapFromImage(const std::string& heightmapFile, ILoadCallback *cb)
	{
		const char *hmfile = heightmapFile.c_str();
		CFileHandler fh(heightmapFile);
		if (!fh.FileExists())
			throw content_error(heightmapFile + " does not exist");

		ILuint ilheightmap;
		ilGenImages (1, &ilheightmap);
		ilBindImage (ilheightmap);

		int len=fh.FileSize();
		assert(len >= 0);

		char *buffer=new char[len];
		fh.Read(buffer, len);
        bool success = !!ilLoadL(IL_TYPE_UNKNOWN, buffer, len);
		delete [] buffer;

		if (!success) {
			ilDeleteImages (1,&ilheightmap);
			cb->PrintMsg ("Failed to load %s", hmfile);
			return false;
		}

		int hmWidth = ilGetInteger (IL_IMAGE_WIDTH);
		int hmHeight = ilGetInteger (IL_IMAGE_HEIGHT);

		// does it have the correct size? 129,257,513
		int testw = 1;
		while (testw < hmWidth) {
			if (testw + 1 == hmWidth)
				break;
			testw <<= 1;
		}
		if (testw>hmWidth || hmWidth!=hmHeight) {
			cb->PrintMsg ("Heightmap %s has wrong dimensions (should be 129x129,257x257...)", hmfile);
			ilDeleteImages (1,&ilheightmap);
			return false;
		}

		// convert
		if (!ilConvertImage (IL_LUMINANCE, IL_UNSIGNED_SHORT)) {
			cb->PrintMsg ("Failed to convert heightmap image (%s) to grayscale image.", hmfile);
			ilDeleteImages (1,&ilheightmap);
			return false;
		}

		// copy the data into the highest detail heightmap
		Heightmap *hm = new Heightmap;
		hm->Alloc (hmWidth,hmHeight);
		ushort* data=(ushort*)ilGetData ();

		for (int y=0;y<hmWidth*hmHeight;y++)
			hm->data[y]=data[y]/float((1<<16)-1);

		// heightmap is copied, so the original image can be deleted
		ilDeleteImages (1,&ilheightmap);
		return hm;
	}

	Vector3 Terrain::TerrainSize ()
	{
		return Vector3 (heightmap->w, 0.0f, heightmap->h) * SquareSize;
	}

	int Terrain::GetHeightmapWidth ()
	{
		return heightmap->w;
	}

	void Terrain::SetShaderParams (Vector3 dir, Vector3 eyevec)
	{
		texturing->SetShaderParams (dir, eyevec);
	}

	// heightmap blitting
	void Terrain::HeightmapUpdated (int sx,int sy,int w,int h)
	{
		// clip
		if (sx < 0) sx = 0;
		if (sy < 0) sy = 0;
		if (sx + w > heightmap->w) w = heightmap->w - sx;
		if (sy + h > heightmap->h) h = heightmap->h - sy;

		// mark for vertex buffer update
		renderDataManager->UpdateRect(sx,sy,w,h);

		// update heightmap mipmap chain
		heightmap->UpdateLower(sx,sy,w,h);
	}

	void Terrain::GetHeightmap (int sx,int sy,int w,int h, float *dest)
	{
		// clip
		if (sx < 0) sx = 0;
		if (sy < 0) sy = 0;
		if (sx + w > heightmap->w) w = heightmap->w - sx;
		if (sy + h > heightmap->h) h = heightmap->h - sy;

		for (int y=sy;y<sy+h;y++)
			for (int x=sx;x<sx+w;x++)
				*(dest++) = heightmap->HeightAt (x,y);
	}

	float* Terrain::GetHeightmap()
	{
		return heightmap->data;
	}

	//TODO: Interpolate
	float Terrain::GetHeight (float x, float z)
	{
		const float s = 1.0f / SquareSize;
		int px = (int)(x*s);
		if (px < 0) px=0;
		if (px >= heightmap->w) px=heightmap->w-1;
		int pz = (int)(z*s);
		if (pz < 0) pz=0;
		if (pz >= heightmap->h) pz=heightmap->h-1;
		return heightmap->HeightAt (px,pz);
	}


	void Terrain::SetShadowParams(ShadowMapParams *smp)
	{
		texturing->SetShadowMapParams(smp);
	}

	RenderContext* Terrain::AddRenderContext (Camera* cam, bool needsTexturing)
	{
		RenderContext *rc = new RenderContext;

		rc->cam = cam;
		rc->needsTexturing = needsTexturing;

		contexts.push_back (rc);
		return rc;
	}

	void Terrain::RemoveRenderContext (RenderContext *rc)
	{
		contexts.erase (find(contexts.begin(),contexts.end(),rc));
		delete rc;
	}

	void Terrain::SetActiveContext (RenderContext *rc)
	{
		activeRC = rc;
	}

};

