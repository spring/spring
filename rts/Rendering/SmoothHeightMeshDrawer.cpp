/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SmoothHeightMeshDrawer.h"

#include "Game/UI/MiniMap.h"
#include "Map/ReadMap.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "System/EventHandler.h"
#include "System/float3.h"
#include "System/SafeUtil.h"

using namespace SmoothHeightMeshNamespace;

static SmoothHeightMeshDrawer* smoothMeshDrawer = nullptr;

SmoothHeightMeshDrawer* SmoothHeightMeshDrawer::GetInstance() {
	if (smoothMeshDrawer == nullptr) {
		smoothMeshDrawer = new SmoothHeightMeshDrawer();
	}
	return smoothMeshDrawer;
}

void SmoothHeightMeshDrawer::FreeInstance() {
	if (smoothMeshDrawer != nullptr) {
		spring::SafeDelete(smoothMeshDrawer);
	}
}

SmoothHeightMeshDrawer::SmoothHeightMeshDrawer()
	: CEventClient("[SmoothHeightMeshDrawer]", 300002, false)
	, drawEnabled(false)
{
	eventHandler.AddClient(this);
}
SmoothHeightMeshDrawer::~SmoothHeightMeshDrawer() {
	eventHandler.RemoveClient(this);
}

void SmoothHeightMeshDrawer::DrawInMiniMap()
{
	if (!(drawEnabled && gs->cheatEnabled))
		return;

	glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0.0f, 1.0f, 0.0f, 1.0f, 0.0, -1.0);
		minimap->ApplyConstraintsMatrix();
	glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glTranslatef3(UpVector);
		glScalef(1.0f / mapDims.mapx, -1.0f / mapDims.mapy, 1.0f);

	glDisable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 0.0f, 0.7f);

	const SmoothHeightMesh::MapChangeTrack& mapChangeTrack = smoothGround.mapChangeTrack;
	const float tileSize = SAMPLES_PER_QUAD * smoothGround.resolution;
	int i = 0;
	for (auto changed : mapChangeTrack.damageMap) {
		if (changed){
			const float x = (i % mapChangeTrack.width) * tileSize;
			const float y = (i / mapChangeTrack.width) * tileSize;
			glRectf(x, y, x + tileSize, y + tileSize);
		}
		i++;
	}

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);

	glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
}

void SmoothHeightMeshDrawer::Draw(float yoffset) {
	if (!(drawEnabled && gs->cheatEnabled))
		return;

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(1.0f);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_CULL_FACE);

	const float quadSize = 4.0f * smoothGround.GetResolution();
	const unsigned int numQuadsX = smoothGround.GetFMaxX() / quadSize;
	const unsigned int numQuadsZ = smoothGround.GetFMaxY() / quadSize;

	CVertexArray* va = GetVertexArray();
	va->Initialize();
	va->EnlargeArrays(numQuadsX * numQuadsZ * 4, 0, VA_SIZE_0);

	for (unsigned int zq = 0; zq < numQuadsZ; zq++) {
		for (unsigned int xq = 0; xq < numQuadsX; xq++) {
			const float x = xq * quadSize;
			const float z = zq * quadSize;

			const float h1 = smoothGround.GetHeight(x,            z           ) + yoffset;
			const float h2 = smoothGround.GetHeight(x + quadSize, z           ) + yoffset;
			const float h3 = smoothGround.GetHeight(x + quadSize, z + quadSize) + yoffset;
			const float h4 = smoothGround.GetHeight(x,            z + quadSize) + yoffset;

			va->AddVertexQ0(float3(x,            h1, z           ));
			va->AddVertexQ0(float3(x + quadSize, h2, z           ));
			va->AddVertexQ0(float3(x + quadSize, h3, z + quadSize));
			va->AddVertexQ0(float3(x,            h4, z + quadSize));
		}
	}

	glColor4ub(0, 255, 0, 255); 
	va->DrawArray0(GL_QUADS);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
