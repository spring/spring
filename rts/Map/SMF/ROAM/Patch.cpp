/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

//
// ROAM Simplistic Implementation
// Added to Spring by Peter Sarkozy (mysterme AT gmail DOT com)
// Billion thanks to Bryan Turner (Jan, 2000)
//                    brturn@bellsouth.net
//
// Based on the Tread Marks engine by Longbow Digital Arts
//                               (www.LongbowDigitalArts.com)
// Much help and hints provided by Seumas McNally, LDA.
//

#include "Patch.h"
#include "RoamMeshDrawer.h"
#include "Game/Camera.h"
#include "Map/ReadMap.h"
#include "Map/SMF/SMFGroundDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/VertexArray.h"
#include "System/Log/ILog.h"
#include "System/ThreadPool.h"
#include "System/TimeProfiler.h"

#include <climits>

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
// STATICS


static int MAX_POOL_SIZE = 8000000;

Patch::RenderMode Patch::renderMode = Patch::VBO;

// one pool per thread
static size_t poolSize = 0;
static std::vector<CTriNodePool> pools[CRoamMeshDrawer::MESH_COUNT];

void CTriNodePool::InitPools(bool shadowPass, size_t newPoolSize)
{
	if (!pools[shadowPass].empty())
		return;

	// int numThreads = GetNumThreads();
	int numThreads = ThreadPool::GetMaxThreads();
	const size_t allocPerThread = std::max(newPoolSize / numThreads, newPoolSize / 3) & (~(size_t)0x1);

	try {
		poolSize = newPoolSize;
		pools[shadowPass].reserve(numThreads);
		for (; numThreads > 0; --numThreads) {
			pools[shadowPass].emplace_back(allocPerThread);
		}
	} catch(const std::bad_alloc& e) {
		LOG_L(L_FATAL, "Failed to allocate memory for ROAM (reducing pool size): %s", e.what());
		MAX_POOL_SIZE = newPoolSize * 0.75f;
		FreePools(shadowPass);
		InitPools(shadowPass, MAX_POOL_SIZE);
	}
}

void CTriNodePool::FreePools(bool shadowPass)
{
	pools[shadowPass].clear();
}


void CTriNodePool::ResetAll(bool shadowPass)
{
	bool outOfNodes = false;

	for (CTriNodePool& pool: pools[shadowPass]) {
		outOfNodes |= pool.OutOfNodes();
		pool.Reset();
	}

	if (outOfNodes && (poolSize < MAX_POOL_SIZE)) {
		FreePools(shadowPass);
		InitPools(shadowPass, std::min<size_t>(poolSize * 2, MAX_POOL_SIZE));
	}
}


CTriNodePool* CTriNodePool::GetPool(bool shadowPass)
{
	return &(pools[shadowPass][ThreadPool::GetThreadNum()]);
}


// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
// CTriNodePool Class

CTriNodePool::CTriNodePool(const size_t poolSize)
{
	assert((poolSize & 0x1) == 0); // we always allocate left & right, so we need an even pool
	m_NextTriNode = 0;
	pool.resize(poolSize);
}


void CTriNodePool::Reset()
{
	// reinit all entries to NULL
	// this saves use calling TriTreeNode's ctor which is slower than a memset
	if (m_NextTriNode > 0)
		memset(&pool[0], 0, sizeof(TriTreeNode) * m_NextTriNode);

	m_NextTriNode = 0;
}


void CTriNodePool::Allocate(TriTreeNode*& left, TriTreeNode*& right)
{
	// IF we've run out of TriTreeNodes, just return NULL (this is handled gracefully)
	if (OutOfNodes())
		return;

	left  = &(pool[m_NextTriNode++]);
	right = &(pool[m_NextTriNode++]);
}


// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
// Patch Class
//

Patch::Patch()
	: smfGroundDrawer(nullptr)
	, currentVariance(nullptr)
	, currentPool(nullptr)
	, isDirty(true)
	, vboVerticesUploaded(false)
	, varianceMaxLimit(std::numeric_limits<float>::max())
	, camDistLODFactor(1.0f)
	, coors(-1, -1)
	, triList(0)
	, vertexBuffer(0)
	, vertexIndexBuffer(0)
{
	varianceLeft.resize(1 << VARIANCE_DEPTH);
	varianceRight.resize(1 << VARIANCE_DEPTH);

	// NOTE:
	//   shadow-mesh patches are only ever viewed by one camera
	//   normal-mesh patches can be viewed by *multiple* types!
	lastDrawFrames.resize(CCamera::CAMTYPE_VISCUL);
}

Patch::~Patch()
{
	glDeleteLists(triList, 1);

	if (GLEW_ARB_vertex_buffer_object) {
		glDeleteBuffersARB(1, &vertexBuffer);
		glDeleteBuffersARB(1, &vertexIndexBuffer);
	}

	triList = 0;
	vertexBuffer = 0;
	vertexIndexBuffer = 0;
}

void Patch::Init(CSMFGroundDrawer* _drawer, int patchX, int patchZ)
{
	coors.x = patchX;
	coors.y = patchZ;

	smfGroundDrawer = _drawer;

	// Store pointer to first byte of the height data for this patch.


	// Attach the two m_Base triangles together
	baseLeft.BaseNeighbor  = &baseRight;
	baseRight.BaseNeighbor = &baseLeft;

	// Create used OpenGL objects
	triList = glGenLists(1);

	if (GLEW_ARB_vertex_buffer_object) {
		glGenBuffersARB(1, &vertexBuffer);
		glGenBuffersARB(1, &vertexIndexBuffer);
	}


	vertices.resize(3 * (PATCH_SIZE + 1) * (PATCH_SIZE + 1));
	unsigned int index = 0;

	// initialize vertices
	for (int z = coors.y; z <= (coors.y + PATCH_SIZE); z++) {
		for (int x = coors.x; x <= (coors.x + PATCH_SIZE); x++) {
			vertices[index++] = x * SQUARE_SIZE;
			vertices[index++] = 0.0f;
			vertices[index++] = z * SQUARE_SIZE;
		}
	}

	UpdateHeightMap();
}

void Patch::Reset()
{
	// Reset the important relationships
	baseLeft  = TriTreeNode();
	baseRight = TriTreeNode();

	// Attach the two m_Base triangles together
	baseLeft.BaseNeighbor  = &baseRight;
	baseRight.BaseNeighbor = &baseLeft;
}


void Patch::UpdateHeightMap(const SRectangle& rect)
{
	const float* hMap = readMap->GetCornerHeightMapUnsynced();

	for (int z = rect.z1; z <= rect.z2; z++) {
		for (int x = rect.x1; x <= rect.x2; x++) {
			const int vindex = (z * (PATCH_SIZE + 1) + x) * 3;

			const int xw = x + coors.x;
			const int zw = z + coors.y;

			// only update y-coord
			vertices[vindex + 1] = hMap[zw * mapDims.mapxp1 + xw];
		}
	}

	VBOUploadVertices();
	isDirty = true;
}


void Patch::VBOUploadVertices()
{
	if (renderMode == VBO) {
		// Upload vertexBuffer
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, vertexBuffer);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW_ARB);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

		vboVerticesUploaded = true;
	} else {
		vboVerticesUploaded = false;
	}
}


// -------------------------------------------------------------------------------------------------
// Split a single Triangle and link it into the mesh.
// Will correctly force-split diamonds.
//
void Patch::Split(TriTreeNode* tri)
{
	// We are already split, no need to do it again.
	if (!tri->IsLeaf())
		return;

	// If this triangle is not in a proper diamond, force split our base neighbor
	if (tri->BaseNeighbor && (tri->BaseNeighbor->BaseNeighbor != tri))
		Split(tri->BaseNeighbor);

	// Create children and link into mesh
	currentPool->Allocate(tri->LeftChild, tri->RightChild);

	// If creation failed, just exit.
	if (!tri->IsBranch()) {
		// make sure both nodes are NULL if just the right one failed
		// special handling the cause that only one of them is NULL wouldn't make sense (only less performance)
		tri->LeftChild  = nullptr;
		tri->RightChild = nullptr;
		return;
	}

	// Fill in the information we can get from the parent (neighbor pointers)
	tri->LeftChild->BaseNeighbor = tri->LeftNeighbor;
	tri->LeftChild->LeftNeighbor = tri->RightChild;

	tri->RightChild->BaseNeighbor = tri->RightNeighbor;
	tri->RightChild->RightNeighbor = tri->LeftChild;

	// Link our Left Neighbor to the new children
	if (tri->LeftNeighbor != nullptr) {
		if (tri->LeftNeighbor->BaseNeighbor == tri)
			tri->LeftNeighbor->BaseNeighbor = tri->LeftChild;
		else if (tri->LeftNeighbor->LeftNeighbor == tri)
			tri->LeftNeighbor->LeftNeighbor = tri->LeftChild;
		else if (tri->LeftNeighbor->RightNeighbor == tri)
			tri->LeftNeighbor->RightNeighbor = tri->LeftChild;
		else
			;// Illegal Left Neighbor!
	}

	// Link our Right Neighbor to the new children
	if (tri->RightNeighbor != nullptr) {
		if (tri->RightNeighbor->BaseNeighbor == tri)
			tri->RightNeighbor->BaseNeighbor = tri->RightChild;
		else if (tri->RightNeighbor->RightNeighbor == tri)
			tri->RightNeighbor->RightNeighbor = tri->RightChild;
		else if (tri->RightNeighbor->LeftNeighbor == tri)
			tri->RightNeighbor->LeftNeighbor = tri->RightChild;
		else
			;// Illegal Right Neighbor!
	}

	// Link our Base Neighbor to the new children
	if (tri->BaseNeighbor != nullptr) {
		if (tri->BaseNeighbor->IsBranch()) {
			tri->BaseNeighbor->LeftChild->RightNeighbor = tri->RightChild;
			tri->BaseNeighbor->RightChild->LeftNeighbor = tri->LeftChild;
			tri->LeftChild->RightNeighbor = tri->BaseNeighbor->RightChild;
			tri->RightChild->LeftNeighbor = tri->BaseNeighbor->LeftChild;
		} else {
			// Base Neighbor (in a diamond with us) was not split yet, so do that now.
			Split(tri->BaseNeighbor);
		}
	} else {
		// An edge triangle, trivial case.
		tri->LeftChild->RightNeighbor = nullptr;
		tri->RightChild->LeftNeighbor = nullptr;
	}
}


// ---------------------------------------------------------------------
// Tessellate a Patch.
// Will continue to split until the variance metric is met.
//
void Patch::RecursTessellate(TriTreeNode* tri, const int2 left, const int2 right, const int2 apex, const int node)
{
	// bail if we can not tessellate further in at least one dimension
	if ((abs(left.x - right.x) <= 1) && (abs(left.y - right.y) <= 1))
		return;

	// default > 1; when variance isn't saved this issues further tessellation
	float TriVariance = 10.0f;

	if (node < (1 << VARIANCE_DEPTH)) {
		// make max tessellation viewRadius dependent
		// w/o this huge cliffs cause huge variances and will always tessellate
		// fully independent of camdist (-> huge/distfromcam ~= huge)
		const float myVariance = std::min(currentVariance[node], varianceMaxLimit);

		const int sizeX = std::max(left.x - right.x, right.x - left.x);
		const int sizeY = std::max(left.y - right.y, right.y - left.y);
		const int size  = std::max(sizeX, sizeY);

		// take distance, variance and patch size into consideration
		TriVariance = (myVariance * PATCH_SIZE * size) * camDistLODFactor;
	}

	// stop tesselation
	if (TriVariance <= 1.0f)
		return;

	Split(tri);

	if (tri->IsBranch()) {
		// triangle was split, also try to split its children
		const int2 center = {(left.x + right.x) >> 1, (left.y + right.y) >> 1};

		RecursTessellate(tri->LeftChild,  apex,  left, center, (node << 1)    );
		RecursTessellate(tri->RightChild, right, apex, center, (node << 1) + 1);
	}
}


// ---------------------------------------------------------------------
// Render the tree.
//

void Patch::RecursRender(const TriTreeNode* tri, const int2 left, const int2 right, const int2 apex)
{
	if (tri->IsLeaf()) {
		indices.push_back(apex.x  + apex.y  * (PATCH_SIZE + 1));
		indices.push_back(left.x  + left.y  * (PATCH_SIZE + 1));
		indices.push_back(right.x + right.y * (PATCH_SIZE + 1));
		return;
	}

	const int2 center = {(left.x + right.x) >> 1, (left.y + right.y) >> 1};

	RecursRender(tri->LeftChild,  apex,  left, center);
	RecursRender(tri->RightChild, right, apex, center);
}


void Patch::GenerateIndices()
{
	indices.clear();
	RecursRender(&baseLeft,  int2(         0, PATCH_SIZE), int2(PATCH_SIZE,          0), int2(         0,          0));
	RecursRender(&baseRight, int2(PATCH_SIZE,          0), int2(         0, PATCH_SIZE), int2(PATCH_SIZE, PATCH_SIZE));
}

float Patch::GetHeight(int2 pos)
{
	const int vindex = (pos.y * (PATCH_SIZE + 1) + pos.x) * 3 + 1;
	assert(readMap->GetCornerHeightMapUnsynced()[(coors.y + pos.y) * mapDims.mapxp1 + (coors.x + pos.x)] == vertices[vindex]);
	return vertices[vindex];
}

// ---------------------------------------------------------------------
// Computes Variance over the entire tree.  Does not examine node relationships.
//
float Patch::RecursComputeVariance(
	const   int2 left,
	const   int2 rght,
	const   int2 apex,
	const float3 hgts,
	const    int node
) {
	/*      A
	 *     /|\
	 *    / | \
	 *   /  |  \
	 *  /   |   \
	 * L----M----R
	 *
	 * first compute the XZ coordinates of 'M' (hypotenuse middle)
	 */
	const int2 mpos = {(left.x + rght.x) >> 1, (left.y + rght.y) >> 1};

	// get the height value at M

	const float mhgt = GetHeight(mpos);

	// variance of this triangle is the actual height at its hypotenuse
	// midpoint minus the interpolated height; use values passed on the
	// stack instead of re-accessing the heightmap
	float myVariance = math::fabs(mhgt - ((hgts.x + hgts.y) * 0.5f));

	// shore lines get more variance for higher accuracy
	// NOTE: .x := height(L), .y := height(R), .z := height(A)
	//
	if ((hgts.x * hgts.y) < 0.0f || (hgts.x * mhgt) < 0.0f || (hgts.y * mhgt) < 0.0f)
		myVariance = std::max(myVariance * 1.5f, 20.0f);

	// myVariance = MAX(abs(left.x - rght.x), abs(left.y - rght.y)) * myVariance;

	// save some CPU, only calculate variance down to a 4x4 block
	if ((abs(left.x - rght.x) >= 4) || (abs(left.y - rght.y) >= 4)) {
		const float3 hgts1 = {hgts.z, hgts.x, mhgt};
		const float3 hgts2 = {hgts.y, hgts.z, mhgt};

		const float child1Variance = RecursComputeVariance(apex, left, mpos, hgts1, (node << 1)    );
		const float child2Variance = RecursComputeVariance(rght, apex, mpos, hgts2, (node << 1) + 1);

		// final variance for this node is the max of its own variance and that of its children
		myVariance = std::max(myVariance, child1Variance);
		myVariance = std::max(myVariance, child2Variance);
	}

	// NOTE: Variance is never zero
	myVariance = std::max(0.001f, myVariance);

	// store the final variance for this node
	if (node < (1 << VARIANCE_DEPTH))
		currentVariance[node] = myVariance;

	return myVariance;
}


// ---------------------------------------------------------------------
// Compute the variance tree for each of the Binary Triangles in this patch.
//
void Patch::ComputeVariance()
{
	{
		currentVariance = &varianceLeft[0];

		const   int2 left = {         0, PATCH_SIZE};
		const   int2 rght = {PATCH_SIZE,          0};
		const   int2 apex = {         0,          0};
		const float3 hgts = {
			GetHeight(left),
			GetHeight(rght),
			GetHeight(apex),
		};

		RecursComputeVariance(left, rght, apex, hgts, 1);
	}

	{
		currentVariance = &varianceRight[0];

		const   int2 left = {PATCH_SIZE,          0};
		const   int2 rght = {         0, PATCH_SIZE};
		const   int2 apex = {PATCH_SIZE, PATCH_SIZE};
		const float3 hgts = {
			GetHeight(left),
			GetHeight(rght),
			GetHeight(apex),
		};

		RecursComputeVariance(left, rght, apex, hgts, 1);
	}

	// Clear the dirty flag for this patch
	isDirty = false;
}


// ---------------------------------------------------------------------
// Create an approximate mesh.
//
bool Patch::Tessellate(const float3& campos, int groundDetail, bool shadowPass)
{
	// Set/Update LOD params (FIXME: wrong height?)
	const float myx = (coors.x + PATCH_SIZE / 2) * SQUARE_SIZE;
	const float myz = (coors.y + PATCH_SIZE / 2) * SQUARE_SIZE;
	const float myy = (readMap->GetCurrMinHeight() + readMap->GetCurrMaxHeight()) * 0.5f;
	const float3 myPos(myx, myy, myz);
	currentPool = CTriNodePool::GetPool(shadowPass);

	camDistLODFactor  = myPos.distance(campos);
	camDistLODFactor *= 300.0f / groundDetail; // MAGIC NUMBER 1: increase the dividend to reduce LOD in camera distance
	camDistLODFactor  = std::max(1.0f, camDistLODFactor);
	camDistLODFactor  = 1.0f / camDistLODFactor;

	// MAGIC NUMBER 2:
	//   variances are clamped by it, so it regulates how strong areas are tessellated.
	//   Note, the maximum tessellation is untouched by it. Instead it reduces the maximum
	//   LOD in distance, while the param above defines the overall FallOff rate.
	varianceMaxLimit = groundDetail * 0.35f;

	{
		// Split each of the base triangles
		currentVariance = &varianceLeft[0];

		const int2 left = {coors.x,              coors.y + PATCH_SIZE};
		const int2 rght = {coors.x + PATCH_SIZE, coors.y             };
		const int2 apex = {coors.x,              coors.y             };

		RecursTessellate(&baseLeft, left, rght, apex, 1);
	}
	{
		currentVariance = &varianceRight[0];

		const int2 left = {coors.x + PATCH_SIZE, coors.y             };
		const int2 rght = {coors.x,              coors.y + PATCH_SIZE};
		const int2 apex = {coors.x + PATCH_SIZE, coors.y + PATCH_SIZE};

		RecursTessellate(&baseRight, left, rght, apex, 1);
	}

	return (!currentPool->OutOfNodes());
}


// ---------------------------------------------------------------------
// Render the mesh.
//

void Patch::Draw()
{
	switch (renderMode) {
		case VA: {
			glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(3, GL_FLOAT, 0, &vertices[0]);
				glDrawRangeElements(GL_TRIANGLES, 0, vertices.size(), indices.size(), GL_UNSIGNED_INT, &indices[0]);
			glDisableClientState(GL_VERTEX_ARRAY);
		} break;

		case DL: {
			glCallList(triList);
		} break;

		case VBO: {
			// enable VBOs
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, vertexBuffer); // coors
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, vertexIndexBuffer); // indices

				glEnableClientState(GL_VERTEX_ARRAY);
					glVertexPointer(3, GL_FLOAT, 0, 0); // last param is offset, not ptr
					glDrawRangeElements(GL_TRIANGLES, 0, vertices.size(), indices.size(), GL_UNSIGNED_INT, 0);
				glDisableClientState(GL_VERTEX_ARRAY);

			// disable VBO mode
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		} break;

		default: {
			assert(false);
		} break;
	}
}


void Patch::DrawBorder()
{
	CVertexArray* va = GetVertexArray();
	GenerateBorderIndices(va);
	va->DrawArrayC(GL_TRIANGLES);
}


void Patch::RecursBorderRender(
	CVertexArray* va,
	const TriTreeNode* tri,
	const int2 left,
	const int2 rght,
	const int2 apex,
	int depth,
	bool leftChild
) {
	if (tri->IsLeaf()) {
		const float3& v1 = *(float3*)&vertices[(apex.x + apex.y * (PATCH_SIZE + 1))*3];
		const float3& v2 = *(float3*)&vertices[(left.x + left.y * (PATCH_SIZE + 1))*3];
		const float3& v3 = *(float3*)&vertices[(rght.x + rght.y * (PATCH_SIZE + 1))*3];

		static const unsigned char white[] = {255,255,255,255};
		static const unsigned char trans[] = {255,255,255,0};

		va->EnlargeArrays(6, 0, VA_SIZE_C);

		if ((depth & 1) == 0) {
			va->AddVertexQC(v2,                          white);
			va->AddVertexQC(float3(v2.x, -400.0f, v2.z), trans);
			va->AddVertexQC(float3(v3.x, v3.y, v3.z),    white);

			va->AddVertexQC(v3,                          white);
			va->AddVertexQC(float3(v2.x, -400.0f, v2.z), trans);
			va->AddVertexQC(float3(v3.x, -400.0f, v3.z), trans);
		} else {
			if (leftChild) {
				va->AddVertexQC(v1,                          white);
				va->AddVertexQC(float3(v1.x, -400.0f, v1.z), trans);
				va->AddVertexQC(float3(v2.x, v2.y, v2.z),    white);

				va->AddVertexQC(v2,                          white);
				va->AddVertexQC(float3(v1.x, -400.0f, v1.z), trans);
				va->AddVertexQC(float3(v2.x, -400.0f, v2.z), trans);
			} else {
				va->AddVertexQC(v3,                          white);
				va->AddVertexQC(float3(v3.x, -400.0f, v3.z), trans);
				va->AddVertexQC(float3(v1.x, v1.y, v1.z),    white);

				va->AddVertexQC(v1,                          white);
				va->AddVertexQC(float3(v3.x, -400.0f, v3.z), trans);
				va->AddVertexQC(float3(v1.x, -400.0f, v1.z), trans);
			}
		}

		return;
	}

	const int2 center = {(left.x + rght.x) >> 1, (left.y + rght.y) >> 1};

	if ((depth & 1) == 0) {
		       RecursBorderRender(va, tri->LeftChild,  apex, left, center, depth + 1, !leftChild);
		return RecursBorderRender(va, tri->RightChild, rght, apex, center, depth + 1,  leftChild); // return is needed for tail call optimization (it's still unlikely gcc does so...)
	}

	if (leftChild) {
		return RecursBorderRender(va, tri->LeftChild,  apex, left, center, depth + 1,  leftChild);
	} else {
		return RecursBorderRender(va, tri->RightChild, rght, apex, center, depth + 1, !leftChild);
	}
}

void Patch::GenerateBorderIndices(CVertexArray* va)
{
	va->Initialize();

	const bool isLeftBorder   = (baseLeft.LeftNeighbor   == nullptr);
	const bool isBottomBorder = (baseRight.RightNeighbor == nullptr);
	const bool isRightBorder  = (baseLeft.RightNeighbor  == nullptr);
	const bool isTopBorder    = (baseRight.LeftNeighbor  == nullptr);

	if (isLeftBorder)   RecursBorderRender(va, &baseLeft,  int2(0, PATCH_SIZE), int2(PATCH_SIZE, 0), int2(0, 0),                   1, true);
	if (isBottomBorder) RecursBorderRender(va, &baseRight, int2(PATCH_SIZE, 0), int2(0, PATCH_SIZE), int2(PATCH_SIZE, PATCH_SIZE), 1, false);
	if (isRightBorder)  RecursBorderRender(va, &baseLeft,  int2(0, PATCH_SIZE), int2(PATCH_SIZE, 0), int2(0, 0),                   1, false);
	if (isTopBorder)    RecursBorderRender(va, &baseRight, int2(PATCH_SIZE, 0), int2(0, PATCH_SIZE), int2(PATCH_SIZE, PATCH_SIZE), 1, true);
}


void Patch::Upload()
{
	switch (renderMode) {
		case DL: {
			glNewList(triList, GL_COMPILE);
				glEnableClientState(GL_VERTEX_ARRAY);
					glVertexPointer(3, GL_FLOAT, 0, &vertices[0]);
					glDrawRangeElements(GL_TRIANGLES, 0, vertices.size(), indices.size(), GL_UNSIGNED_INT, &indices[0]);
				glDisableClientState(GL_VERTEX_ARRAY);
			glEndList();
		} break;

		case VBO: {
			if (!vboVerticesUploaded) VBOUploadVertices();
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, vertexIndexBuffer);
			glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indices.size() * sizeof(unsigned), &indices[0], GL_DYNAMIC_DRAW_ARB);

			/*
			int bufferSize = 0;
			glGetBufferParameterivARB(GL_ELEMENT_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &bufferSize);
			if(rend != bufferSize) {
				glDeleteBuffersARB(1, &vertexIndexBuffer);
				LOG( "[createVBO()] Data size is mismatch with input array\n" );
			}
			*/

			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		} break;

		default: {
		} break;
	}
}

void Patch::SetSquareTexture() const
{
	smfGroundDrawer->SetupBigSquare(coors.x / PATCH_SIZE, coors.y / PATCH_SIZE);
}


void Patch::SwitchRenderMode(int mode)
{
	if (mode < 0) {
		mode = renderMode + 1;
		mode %= 3;
	}

	if (!GLEW_ARB_vertex_buffer_object && mode == VBO) {
		mode = DL;
	}

	if (mode == renderMode)
		return;

	switch (mode) {
		case VA: {
			LOG("Set ROAM mode to VA");
			renderMode = VA;
		} break;
		case DL: {
			LOG("Set ROAM mode to DisplayLists");
			renderMode = DL;
		} break;
		case VBO: {
			LOG("Set ROAM mode to VBO");
			renderMode = VBO;
		} break;
	}

	CRoamMeshDrawer::ForceTesselation();
}



// ---------------------------------------------------------------------
// Visibility Update Functions
//

#if 0
void Patch::UpdateVisibility(CCamera* cam)
{
	const float3 mins( coors.x               * SQUARE_SIZE, readMap->GetCurrMinHeight(),  coors.y               * SQUARE_SIZE);
	const float3 maxs((coors.x + PATCH_SIZE) * SQUARE_SIZE, readMap->GetCurrMaxHeight(), (coors.y + PATCH_SIZE) * SQUARE_SIZE);

	if (!cam->InView(mins, maxs))
		return;

	lastDrawFrames[cam->GetCamType()] = globalRendering->drawFrame;
}
#endif


class CPatchInViewChecker : public CReadMap::IQuadDrawer
{
public:
	void ResetState() {}
	void ResetState(CCamera* c = nullptr, Patch* p = nullptr, int xsize = 0) {
		testCamera = c;
		patchArray = p;
		numPatchesX = xsize;
	}

	void DrawQuad(int x, int y) {
		patchArray[y * numPatchesX + x].lastDrawFrames[testCamera->GetCamType()] = globalRendering->drawFrame;
	}

private:
	CCamera* testCamera;
	Patch* patchArray;

	int numPatchesX;
};


void Patch::UpdateVisibility(CCamera* cam, std::vector<Patch>& patches, const int numPatchesX)
{
	#if 0
	// very slow
	for (Patch& p: patches) {
		p.UpdateVisibility(cam);
	}
	#else
	// very fast
	static CPatchInViewChecker checker;

	assert(cam->GetCamType() < CCamera::CAMTYPE_VISCUL);
	checker.ResetState(cam, &patches[0], numPatchesX);

	cam->GetFrustumSides(readMap->GetCurrMinHeight() - 100.0f, readMap->GetCurrMaxHeight() + 100.0f, SQUARE_SIZE);
	readMap->GridVisibility(cam, &checker, 1e9, PATCH_SIZE);
	#endif
}

bool Patch::IsVisible(const CCamera* cam) const {
	return (lastDrawFrames[cam->GetCamType()] >= globalRendering->drawFrame);
}

