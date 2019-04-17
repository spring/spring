/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SmoothHeightMeshDrawer.h"
#include "Game/Camera.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "Rendering/GL/RenderDataBuffer.hpp"

SmoothHeightMeshDrawer* SmoothHeightMeshDrawer::GetInstance() {
	static SmoothHeightMeshDrawer drawer;
	return &drawer;
}

void SmoothHeightMeshDrawer::Draw(float yoffset) {
	if (!drawEnabled)
		return;

	glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	const float quadSize = 4.0f * smoothGround.GetResolution();
	const unsigned int numQuadsX = smoothGround.GetFMaxX() / quadSize;
	const unsigned int numQuadsZ = smoothGround.GetFMaxY() / quadSize;
	const SColor quadColor = {0.0f, 1.0f, 0.0f, 1.0f};

	GL::RenderDataBufferC* buffer = GL::GetRenderBufferC();
	Shader::IProgramObject* shader = buffer->GetShader();

	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, camera->GetViewMatrix());
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, camera->GetProjectionMatrix());

	for (unsigned int zq = 0; zq <= numQuadsZ; zq++) {
		for (unsigned int xq = 0; xq <= numQuadsX; xq++) {
			const float x = xq * quadSize;
			const float z = zq * quadSize;

			const float h1 = smoothGround.GetHeightAboveWater(x,            z           ) + yoffset;
			const float h2 = smoothGround.GetHeightAboveWater(x + quadSize, z           ) + yoffset;
			const float h3 = smoothGround.GetHeightAboveWater(x + quadSize, z + quadSize) + yoffset;
			const float h4 = smoothGround.GetHeightAboveWater(x,            z + quadSize) + yoffset;

			buffer->SafeAppend({{x,            h1, z           }, quadColor}); // tl
			buffer->SafeAppend({{x + quadSize, h2, z           }, quadColor}); // tr
			buffer->SafeAppend({{x + quadSize, h3, z + quadSize}, quadColor}); // br

			buffer->SafeAppend({{x + quadSize, h3, z + quadSize}, quadColor}); // br
			buffer->SafeAppend({{x,            h4, z + quadSize}, quadColor}); // bl
			buffer->SafeAppend({{x,            h1, z           }, quadColor}); // tl
		}
	}

	buffer->Submit(GL_TRIANGLES);
	shader->Disable();

	glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

