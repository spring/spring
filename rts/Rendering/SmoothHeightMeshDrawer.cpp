/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SmoothHeightMeshDrawer.h"
#include "Game/UnsyncedGameCommands.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "Rendering/GL/VertexArray.h"
#include "System/float3.h"


namespace { // prevents linking problems in case of duplicate symbols

class AirMeshActionExecutor : public IUnsyncedActionExecutor {
public:
	static const std::string COMMAND_NAME;

	AirMeshActionExecutor() : IUnsyncedActionExecutor(COMMAND_NAME,
			"Show/Hide the smooth air-mesh map overlay") {}

	bool Execute(const UnsyncedAction& action) const {
		SetBoolArg(smoothHeightMeshDrawer->DrawEnabled(), action.GetArgs());
		LogSystemStatus("smooth air-mesh map overlay", smoothHeightMeshDrawer->DrawEnabled());
		return true;
	}
};

const std::string AirMeshActionExecutor::COMMAND_NAME = "AirMesh";

} // namespace (unnamed)


SmoothHeightMeshDrawer::SmoothHeightMeshDrawer()
		: drawEnabled(false)
{
	unsyncedGameCommands->AddActionExecutor(new AirMeshActionExecutor());
}

SmoothHeightMeshDrawer::~SmoothHeightMeshDrawer() {

	if (unsyncedGameCommands != NULL) {
		unsyncedGameCommands->RemoveActionExecutor(AirMeshActionExecutor::COMMAND_NAME);
	}
}

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

	const float quadSize = 4.0f * smoothGround->GetResolution();
	const unsigned int numQuadsX = smoothGround->GetFMaxX() / quadSize;
	const unsigned int numQuadsZ = smoothGround->GetFMaxY() / quadSize;

	CVertexArray* va = GetVertexArray();
	va->Initialize();
	va->EnlargeArrays((numQuadsX + 1) * (numQuadsZ + 1) * 4, 0, VA_SIZE_0);

	for (unsigned int zq = 0; zq <= numQuadsZ; zq++) {
		for (unsigned int xq = 0; xq <= numQuadsX; xq++) {
			const float x = xq * quadSize;
			const float z = zq * quadSize;
			const float h1 = smoothGround->GetHeightAboveWater(x,            z           ) + yoffset;
			const float h2 = smoothGround->GetHeightAboveWater(x + quadSize, z           ) + yoffset;
			const float h3 = smoothGround->GetHeightAboveWater(x + quadSize, z + quadSize) + yoffset;
			const float h4 = smoothGround->GetHeightAboveWater(x,            z + quadSize) + yoffset;

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
