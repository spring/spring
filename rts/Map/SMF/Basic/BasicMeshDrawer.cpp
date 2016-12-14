/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BasicMeshDrawer.h"
#include "Game/Camera.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Map/SMF/SMFGroundDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "System/EventHandler.h"

#ifndef glPrimitiveRestartIndex
#define glPrimitiveRestartIndex glPrimitiveRestartIndexNV
#endif

#define USE_TRIANGLE_STRIPS 1
#define USE_MIPMAP_BUFFERS  0
#define USE_PACKED_BUFFERS  1



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



CBasicMeshDrawer::CBasicMeshDrawer(CSMFReadMap* rm, CSMFGroundDrawer* gd)
	: CEventClient("[CBasicMeshDrawer]", 717171, false)
	, smfReadMap(rm)
	, smfGroundDrawer(gd)
{
	eventHandler.AddClient(this);

	numPatchesX = mapDims.mapx / PATCH_SIZE;
	numPatchesY = mapDims.mapy / PATCH_SIZE;
	drawPassLOD = 0;

	assert(numPatchesX >= 1);
	assert(numPatchesY >= 1);

	// f(x)=125.0*x^2 for x in [1,LOD_LEVELS]
	for (uint32_t n = 0; n < LOD_LEVELS; n++) {
		lodDistTable[n] = 125.0f * Square(n + 1);
	}

	meshPatches.resize(numPatchesX * numPatchesY);

	for (uint32_t y = 0; y < numPatchesY; y += 1) {
		for (uint32_t x = 0; x < numPatchesX; x += 1) {
			MeshPatch& patch = meshPatches[y * numPatchesX + x];

			patch.squareVertexPtrs.fill(nullptr);
			patch.squareNormalPtrs.fill(nullptr);

			patch.visUpdateFrames.fill(0);
			patch.uhmUpdateFrames.fill(0);
		}
	}

	for (uint32_t n = 0; n < LOD_LEVELS; n += 1) {
		UploadPatchIndices(n);
	}

	UnsyncedHeightMapUpdate(SRectangle{0, 0, mapDims.mapx, mapDims.mapy});
}

CBasicMeshDrawer::~CBasicMeshDrawer() {
	eventHandler.RemoveClient(this);
}



void CBasicMeshDrawer::Update() {
	CCamera* activeCam = CCamera::GetActiveCamera();
	MeshPatch* meshPatch = &meshPatches[0];

	patchVisTestDrawer.ResetState(activeCam, meshPatch, numPatchesX);

	activeCam->GetFrustumSides(readMap->GetCurrMinHeight() - 100.0f, readMap->GetCurrMaxHeight() + 100.0f, SQUARE_SIZE);
	readMap->GridVisibility(activeCam, &patchVisTestDrawer, 1e9, PATCH_SIZE);
}

void CBasicMeshDrawer::UnsyncedHeightMapUpdate(const SRectangle& rect) {
	const uint32_t minPatchX = std::max(rect.x1 / PATCH_SIZE,    (              0));
	const uint32_t minPatchY = std::max(rect.z1 / PATCH_SIZE,    (              0));
	const uint32_t maxPatchX = std::min(rect.x2 / PATCH_SIZE, int(numPatchesX - 1));
	const uint32_t maxPatchY = std::min(rect.z2 / PATCH_SIZE, int(numPatchesY - 1));
	const uint32_t lodLevels = std::max(1, LOD_LEVELS * USE_MIPMAP_BUFFERS);

	const float* heightMap = readMap->GetCornerHeightMapUnsynced();
	const float3* normalMap = readMap->GetVisVertexNormalsUnsynced();

	// TODO: update asynchronously
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
	MeshPatch& patch = meshPatches[py * numPatchesX + px];

	VBO& squareVertexBuffer = patch.squareVertexBuffers[n];
	#if (USE_PACKED_BUFFERS == 0)
	VBO& squareNormalBuffer = patch.squareNormalBuffers[n];
	#endif

	patch.uhmUpdateFrames[0] = globalRendering->drawFrame;

	const uint32_t lodStep  = 1 << n;
	const uint32_t lodVerts = (PATCH_SIZE / lodStep) + 1;

	const uint32_t bpx = px * PATCH_SIZE;
	const uint32_t bpy = py * PATCH_SIZE;

	uint32_t vertexIndx = 0;
	#if (USE_PACKED_BUFFERS == 0)
	uint32_t normalIndx = 0;
	#endif

	{
		squareVertexBuffer.Bind(GL_ARRAY_BUFFER);
		squareVertexBuffer.New((lodVerts * lodVerts) * sizeof(float3) * (USE_PACKED_BUFFERS + 1), GL_DYNAMIC_DRAW);

		float3* verts = patch.squareVertexPtrs[n];

		#ifdef GL_MAP_PERSISTENT_BIT
		if (verts == nullptr)
			verts = reinterpret_cast<float3*>(squareVertexBuffer.MapBuffer(GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_PERSISTENT_BIT));
		#endif

		if (verts == nullptr) {
			verts = reinterpret_cast<float3*>(squareVertexBuffer.MapBuffer());
		} else {
			patch.squareVertexPtrs[n] = verts;
		}

		assert(verts != nullptr);

		for (uint32_t vy = 0; vy < lodVerts; vy += 1) {
			for (uint32_t vx = 0; vx < lodVerts; vx += 1) {
				const uint32_t lvx = vx * lodStep;
				const uint32_t lvy = vy * lodStep;

				verts[vertexIndx  ].x = (bpx + lvx) * SQUARE_SIZE;
				verts[vertexIndx  ].z = (bpy + lvy) * SQUARE_SIZE;
				verts[vertexIndx++].y = chm[(bpy + lvy) * mapDims.mapxp1 + (bpx + lvx)];

				#if (USE_PACKED_BUFFERS == 1)
				verts[vertexIndx++] = cnm[(bpy + lvy) * mapDims.mapxp1 + (bpx + lvx)];
				#endif
			}
		}

		if (squareVertexBuffer.mapped)
			squareVertexBuffer.UnmapBuffer();

		squareVertexBuffer.Unbind();
	}
	#if (USE_PACKED_BUFFERS == 0)
	{
		squareNormalBuffer.Bind(GL_ARRAY_BUFFER);
		squareNormalBuffer.New((lodVerts * lodVerts) * sizeof(float3), GL_DYNAMIC_DRAW);

		float3* nrmls = patch.squareNormalPtrs[n];

		#ifdef GL_MAP_PERSISTENT_BIT
		if (nrmls == nullptr)
			nrmls = reinterpret_cast<float3*>(squareNormalBuffer.MapBuffer(GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_PERSISTENT_BIT));
		#endif

		if (nrmls == nullptr) {
			nrmls = reinterpret_cast<float3*>(squareNormalBuffer.MapBuffer());
		} else {
			patch.squareNormalPtrs[n] = nrmls;
		}

		assert(nrmls != nullptr);

		for (uint32_t vy = 0; vy < lodVerts; vy += 1) {
			for (uint32_t vx = 0; vx < lodVerts; vx += 1) {
				const uint32_t lvx = vx * lodStep;
				const uint32_t lvy = vy * lodStep;

				nrmls[normalIndx++] = cnm[(bpy + lvy) * mapDims.mapxp1 + (bpx + lvx)];
			}
		}

		if (squareNormalBuffer.mapped)
			squareNormalBuffer.UnmapBuffer();

		squareNormalBuffer.Unbind();
	}
	#endif
}

void CBasicMeshDrawer::UploadPatchBorderGeometry(uint32_t n, uint32_t px, uint32_t py, const float* chm, const float3* cnm) {
	MeshPatch& patch = meshPatches[py * numPatchesX + px];

	const uint32_t lodStep  = 1 << n;
	const uint32_t lodVerts = (PATCH_SIZE / lodStep) + 1;

	const uint32_t bpx = px * PATCH_SIZE;
	const uint32_t bpy = py * PATCH_SIZE;

	uint32_t vertexIndx = 0;

	if (px == 0) {
		vertexIndx = 0;

		VBO& borderVertexBuffer = patch.borderVertexBuffers[MAP_BORDER_L][n];
		VBO& borderNormalBuffer = patch.borderNormalBuffers[MAP_BORDER_L][n];

		{
			borderVertexBuffer.Bind(GL_ARRAY_BUFFER);
			borderVertexBuffer.New(lodVerts * sizeof(float3) * 2, GL_DYNAMIC_DRAW);

			float3* verts = reinterpret_cast<float3*>(borderVertexBuffer.MapBuffer());
			assert(verts != nullptr);

			for (uint32_t vy = 0; vy < lodVerts; vy += 1) {
				verts[vertexIndx           ].x = 0.0f;
				verts[vertexIndx + lodVerts].x = 0.0f;
				verts[vertexIndx           ].z =     (bpy + vy * lodStep) * SQUARE_SIZE;
				verts[vertexIndx + lodVerts].z =     (bpy + vy * lodStep) * SQUARE_SIZE;
				verts[vertexIndx           ].y = chm[(bpy + vy * lodStep) * mapDims.mapxp1 + 0];
				verts[vertexIndx + lodVerts].y = std::min(readMap->GetInitMinHeight(), -500.0f);

				vertexIndx += 1;
			}

			borderVertexBuffer.UnmapBuffer();
			borderVertexBuffer.Unbind();
		}

		// terrain normals point up, borders do not
		UploadPatchBorderNormals(borderNormalBuffer, -RgtVector, lodVerts);
	}
	if (px == (numPatchesX - 1)) {
		vertexIndx = 0;

		VBO& borderVertexBuffer = patch.borderVertexBuffers[MAP_BORDER_R][n];
		VBO& borderNormalBuffer = patch.borderNormalBuffers[MAP_BORDER_R][n];

		{
			borderVertexBuffer.Bind(GL_ARRAY_BUFFER);
			borderVertexBuffer.New(lodVerts * sizeof(float3) * 2, GL_DYNAMIC_DRAW);

			float3* verts = reinterpret_cast<float3*>(borderVertexBuffer.MapBuffer());
			assert(verts != nullptr);

			for (uint32_t vy = 0; vy < lodVerts; vy += 1) {
				verts[vertexIndx           ].x = mapDims.mapx * SQUARE_SIZE;
				verts[vertexIndx + lodVerts].x = mapDims.mapx * SQUARE_SIZE;
				verts[vertexIndx           ].z =     (bpy + vy * lodStep) * SQUARE_SIZE;
				verts[vertexIndx + lodVerts].z =     (bpy + vy * lodStep) * SQUARE_SIZE;
				verts[vertexIndx           ].y = chm[(bpy + vy * lodStep) * mapDims.mapxp1 + mapDims.mapx];
				verts[vertexIndx + lodVerts].y = std::min(readMap->GetInitMinHeight(), -500.0f);

				vertexIndx += 1;
			}

			borderVertexBuffer.UnmapBuffer();
			borderVertexBuffer.Unbind();
		}

		UploadPatchBorderNormals(borderNormalBuffer, RgtVector, lodVerts);
	}

	if (py == 0) {
		vertexIndx = 0;

		VBO& borderVertexBuffer = patch.borderVertexBuffers[MAP_BORDER_T][n];
		VBO& borderNormalBuffer = patch.borderNormalBuffers[MAP_BORDER_T][n];

		{
			borderVertexBuffer.Bind(GL_ARRAY_BUFFER);
			borderVertexBuffer.New(lodVerts * sizeof(float3) * 2, GL_DYNAMIC_DRAW);

			float3* verts = reinterpret_cast<float3*>(borderVertexBuffer.MapBuffer());
			assert(verts != nullptr);

			for (uint32_t vx = 0; vx < lodVerts; vx += 1) {
				verts[vertexIndx           ].x = (bpx + vx * lodStep) * SQUARE_SIZE;
				verts[vertexIndx + lodVerts].x = (bpx + vx * lodStep) * SQUARE_SIZE;
				verts[vertexIndx           ].z = 0.0f;
				verts[vertexIndx + lodVerts].z = 0.0f;
				verts[vertexIndx           ].y = chm[0 + (bpx + vx * lodStep)];
				verts[vertexIndx + lodVerts].y = std::min(readMap->GetInitMinHeight(), -500.0f);

				vertexIndx += 1;
			}

			borderVertexBuffer.UnmapBuffer();
			borderVertexBuffer.Unbind();
		}

		UploadPatchBorderNormals(borderNormalBuffer, -FwdVector, lodVerts);
	}
	if (py == (numPatchesY - 1)) {
		vertexIndx = 0;

		VBO& borderVertexBuffer = patch.borderVertexBuffers[MAP_BORDER_B][n];
		VBO& borderNormalBuffer = patch.borderNormalBuffers[MAP_BORDER_B][n];

		{
			borderVertexBuffer.Bind(GL_ARRAY_BUFFER);
			borderVertexBuffer.New(lodVerts * sizeof(float3) * 2, GL_DYNAMIC_DRAW);

			float3* verts = reinterpret_cast<float3*>(borderVertexBuffer.MapBuffer());
			assert(verts != nullptr);

			for (uint32_t vx = 0; vx < lodVerts; vx += 1) {
				verts[vertexIndx           ].x = (bpx + vx * lodStep) * SQUARE_SIZE;
				verts[vertexIndx + lodVerts].x = (bpx + vx * lodStep) * SQUARE_SIZE;
				verts[vertexIndx           ].z = mapDims.mapy * SQUARE_SIZE;
				verts[vertexIndx + lodVerts].z = mapDims.mapy * SQUARE_SIZE;
				verts[vertexIndx           ].y = chm[mapDims.mapy * mapDims.mapxp1 + (bpx + vx * lodStep)];
				verts[vertexIndx + lodVerts].y = std::min(readMap->GetInitMinHeight(), -500.0f);

				vertexIndx += 1;
			}

			borderVertexBuffer.UnmapBuffer();
			borderVertexBuffer.Unbind();
		}

		UploadPatchBorderNormals(borderNormalBuffer, FwdVector, lodVerts);
	}
}

void CBasicMeshDrawer::UploadPatchBorderNormals(VBO& normalBuffer, const float3& normalVector, uint32_t lodVerts) {
	// only have to upload these once
	if (normalBuffer.GetSize() != 0)
		return;

	normalBuffer.Bind(GL_ARRAY_BUFFER);
	normalBuffer.New(lodVerts * sizeof(float3) * 2, GL_STATIC_DRAW);

	float3* nrmls = reinterpret_cast<float3*>(normalBuffer.MapBuffer());
	assert(nrmls != nullptr);

	uint32_t normalIndx = 0;

	for (uint32_t vi = 0; vi < lodVerts; vi += 1) {
		nrmls[normalIndx           ] = normalVector;
		nrmls[normalIndx + lodVerts] = normalVector;

		normalIndx += 1;
	}

	normalBuffer.UnmapBuffer();
	normalBuffer.Unbind();
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

	VBO& squareIndexBuffer = lodSquareIndexBuffers[n];
	VBO& borderIndexBuffer = lodBorderIndexBuffers[n];

	{
		squareIndexBuffer.Bind(GL_ELEMENT_ARRAY_BUFFER);
		#if (USE_TRIANGLE_STRIPS == 0)
		squareIndexBuffer.New((numPolys / (lodStep * lodStep)) * 3 * sizeof(uint16_t), GL_STATIC_DRAW);
		#else
		squareIndexBuffer.New(((lodQuads * 2 + 3) * lodQuads) * sizeof(uint16_t), GL_STATIC_DRAW);
		#endif

		uint16_t* indcs = reinterpret_cast<uint16_t*>(squareIndexBuffer.MapBuffer());

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

		squareIndexBuffer.UnmapBuffer();
		squareIndexBuffer.Unbind();
	}
	{
		borderIndexBuffer.Bind(GL_ELEMENT_ARRAY_BUFFER);
		borderIndexBuffer.New(((lodQuads * 2 + 3) * 1) * sizeof(uint16_t), GL_STATIC_DRAW);

		uint16_t* indcs = reinterpret_cast<uint16_t*>(borderIndexBuffer.MapBuffer());

		uint32_t indxCtr = 0;
		// uint32_t quadCtr = 0;

		for (uint32_t vi = 0; vi < lodQuads; vi += 1) {
			indcs[indxCtr++] = (vi               ); // A
			indcs[indxCtr++] = (vi +     lodVerts); // C
		}

		indcs[indxCtr++] = (lodQuads           ); // B
		indcs[indxCtr++] = (lodQuads + lodVerts); // D
		indcs[indxCtr++] = 0xFFFF;

		borderIndexBuffer.UnmapBuffer();
		borderIndexBuffer.Unbind();
	}
}



uint32_t CBasicMeshDrawer::CalcDrawPassLOD(const CCamera* cam, const DrawPass::e& drawPass) const {
	// higher detail biases LOD-step toward a smaller value
	// NOTE: should perhaps prevent an insane initial bias?
	int32_t lodBias = smfGroundDrawer->GetGroundDetail(drawPass) % LOD_LEVELS;
	int32_t lodIndx = LOD_LEVELS - 1;

	// force SP and NP to equal LOD; avoids projection issues
	if (drawPass == DrawPass::Shadow)
		cam = CCamera::GetCamera(CCamera::CAMTYPE_PLAYER);

	{
		const CUnit* hitUnit = nullptr;
		const CFeature* hitFeature = nullptr;

		float mapRayDist = 0.0f;

		if ((mapRayDist = TraceRay::GuiTraceRay(cam->GetPos(), cam->GetDir(), globalRendering->viewRange, nullptr, hitUnit, hitFeature, false, true, true)) < 0.0f)
			mapRayDist = CGround::LinePlaneCol(cam->GetPos(), cam->GetDir(), globalRendering->viewRange, readMap->GetCurrMinHeight());
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



void CBasicMeshDrawer::DrawSquareMeshPatch(const MeshPatch& meshPatch, const VBO& indexBuffer, const CCamera* activeCam) const {
	const VBO& vertexBuffer = meshPatch.squareVertexBuffers[drawPassLOD * USE_MIPMAP_BUFFERS];
	#if (USE_PACKED_BUFFERS == 0)
	const VBO& normalBuffer = meshPatch.squareNormalBuffers[drawPassLOD * USE_MIPMAP_BUFFERS];
	#endif

	#if (USE_PACKED_BUFFERS == 0)
		vertexBuffer.Bind(GL_ARRAY_BUFFER);
		assert(vertexBuffer.GetPtr() == nullptr);
		glVertexPointer(3, GL_FLOAT, sizeof(float3), vertexBuffer.GetPtr());
		vertexBuffer.Unbind();

		normalBuffer.Bind(GL_ARRAY_BUFFER);
		assert(normalBuffer.GetPtr() == nullptr);
		glNormalPointer(GL_FLOAT, sizeof(float3), normalBuffer.GetPtr());
		normalBuffer.Unbind();
	#else
		vertexBuffer.Bind(GL_ARRAY_BUFFER);
		assert(vertexBuffer.GetPtr() == nullptr);
		glVertexPointer(3, GL_FLOAT, sizeof(float3) * 2, 0);
		glNormalPointer(GL_FLOAT, sizeof(float3), reinterpret_cast<void*>(sizeof(float3)));
		vertexBuffer.Unbind();
	#endif

	#if (USE_TRIANGLE_STRIPS == 0)
		glDrawElements(GL_TRIANGLES, (indexBuffer.GetSize() / sizeof(uint16_t)), GL_UNSIGNED_SHORT, indexBuffer.GetPtr());
		// glDrawRangeElements(GL_TRIANGLES, 0, (lodVerts * lodVerts) - 1, (indexBuffer.GetSize() / sizeof(uint16_t)), GL_UNSIGNED_SHORT, indexBuffer.GetPtr());
	#else
		glPrimitiveRestartIndex(0xFFFF);
		glDrawElements(GL_TRIANGLE_STRIP, (indexBuffer.GetSize() / sizeof(uint16_t)), GL_UNSIGNED_SHORT, indexBuffer.GetPtr());
	#endif
}

void CBasicMeshDrawer::DrawMesh(const DrawPass::e& drawPass) {
	Update();

	const CCamera* activeCam = CCamera::GetActiveCamera();
	const VBO& indexBuffer = lodSquareIndexBuffers[drawPassLOD = CalcDrawPassLOD(activeCam, drawPass)];

	indexBuffer.Bind(GL_ELEMENT_ARRAY_BUFFER);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	#if (USE_TRIANGLE_STRIPS == 1)
	glEnable(GL_PRIMITIVE_RESTART);
	#endif

	#if (USE_TRIANGLE_STRIPS == 0)
	const uint32_t lodStep  = 1 << drawPassLOD;
	const uint32_t lodQuads = (PATCH_SIZE / lodStep);
	const uint32_t numIndcs = indexBuffer.GetSize() / sizeof(uint16_t);

	assert(numIndcs == (lodQuads * lodQuads * 2 * 3));
	assert(indexBuffer.GetPtr() == nullptr);
	#endif

	for (uint32_t py = 0; py < numPatchesY; py += 1) {
		for (uint32_t px = 0; px < numPatchesX; px += 1) {
			const MeshPatch& meshPatch = meshPatches[py * numPatchesX + px];

			if (meshPatch.visUpdateFrames[activeCam->GetCamType()] < globalRendering->drawFrame)
				continue;

			if (drawPass != DrawPass::Shadow)
				smfGroundDrawer->SetupBigSquare(px, py);

			DrawSquareMeshPatch(meshPatch, indexBuffer, activeCam);
		}
	}

	#if (USE_TRIANGLE_STRIPS == 1)
	glDisable(GL_PRIMITIVE_RESTART);
	#endif
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	indexBuffer.Unbind();
}


void CBasicMeshDrawer::DrawBorderMeshPatch(const MeshPatch& meshPatch, const VBO& indexBuffer, const CCamera* activeCam, uint32_t borderSide) const {
	if (meshPatch.visUpdateFrames[activeCam->GetCamType()] < globalRendering->drawFrame)
		return;

	const VBO& vertexBuffer = meshPatch.borderVertexBuffers[borderSide][drawPassLOD];
	const VBO& normalBuffer = meshPatch.borderNormalBuffers[borderSide][drawPassLOD];

	vertexBuffer.Bind(GL_ARRAY_BUFFER);
	assert(vertexBuffer.GetPtr() == nullptr);
	glVertexPointer(3, GL_FLOAT, sizeof(float3), vertexBuffer.GetPtr());
	vertexBuffer.Unbind();

	normalBuffer.Bind(GL_ARRAY_BUFFER);
	assert(normalBuffer.GetPtr() == nullptr);
	glNormalPointer(GL_FLOAT, sizeof(float3), normalBuffer.GetPtr());
	normalBuffer.Unbind();

	glPrimitiveRestartIndex(0xFFFF);
	glDrawElements(GL_TRIANGLE_STRIP, (indexBuffer.GetSize() / sizeof(uint16_t)), GL_UNSIGNED_SHORT, indexBuffer.GetPtr());
}

void CBasicMeshDrawer::DrawBorderMesh(const DrawPass::e& drawPass) {
	const CCamera* activeCam = CCamera::GetActiveCamera();
	const VBO& indexBuffer = lodBorderIndexBuffers[drawPassLOD];

	indexBuffer.Bind(GL_ELEMENT_ARRAY_BUFFER);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnable(GL_PRIMITIVE_RESTART);

	const uint32_t npxm1 = numPatchesX - 1;
	const uint32_t npym1 = numPatchesY - 1;

	{
		// invert culling; index-pattern for T and R borders is also inverted
		glFrontFace(GL_CW);

		if (drawPass != DrawPass::Shadow) {
			for (uint32_t px = 0; px < numPatchesX; px++) { smfGroundDrawer->SetupBigSquare( px  ,  0); DrawBorderMeshPatch(meshPatches[ 0 * numPatchesX +  px  ], indexBuffer, activeCam, MAP_BORDER_T); }
			for (uint32_t py = 0; py < numPatchesY; py++) { smfGroundDrawer->SetupBigSquare(npxm1, py); DrawBorderMeshPatch(meshPatches[py * numPatchesX + npxm1], indexBuffer, activeCam, MAP_BORDER_R); }
		} else {
			for (uint32_t px = 0; px < numPatchesX; px++) { DrawBorderMeshPatch(meshPatches[ 0 * numPatchesX +  px  ], indexBuffer, activeCam, MAP_BORDER_T); }
			for (uint32_t py = 0; py < numPatchesY; py++) { DrawBorderMeshPatch(meshPatches[py * numPatchesX + npxm1], indexBuffer, activeCam, MAP_BORDER_R); }
		}
	}
	{
		glFrontFace(GL_CCW);

		if (drawPass != DrawPass::Shadow) {
			for (uint32_t px = 0; px < numPatchesX; px++) { smfGroundDrawer->SetupBigSquare(px, npym1); DrawBorderMeshPatch(meshPatches[npym1 * numPatchesX + px], indexBuffer, activeCam, MAP_BORDER_B); }
			for (uint32_t py = 0; py < numPatchesY; py++) { smfGroundDrawer->SetupBigSquare( 0,  py  ); DrawBorderMeshPatch(meshPatches[ py   * numPatchesX +  0], indexBuffer, activeCam, MAP_BORDER_L); }
		} else {
			for (uint32_t px = 0; px < numPatchesX; px++) { DrawBorderMeshPatch(meshPatches[npym1 * numPatchesX + px], indexBuffer, activeCam, MAP_BORDER_B); }
			for (uint32_t py = 0; py < numPatchesY; py++) { DrawBorderMeshPatch(meshPatches[ py   * numPatchesX +  0], indexBuffer, activeCam, MAP_BORDER_L); }
		}
	}

	glDisable(GL_PRIMITIVE_RESTART);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	indexBuffer.Unbind();
}

