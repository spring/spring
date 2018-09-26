/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BasicMeshDrawer.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Map/SMF/SMFGroundDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "System/EventHandler.h"

#define USE_TRIANGLE_STRIPS 1
#define USE_MIPMAP_BUFFERS  0

#if (defined(GLEW_ARB_buffer_storage) && defined(GL_MAP_PERSISTENT_BIT))
#define USE_MAPPED_BUFFERS 1
#else
#define USE_MAPPED_BUFFERS 0
#endif


// do not use GL_MAP_UNSYNCHRONIZED_BIT in either case; causes slow driver sync
#if (USE_MAPPED_BUFFERS == 1)
#define BUFFER_MAP_BITS (GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT)
#else
#define BUFFER_MAP_BITS (GL_MAP_WRITE_BIT)
#endif



typedef CBasicMeshDrawer::MeshPatch MeshPatch;

class MeshPatchVisTestDrawer: public CReadMap::IQuadDrawer {
public:
	void ResetState() override {}
	void ResetState(CCamera* c, MeshPatch* p, uint32_t xsize) {
		testCamera = c;
		patchArray = p;
		numPatches = xsize;
	}

	void DrawQuad(int px, int py) override {
		patchArray[py * numPatches + px].visUpdateFrames[testCamera->GetCamType()] = globalRendering->drawFrame;
	}

private:
	CCamera* testCamera;
	MeshPatch* patchArray;

	uint32_t numPatches;
};

static MeshPatchVisTestDrawer patchVisTestDrawer;



CBasicMeshDrawer::CBasicMeshDrawer(CSMFGroundDrawer* gd)
	: CEventClient("[CBasicMeshDrawer]", 717171, false)
	, smfGroundDrawer(gd)
{
	eventHandler.AddClient(this);

	numPatchesX = mapDims.mapx / PATCH_SIZE;
	numPatchesY = mapDims.mapy / PATCH_SIZE;
	drawPassLOD = 0;

	assert(numPatchesX >= 1);
	assert(numPatchesY >= 1);

	// const auto& lodDistFunc = [](uint32_t i) { return (125.0f * i * i        ); }; // f(x)=125.0*x^2
	// const auto& lodDistFunc = [](uint32_t i) { return ( 25.0f * i * i * i    ); }; // f(x)=25.0*x^3
	const auto& lodDistFunc = [](uint32_t i) { return (  5.0f * i * i * i * i); }; // f(x)=5.0*x^4

	for (uint32_t n = 0; n < LOD_LEVELS; n++) {
		lodDistTable[n] = lodDistFunc(n + 1);
	}

	meshPatches.resize(numPatchesX * numPatchesY);

	for (uint32_t y = 0; y < numPatchesY; y += 1) {
		for (uint32_t x = 0; x < numPatchesX; x += 1) {
			MeshPatch& meshPatch = meshPatches[y * numPatchesX + x];

			meshPatch.squareVertexPtrs.fill(nullptr);

			meshPatch.visUpdateFrames.fill(0);
			meshPatch.uhmUpdateFrames.fill(0);
		}
	}

	for (uint32_t n = 0; n < LOD_LEVELS; n += 1) {
		UploadPatchIndices(n);
	}

	UnsyncedHeightMapUpdate(SRectangle{0, 0, mapDims.mapx, mapDims.mapy});
}

CBasicMeshDrawer::~CBasicMeshDrawer() {
	eventHandler.RemoveClient(this);

	#if (USE_MAPPED_BUFFERS == 1)
	for (uint32_t y = 0; y < numPatchesY; y += 1) {
		for (uint32_t x = 0; x < numPatchesX; x += 1) {
			MeshPatch& meshPatch = meshPatches[y * numPatchesX + x];

			for (uint32_t n = 0; n < LOD_LEVELS; n += 1) {
				meshPatch.squareVertexBuffers[n].vbo.Release();
				meshPatch.squareVertexBuffers[n].vao.Delete();
			}
			for (uint32_t i = MAP_BORDER_L; i <= MAP_BORDER_B; i++) {
				for (uint32_t n = 0; n < LOD_LEVELS; n += 1) {
					meshPatch.borderVertexBuffers[i][n].vbo.Release();
					meshPatch.borderVertexBuffers[i][n].vao.Delete();
				}
			}

			meshPatch.squareVertexPtrs.fill(nullptr);
		}
	}
	#endif

	for (uint32_t n = 0; n < LOD_LEVELS; n += 1) {
		lodSquareIndexBuffers[n].Release();
		lodBorderIndexBuffers[n].Release();
	}
}



void CBasicMeshDrawer::Update(const DrawPass::e& drawPass) {
	CCamera* activeCam = CCameraHandler::GetActiveCamera();
	MeshPatch* meshPatch = &meshPatches[0];

	patchVisTestDrawer.ResetState(activeCam, meshPatch, numPatchesX);

	activeCam->CalcFrustumLines(readMap->GetCurrMinHeight() - 100.0f, readMap->GetCurrMaxHeight() + 100.0f, SQUARE_SIZE);
	readMap->GridVisibility(activeCam, &patchVisTestDrawer, 1e9, PATCH_SIZE);

	drawPassLOD = CalcDrawPassLOD(activeCam, drawPass);
}

void CBasicMeshDrawer::UnsyncedHeightMapUpdate(const SRectangle& rect) {
	const uint32_t minPatchX = std::max(rect.x1 / PATCH_SIZE,    (              0));
	const uint32_t minPatchY = std::max(rect.z1 / PATCH_SIZE,    (              0));
	const uint32_t maxPatchX = std::min(rect.x2 / PATCH_SIZE, int(numPatchesX - 1));
	const uint32_t maxPatchY = std::min(rect.z2 / PATCH_SIZE, int(numPatchesY - 1));
	const uint32_t lodLevels = std::max(1, LOD_LEVELS * USE_MIPMAP_BUFFERS);

	const float* heightMap = readMap->GetCornerHeightMapUnsynced();
	const float3* normalMap = readMap->GetVisVertexNormalsUnsynced();

	// TODO: update asynchronously, clip rect against patch bounds
	for (uint32_t py = minPatchY; py <= maxPatchY; py += 1) {
		for (uint32_t px = minPatchX; px <= maxPatchX; px += 1) {
			for (uint32_t n = 0; n < lodLevels; n += 1) {
				UploadPatchSquareGeometry(n, px, py, heightMap, normalMap);
			}
			// need border data at all MIP's regardless of USE_MIPMAP_BUFFERS
			for (uint32_t n = 0; n < LOD_LEVELS; n += 1) {
				UploadPatchBorderGeometry(n, px, py, heightMap, normalMap);
			}
		}
	}
}



void CBasicMeshDrawer::UploadPatchSquareGeometry(uint32_t n, uint32_t px, uint32_t py, const float* chm, const float3* cnm) {
	MeshPatch& meshPatch = meshPatches[py * numPatchesX + px];

	VAO& squareVAO = meshPatch.squareVertexBuffers[n].vao;
	VBO& squareVBO = meshPatch.squareVertexBuffers[n].vbo;

	meshPatch.uhmUpdateFrames[0] = globalRendering->drawFrame;

	const uint32_t lodStep  = 1 << n;
	const uint32_t lodVerts = (PATCH_SIZE / lodStep) + 1;

	const uint32_t bpx = px * PATCH_SIZE;
	const uint32_t bpy = py * PATCH_SIZE;

	uint32_t vertexIndx = 0;

	{
		#if (USE_MAPPED_BUFFERS == 1)
		// HACK: the VBO constructor defaults to storage=false
		squareVBO.immutableStorage = true;
		#endif

		if (squareVBO.vboId == 0)
			squareVAO.Generate();

		squareVAO.Bind();
		squareVBO.Bind(GL_ARRAY_BUFFER);
		squareVBO.New((lodVerts * lodVerts) * sizeof(float3), GL_DYNAMIC_DRAW);


		float3* verts = meshPatch.squareVertexPtrs[n];

		if (verts == nullptr)
			verts = reinterpret_cast<float3*>(squareVBO.MapBuffer(BUFFER_MAP_BITS));

		#if (USE_MAPPED_BUFFERS == 1)
		meshPatch.squareVertexPtrs[n] = verts;
		#endif

		if (verts != nullptr) {
			// TODO: geometry shader?
			for (uint32_t vy = 0; vy < lodVerts; vy += 1) {
				for (uint32_t vx = 0; vx < lodVerts; vx += 1) {
					const uint32_t lvx = vx * lodStep;
					const uint32_t lvy = vy * lodStep;

					verts[vertexIndx  ].x = (bpx + lvx) * SQUARE_SIZE;
					verts[vertexIndx  ].z = (bpy + lvy) * SQUARE_SIZE;
					verts[vertexIndx++].y = chm[(bpy + lvy) * mapDims.mapxp1 + (bpx + lvx)];
				}
			}
		}

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(float3), VA_TYPE_OFFSET(float, 0));

		#if (USE_MAPPED_BUFFERS == 0)
		squareVBO.UnmapBuffer();
		#endif
		squareVAO.Unbind();
		squareVBO.Unbind();

		glDisableVertexAttribArray(0);
	}
}

void CBasicMeshDrawer::UploadPatchBorderGeometry(uint32_t n, uint32_t px, uint32_t py, const float* chm, const float3* cnm) {
	MeshPatch& meshPatch = meshPatches[py * numPatchesX + px];

	const uint32_t lodStep  = 1 << n;
	const uint32_t lodVerts = (PATCH_SIZE / lodStep) + 1;

	const uint32_t bpx = px * PATCH_SIZE;
	const uint32_t bpy = py * PATCH_SIZE;

	uint32_t vertexIndx = 0;

	if (px == 0) {
		vertexIndx = 0;

		VAO& borderVAO = meshPatch.borderVertexBuffers[MAP_BORDER_L][n].vao;
		VBO& borderVBO = meshPatch.borderVertexBuffers[MAP_BORDER_L][n].vbo;

		{
			if (borderVBO.vboId == 0)
				borderVAO.Generate();

			borderVAO.Bind();
			borderVBO.Bind(GL_ARRAY_BUFFER);
			borderVBO.New(lodVerts * sizeof(VA_TYPE_C) * 2, GL_DYNAMIC_DRAW);

			VA_TYPE_C* verts = reinterpret_cast<VA_TYPE_C*>(borderVBO.MapBuffer());

			if (verts != nullptr) {
				for (uint32_t vy = 0; vy < lodVerts; vy += 1) {
					verts[vertexIndx           ].p.x = 0.0f;
					verts[vertexIndx + lodVerts].p.x = 0.0f;
					verts[vertexIndx           ].p.z =     (bpy + vy * lodStep) * SQUARE_SIZE;
					verts[vertexIndx + lodVerts].p.z =     (bpy + vy * lodStep) * SQUARE_SIZE;
					verts[vertexIndx           ].p.y = chm[(bpy + vy * lodStep) * mapDims.mapxp1 + 0]; // upper
					verts[vertexIndx + lodVerts].p.y = std::min(readMap->GetInitMinHeight(), -500.0f); // lower

					// terrain normals point up, borders do not
					// currently not needed for border rendering
					// verts[vertexIndx           ].n = -RgtVector;
					// verts[vertexIndx + lodVerts].n = -RgtVector;

					verts[vertexIndx           ].c = SColor(255, 255, 255, 255);
					verts[vertexIndx + lodVerts].c = SColor(255, 255, 255,   0);

					vertexIndx += 1;
				}
			}

			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VA_TYPE_C), VA_TYPE_OFFSET(float, 0));
			glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, false, sizeof(VA_TYPE_C), VA_TYPE_OFFSET(float, 3));

			borderVBO.UnmapBuffer();
			borderVAO.Unbind();
			borderVBO.Unbind();

			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(0);
		}
	}
	if (px == (numPatchesX - 1)) {
		vertexIndx = 0;

		VAO& borderVAO = meshPatch.borderVertexBuffers[MAP_BORDER_R][n].vao;
		VBO& borderVBO = meshPatch.borderVertexBuffers[MAP_BORDER_R][n].vbo;

		{
			if (borderVBO.vboId == 0)
				borderVAO.Generate();

			borderVAO.Bind();
			borderVBO.Bind(GL_ARRAY_BUFFER);
			borderVBO.New(lodVerts * sizeof(VA_TYPE_C) * 2, GL_DYNAMIC_DRAW);

			VA_TYPE_C* verts = reinterpret_cast<VA_TYPE_C*>(borderVBO.MapBuffer());

			if (verts != nullptr) {
				for (uint32_t vy = 0; vy < lodVerts; vy += 1) {
					verts[vertexIndx           ].p.x = mapDims.mapx * SQUARE_SIZE;
					verts[vertexIndx + lodVerts].p.x = mapDims.mapx * SQUARE_SIZE;
					verts[vertexIndx           ].p.z =     (bpy + vy * lodStep) * SQUARE_SIZE;
					verts[vertexIndx + lodVerts].p.z =     (bpy + vy * lodStep) * SQUARE_SIZE;
					verts[vertexIndx           ].p.y = chm[(bpy + vy * lodStep) * mapDims.mapxp1 + mapDims.mapx];
					verts[vertexIndx + lodVerts].p.y = std::min(readMap->GetInitMinHeight(), -500.0f);

					// verts[vertexIndx           ].n = RgtVector;
					// verts[vertexIndx + lodVerts].n = RgtVector;

					verts[vertexIndx           ].c = SColor(255, 255, 255, 255);
					verts[vertexIndx + lodVerts].c = SColor(255, 255, 255,   0);

					vertexIndx += 1;
				}
			}

			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VA_TYPE_C), VA_TYPE_OFFSET(float, 0));
			glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, false, sizeof(VA_TYPE_C), VA_TYPE_OFFSET(float, 3));

			borderVBO.UnmapBuffer();
			borderVAO.Unbind();
			borderVBO.Unbind();

			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(0);
		}
	}

	if (py == 0) {
		vertexIndx = 0;

		VAO& borderVAO = meshPatch.borderVertexBuffers[MAP_BORDER_T][n].vao;
		VBO& borderVBO = meshPatch.borderVertexBuffers[MAP_BORDER_T][n].vbo;

		{
			if (borderVBO.vboId == 0)
				borderVAO.Generate();

			borderVAO.Bind();
			borderVBO.Bind(GL_ARRAY_BUFFER);
			borderVBO.New(lodVerts * sizeof(VA_TYPE_C) * 2, GL_DYNAMIC_DRAW);

			VA_TYPE_C* verts = reinterpret_cast<VA_TYPE_C*>(borderVBO.MapBuffer());

			if (verts != nullptr) {
				for (uint32_t vx = 0; vx < lodVerts; vx += 1) {
					verts[vertexIndx           ].p.x = (bpx + vx * lodStep) * SQUARE_SIZE;
					verts[vertexIndx + lodVerts].p.x = (bpx + vx * lodStep) * SQUARE_SIZE;
					verts[vertexIndx           ].p.z = 0.0f;
					verts[vertexIndx + lodVerts].p.z = 0.0f;
					verts[vertexIndx           ].p.y = chm[0 + (bpx + vx * lodStep)];
					verts[vertexIndx + lodVerts].p.y = std::min(readMap->GetInitMinHeight(), -500.0f);

					// verts[vertexIndx           ].n = -FwdVector;
					// verts[vertexIndx + lodVerts].n = -FwdVector;

					verts[vertexIndx           ].c = SColor(255, 255, 255, 255);
					verts[vertexIndx + lodVerts].c = SColor(255, 255, 255,   0);

					vertexIndx += 1;
				}
			}

			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VA_TYPE_C), VA_TYPE_OFFSET(float, 0));
			glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, false, sizeof(VA_TYPE_C), VA_TYPE_OFFSET(float, 3));

			borderVBO.UnmapBuffer();
			borderVAO.Unbind();
			borderVBO.Unbind();

			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(0);
		}
	}
	if (py == (numPatchesY - 1)) {
		vertexIndx = 0;

		VAO& borderVAO = meshPatch.borderVertexBuffers[MAP_BORDER_B][n].vao;
		VBO& borderVBO = meshPatch.borderVertexBuffers[MAP_BORDER_B][n].vbo;

		{
			if (borderVBO.vboId == 0)
				borderVAO.Generate();

			borderVAO.Bind();
			borderVBO.Bind(GL_ARRAY_BUFFER);
			borderVBO.New(lodVerts * sizeof(VA_TYPE_C) * 2, GL_DYNAMIC_DRAW);

			VA_TYPE_C* verts = reinterpret_cast<VA_TYPE_C*>(borderVBO.MapBuffer());

			if (verts != nullptr) {
				for (uint32_t vx = 0; vx < lodVerts; vx += 1) {
					verts[vertexIndx           ].p.x = (bpx + vx * lodStep) * SQUARE_SIZE;
					verts[vertexIndx + lodVerts].p.x = (bpx + vx * lodStep) * SQUARE_SIZE;
					verts[vertexIndx           ].p.z = mapDims.mapy * SQUARE_SIZE;
					verts[vertexIndx + lodVerts].p.z = mapDims.mapy * SQUARE_SIZE;
					verts[vertexIndx           ].p.y = chm[mapDims.mapy * mapDims.mapxp1 + (bpx + vx * lodStep)];
					verts[vertexIndx + lodVerts].p.y = std::min(readMap->GetInitMinHeight(), -500.0f);

					// verts[vertexIndx           ].n = FwdVector;
					// verts[vertexIndx + lodVerts].n = FwdVector;

					verts[vertexIndx           ].c = SColor(255, 255, 255, 255);
					verts[vertexIndx + lodVerts].c = SColor(255, 255, 255,   0);

					vertexIndx += 1;
				}
			}

			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VA_TYPE_C), VA_TYPE_OFFSET(float, 0));
			glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, false, sizeof(VA_TYPE_C), VA_TYPE_OFFSET(float, 3));

			borderVBO.UnmapBuffer();
			borderVAO.Unbind();
			borderVBO.Unbind();

			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(0);
		}
	}
}


void CBasicMeshDrawer::UploadPatchIndices(uint32_t n) {
	// base-level constants
	constexpr uint32_t numVerts = PATCH_SIZE + 1;
	#if (USE_TRIANGLE_STRIPS == 0)
	constexpr uint32_t numQuads = PATCH_SIZE;
	constexpr uint32_t numPolys = (numQuads * numQuads) * 2;
	#endif

	const uint32_t lodStep  = 1 << n;
	const uint32_t lodQuads = (PATCH_SIZE / lodStep);
	const uint32_t lodVerts = (PATCH_SIZE / lodStep) + 1;

	VBO& squareIBO = lodSquareIndexBuffers[n];
	VBO& borderIBO = lodBorderIndexBuffers[n];

	{
		// NOTE:
		//   with GL_STATIC_DRAW, driver spams [OPENGL_DEBUG] id=131186 source=API type=PERFORMANCE severity=MEDIUM
		//   msg="Buffer performance warning: Buffer object 18 (bound to GL_ELEMENT_ARRAY_BUFFER_ARB, usage hint is
		//   GL_STATIC_DRAW) is being copied/moved from VIDEO memory to HOST memory." unless USE_MAPPED_BUFFERS=0
		squareIBO.Bind(GL_ELEMENT_ARRAY_BUFFER);
		#if (USE_TRIANGLE_STRIPS == 0)
		squareIBO.New((numPolys / (lodStep * lodStep)) * 3 * sizeof(uint16_t), GL_DYNAMIC_DRAW);
		#else
		squareIBO.New(((lodQuads * 2 + 3) * lodQuads) * sizeof(uint16_t), GL_DYNAMIC_DRAW);
		#endif

		uint16_t* indcs = reinterpret_cast<uint16_t*>(squareIBO.MapBuffer());

		if (indcs != nullptr) {
			uint32_t indxCtr = 0;
			// uint32_t quadCtr = 0;

			// A B    B   or   A ... [B]
			// C    C D        C ... [D]
			for (uint32_t vy = 0; vy < lodQuads; vy += 1) {
				#if (USE_TRIANGLE_STRIPS == 0)
					for (uint32_t vx = 0; vx < lodQuads; vx += 1) {
						#if (USE_MIPMAP_BUFFERS == 1)
							indcs[indxCtr++] = ((vy * lodVerts) + (vx +     lodVerts)); // C
							indcs[indxCtr++] = ((vy * lodVerts) + (vx + 1           )); // B
							indcs[indxCtr++] = ((vy * lodVerts) + (vx               )); // A

							indcs[indxCtr++] = ((vy * lodVerts) + (vx     + lodVerts)); // C
							indcs[indxCtr++] = ((vy * lodVerts) + (vx + 1 + lodVerts)); // D
							indcs[indxCtr++] = ((vy * lodVerts) + (vx + 1           )); // B
						#else
							indcs[indxCtr++] = ((vy * numVerts) + (vx +     numVerts)) * lodStep; // C
							indcs[indxCtr++] = ((vy * numVerts) + (vx + 1           )) * lodStep; // B
							indcs[indxCtr++] = ((vy * numVerts) + (vx               )) * lodStep; // A

							indcs[indxCtr++] = ((vy * numVerts) + (vx     + numVerts)) * lodStep; // C
							indcs[indxCtr++] = ((vy * numVerts) + (vx + 1 + numVerts)) * lodStep; // D
							indcs[indxCtr++] = ((vy * numVerts) + (vx + 1           )) * lodStep; // B
						#endif
					}

				#else

					for (uint32_t vx = 0; vx < lodQuads; vx += 1) {
						#if (USE_MIPMAP_BUFFERS == 1)
							indcs[indxCtr++] = ((vy * lodVerts) + (vx               )); // A
							indcs[indxCtr++] = ((vy * lodVerts) + (vx +     lodVerts)); // C
						#else
							indcs[indxCtr++] = ((vy * numVerts) + (vx               )) * lodStep; // A
							indcs[indxCtr++] = ((vy * numVerts) + (vx +     numVerts)) * lodStep; // C
						#endif
					}

					#if (USE_MIPMAP_BUFFERS == 1)
						indcs[indxCtr++] = ((vy * lodVerts) + (lodQuads           )); // B
						indcs[indxCtr++] = ((vy * lodVerts) + (lodQuads + lodVerts)); // D
					#else
						indcs[indxCtr++] = ((vy * numVerts) + (lodQuads           )) * lodStep; // B
						indcs[indxCtr++] = ((vy * numVerts) + (lodQuads + numVerts)) * lodStep; // D
					#endif

					// terminator
					indcs[indxCtr++] = 0xFFFF;
				#endif
			}

			#if (USE_TRIANGLE_STRIPS == 0)
			assert(indxCtr == ((numPolys / (lodStep * lodStep)) * 3));
			#else
			assert(indxCtr == ((lodQuads * 2 + 3) * lodQuads));
			#endif
		}

		squareIBO.UnmapBuffer();
		squareIBO.Unbind();
	}
	{
		borderIBO.Bind(GL_ELEMENT_ARRAY_BUFFER);
		borderIBO.New(((lodQuads * 2 + 3) * 1) * sizeof(uint16_t), GL_DYNAMIC_DRAW);

		uint16_t* indcs = reinterpret_cast<uint16_t*>(borderIBO.MapBuffer());

		if (indcs != nullptr) {
			uint32_t indxCtr = 0;
			// uint32_t quadCtr = 0;

			for (uint32_t vi = 0; vi < lodQuads; vi += 1) {
				indcs[indxCtr++] = (vi               ); // A
				indcs[indxCtr++] = (vi +     lodVerts); // C
			}

			indcs[indxCtr++] = (lodQuads           ); // B
			indcs[indxCtr++] = (lodQuads + lodVerts); // D
			indcs[indxCtr++] = 0xFFFF;
		}

		borderIBO.UnmapBuffer();
		borderIBO.Unbind();
	}
}



uint32_t CBasicMeshDrawer::CalcDrawPassLOD(const CCamera* cam, const DrawPass::e& drawPass) const {
	// higher detail biases LOD-step toward a smaller value
	// NOTE: should perhaps prevent an insane initial bias?
	int32_t lodBias = smfGroundDrawer->GetGroundDetail(drawPass) % LOD_LEVELS;
	int32_t lodIndx = LOD_LEVELS - 1;

	// force SP and NP to equal LOD; avoids projection issues
	if (drawPass == DrawPass::Shadow || drawPass == DrawPass::WaterReflection)
		cam = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);

	{
		const CUnit* hitUnit = nullptr;
		const CFeature* hitFeature = nullptr;

		float mapRayDist = 0.0f;

		if ((mapRayDist = TraceRay::GuiTraceRay(cam->GetPos(), cam->GetDir(), cam->GetFarPlaneDist(), nullptr, hitUnit, hitFeature, false, true, true)) < 0.0f)
			mapRayDist = CGround::LinePlaneCol(cam->GetPos(), cam->GetDir(), cam->GetFarPlaneDist(), readMap->GetCurrMinHeight());
		if (mapRayDist < 0.0f)
			return lodIndx;

		for (uint32_t n = 0; n < LOD_LEVELS; n += 1) {
			if (mapRayDist < lodDistTable[n]) {
				lodIndx = n;
				break;
			}
		}
	}

	switch (drawPass) {
		case DrawPass::Normal         : { return (std::max(lodIndx - lodBias, 0)); } break;
		case DrawPass::Shadow         : { return (std::max(lodIndx - lodBias, 0)); } break;
		case DrawPass::TerrainDeferred: { return (std::max(lodIndx - lodBias, 0)); } break;
		default: {} break;
	}

	// prevent reflections etc from becoming too low-res
	return (Clamp(lodIndx - lodBias, 0, LOD_LEVELS - 4));
}



void CBasicMeshDrawer::DrawSquareMeshPatch(const MeshPatch& meshPatch, const CCamera* activeCam) const {
	const VAO& squareVAO = meshPatch.squareVertexBuffers[drawPassLOD * USE_MIPMAP_BUFFERS].vao;
	const VBO& squareIBO = lodSquareIndexBuffers[drawPassLOD];

	// supply indices separately after VAO is bound
	squareVAO.Bind();
	squareIBO.Bind(GL_ELEMENT_ARRAY_BUFFER);

	#if (USE_TRIANGLE_STRIPS == 0)
	const uint32_t lodStep  = 1 << drawPassLOD;
	const uint32_t lodQuads = (PATCH_SIZE / lodStep);
	const uint32_t numIndcs = squareIBO.GetSize() / sizeof(uint16_t);

	assert(numIndcs == (lodQuads * lodQuads * 2 * 3));
	assert(squareIBO.GetPtr() == nullptr);
	#endif

	#if (USE_TRIANGLE_STRIPS == 0)
		glDrawElements(GL_TRIANGLES, (squareIBO.GetSize() / sizeof(uint16_t)), GL_UNSIGNED_SHORT, squareIBO.GetPtr());
		// glDrawRangeElements(GL_TRIANGLES, 0, (lodVerts * lodVerts) - 1, (squareIBO.GetSize() / sizeof(uint16_t)), GL_UNSIGNED_SHORT, squareIBO.GetPtr());
	#else
		glPrimitiveRestartIndex(0xFFFF);
		glDrawElements(GL_TRIANGLE_STRIP, (squareIBO.GetSize() / sizeof(uint16_t)), GL_UNSIGNED_SHORT, squareIBO.GetPtr());
	#endif

	squareIBO.Unbind();
	squareVAO.Unbind();
}

void CBasicMeshDrawer::DrawMesh(const DrawPass::e& drawPass) {
	Update(drawPass);

	#if (USE_TRIANGLE_STRIPS == 1)
	glEnable(GL_PRIMITIVE_RESTART);
	#endif

	const CCamera* activeCam = CCameraHandler::GetActiveCamera();

	for (uint32_t py = 0; py < numPatchesY; py += 1) {
		for (uint32_t px = 0; px < numPatchesX; px += 1) {
			const MeshPatch& meshPatch = meshPatches[py * numPatchesX + px];

			if (meshPatch.visUpdateFrames[activeCam->GetCamType()] < globalRendering->drawFrame)
				continue;

			if (drawPass != DrawPass::Shadow)
				smfGroundDrawer->SetupBigSquare(px, py);

			DrawSquareMeshPatch(meshPatch, activeCam);
		}
	}

	#if (USE_TRIANGLE_STRIPS == 1)
	glDisable(GL_PRIMITIVE_RESTART);
	#endif
}



void CBasicMeshDrawer::DrawBorderMeshPatch(const MeshPatch& meshPatch, const CCamera* activeCam, uint32_t borderSide) const {
	if (meshPatch.visUpdateFrames[activeCam->GetCamType()] < globalRendering->drawFrame)
		return;

	const VAO& borderVAO = meshPatch.borderVertexBuffers[borderSide][drawPassLOD].vao;
	const VBO& borderIBO = lodBorderIndexBuffers[drawPassLOD];

	borderVAO.Bind();
	borderIBO.Bind(GL_ELEMENT_ARRAY_BUFFER);

	glPrimitiveRestartIndex(0xFFFF);
	glDrawElements(GL_TRIANGLE_STRIP, (borderIBO.GetSize() / sizeof(uint16_t)), GL_UNSIGNED_SHORT, borderIBO.GetPtr());

	borderIBO.Unbind();
	borderVAO.Unbind();
}

void CBasicMeshDrawer::DrawBorderMesh(const DrawPass::e& drawPass) {
	// border is always stripped
	glEnable(GL_PRIMITIVE_RESTART);

	const uint32_t npxm1 = numPatchesX - 1;
	const uint32_t npym1 = numPatchesY - 1;

	const CCamera* activeCam = CCameraHandler::GetActiveCamera();

	{
		// invert culling; index-pattern for T and R borders is also inverted
		glAttribStatePtr->FrontFace(GL_CW);

		if (drawPass != DrawPass::Shadow) {
			for (uint32_t px = 0; px < numPatchesX; px++) { smfGroundDrawer->SetupBigSquare( px  ,  0); DrawBorderMeshPatch(meshPatches[ 0 * numPatchesX +  px  ], activeCam, MAP_BORDER_T); }
			for (uint32_t py = 0; py < numPatchesY; py++) { smfGroundDrawer->SetupBigSquare(npxm1, py); DrawBorderMeshPatch(meshPatches[py * numPatchesX + npxm1], activeCam, MAP_BORDER_R); }
		} else {
			for (uint32_t px = 0; px < numPatchesX; px++) { DrawBorderMeshPatch(meshPatches[ 0 * numPatchesX +  px  ], activeCam, MAP_BORDER_T); }
			for (uint32_t py = 0; py < numPatchesY; py++) { DrawBorderMeshPatch(meshPatches[py * numPatchesX + npxm1], activeCam, MAP_BORDER_R); }
		}
	}
	{
		glAttribStatePtr->FrontFace(GL_CCW);

		if (drawPass != DrawPass::Shadow) {
			for (uint32_t px = 0; px < numPatchesX; px++) { smfGroundDrawer->SetupBigSquare(px, npym1); DrawBorderMeshPatch(meshPatches[npym1 * numPatchesX + px], activeCam, MAP_BORDER_B); }
			for (uint32_t py = 0; py < numPatchesY; py++) { smfGroundDrawer->SetupBigSquare( 0,  py  ); DrawBorderMeshPatch(meshPatches[ py   * numPatchesX +  0], activeCam, MAP_BORDER_L); }
		} else {
			for (uint32_t px = 0; px < numPatchesX; px++) { DrawBorderMeshPatch(meshPatches[npym1 * numPatchesX + px], activeCam, MAP_BORDER_B); }
			for (uint32_t py = 0; py < numPatchesY; py++) { DrawBorderMeshPatch(meshPatches[ py   * numPatchesX +  0], activeCam, MAP_BORDER_L); }
		}
	}

	glDisable(GL_PRIMITIVE_RESTART);
}

