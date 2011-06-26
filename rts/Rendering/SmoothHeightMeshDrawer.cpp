/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SmoothHeightMeshDrawer.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "Rendering/GL/VertexArray.h"
#include "System/float3.h"

SmoothHeightMeshDrawer* SmoothHeightMeshDrawer::GetInstance() {
	static SmoothHeightMeshDrawer drawer;
	return &drawer;
}

void SmoothHeightMeshDrawer::Draw(float yoffset) {
	if (!drawEnabled)
		return;

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(1.0f);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_CULL_FACE);

	const unsigned char color[4] = {0, 255, 0, 255};
	const float quadSize = 4.0f * smoothGround->GetResolution();
	const unsigned int numQuadsX = smoothGround->GetFMaxX() / quadSize;
	const unsigned int numQuadsZ = smoothGround->GetFMaxY() / quadSize;

	CVertexArray* va = GetVertexArray();
	va->Initialize();
	va->EnlargeArrays(numQuadsX * numQuadsZ * 4, 0, VA_SIZE_C);

	for (float z = 0; z < smoothGround->GetFMaxY(); z += quadSize) {
		for (float x = 0; x < smoothGround->GetFMaxX(); x += quadSize) {
			const float h1 = smoothGround->GetHeightAboveWater(x,            z           ) + yoffset;
			const float h2 = smoothGround->GetHeightAboveWater(x + quadSize, z           ) + yoffset;
			const float h3 = smoothGround->GetHeightAboveWater(x + quadSize, z + quadSize) + yoffset;
			const float h4 = smoothGround->GetHeightAboveWater(x,            z + quadSize) + yoffset;

			va->AddVertexQC(float3(x,            h1, z           ), color);
			va->AddVertexQC(float3(x + quadSize, h2, z           ), color);
			va->AddVertexQC(float3(x + quadSize, h3, z + quadSize), color);
			va->AddVertexQC(float3(x,            h4, z + quadSize), color);
		}
	}

	va->DrawArrayC(GL_QUADS);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
