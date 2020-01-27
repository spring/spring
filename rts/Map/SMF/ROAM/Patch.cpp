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
#include "Map/ReadMap.h"
#include "Map/SMF/SMFGroundDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "System/Log/ILog.h"
#include "System/Threading/ThreadPool.h"

#include <climits>

static size_t CUR_POOL_SIZE =                 0; // split over all threads
static size_t MAX_POOL_SIZE = NEW_POOL_SIZE * 8; // upper limit for ResetAll


static CTriNodePool pools[CRoamMeshDrawer::MESH_COUNT][ThreadPool::MAX_THREADS];

TriTreeNode TriTreeNode::dummyNode;


void CTriNodePool::InitPools(bool shadowPass, size_t newPoolSize)
{
	for (int j = 0, numThreads = ThreadPool::GetMaxThreads(); newPoolSize > 0; j++) {
		try {
			const size_t thrPoolSize = std::max((CUR_POOL_SIZE = newPoolSize) / numThreads, newPoolSize / 3);

			for (int i = 0; i < numThreads; i++) {
				pools[shadowPass][i].Reset();
				pools[shadowPass][i].Resize(thrPoolSize + (thrPoolSize & 1));
			}

			LOG_L(L_INFO, "[TriNodePool::%s] newPoolSize=" _STPF_ " thrPoolSize=" _STPF_ " (numThreads=%d shadowPass=%d)", __func__, newPoolSize, thrPoolSize, numThreads, shadowPass);
			break;
		} catch (const std::bad_alloc& e) {
			LOG_L(L_FATAL, "[TriNodePool::%s] exception \"%s\" (numThreads=%d shadowPass=%d)", __func__, e.what(), numThreads, shadowPass);

			// try again after reducing the wanted pool-size by a quarter
			newPoolSize = (MAX_POOL_SIZE = newPoolSize - (newPoolSize >> 2));
		}
	}
}

void CTriNodePool::ResetAll(bool shadowPass)
{
	bool outOfNodes = false;

	for (int i = 0, n = ThreadPool::GetMaxThreads(); i < n; i++) {
		outOfNodes |= pools[shadowPass][i].OutOfNodes();
		pools[shadowPass][i].Reset();
	}

	if (!outOfNodes)
		return;
	if (CUR_POOL_SIZE >= MAX_POOL_SIZE)
		return;

	InitPools(shadowPass, std::min(CUR_POOL_SIZE * 2, MAX_POOL_SIZE));
}


CTriNodePool* CTriNodePool::GetPoolForThread(bool shadowPass)
{
	return &(pools[shadowPass][ThreadPool::GetThreadNum()]);
}


void CTriNodePool::Resize(size_t poolSize)
{
	// child nodes are always allocated in pairs, so poolSize must be even
	// (it does not technically need to be non-zero since patch root nodes
	// live outside the pool, but KISS)
	assert((poolSize & 1) == 0);
	assert(poolSize > 0);

	tris.resize(poolSize);
}

bool CTriNodePool::Allocate(TriTreeNode*& left, TriTreeNode*& right)
{
	// pool exhausted, make sure both child nodes are dummies
	if (OutOfNodes()) {
		LOG_L(L_WARNING, "[TriNodePool::%s] #nodes=" _STPF_ " #pool=" _STPF_ " thread=%d", __func__, nextTriNodeIdx, tris.size(), ThreadPool::GetThreadNum());

		left  = &TriTreeNode::dummyNode;
		right = &TriTreeNode::dummyNode;
		return false;
	}

	left  = &tris[nextTriNodeIdx++];
	right = &tris[nextTriNodeIdx++];

	left->Reset();
	right->Reset();
	return true;
}




Patch::Patch()
{
	varianceTrees[0].fill(0.0f);
	varianceTrees[1].fill(0.0f);
	vertices.fill(0.0f);
}

Patch::~Patch()
{
	glDeleteVertexArrays(1, &vertexArrays[0]);
	glDeleteVertexArrays(1, &vertexArrays[1]);

	glDeleteBuffers(1, &vertexBuffers[0]);
	glDeleteBuffers(1, &vertexBuffers[1]);
	glDeleteBuffers(1, &indexBuffer);

	memset(vertexArrays, 0, sizeof(vertexArrays));
	memset(vertexBuffers, 0, sizeof(vertexBuffers));

	indexBuffer = 0;
}

void Patch::Init(CSMFGroundDrawer* _drawer, int patchX, int patchZ)
{
	coors.x = patchX;
	coors.y = patchZ;

	smfGroundDrawer = _drawer;


	// create used OpenGL objects
	glGenVertexArrays(1, &vertexArrays[0]);
	glGenVertexArrays(1, &vertexArrays[1]);

	glGenBuffers(1, &vertexBuffers[0]);
	glGenBuffers(1, &vertexBuffers[1]);
	glGenBuffers(1, &indexBuffer);


	unsigned int index = 0;

	// initialize vertices
	for (int z = coors.y; z <= (coors.y + PATCH_SIZE); z++) {
		for (int x = coors.x; x <= (coors.x + PATCH_SIZE); x++) {
			vertices[index++] = x * SQUARE_SIZE;
			vertices[index++] = 0.0f;
			vertices[index++] = z * SQUARE_SIZE;
		}
	}

	Reset();
	UpdateHeightMap();
}

void Patch::Reset()
{
	// reset the important relationships
	baseLeft  = {};
	baseRight = {};

	// attach the two base-triangles together
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

	UploadVertices();
	// UploadBorderVertices();

	isDirty = true;
}


bool Patch::Split(TriTreeNode* tri)
{
	if (tri->IsDummy())
		return false;

	// we are already split, no need to do it again
	if (!tri->IsLeaf())
		return true;

	// if this triangle is not in a proper diamond, force split our base-neighbor
	if (!tri->BaseNeighbor->IsDummy() && (tri->BaseNeighbor->BaseNeighbor != tri))
		Split(tri->BaseNeighbor);

	// create children and link into mesh, or make this triangle a leaf
	if (!curTriPool->Allocate(tri->LeftChild, tri->RightChild))
		return false;

	assert(tri->IsBranch());

	TriTreeNode* tlc = tri->LeftChild;
	TriTreeNode* trc = tri->RightChild;
	TriTreeNode* tln = tri->LeftNeighbor;
	TriTreeNode* trn = tri->RightNeighbor;
	TriTreeNode* tbn = tri->BaseNeighbor;

	assert(!tlc->IsDummy());
	assert(!trc->IsDummy());

	{
		// fill in the information we can get from the parent (neighbor pointers)
		tlc->BaseNeighbor = tln;
		tlc->LeftNeighbor = trc;
	}
	{
		trc-> BaseNeighbor = trn;
		trc->RightNeighbor = tlc;
	}

	// link our left-neighbor to the new children
	if (!tln->IsDummy()) {
		if (tln->BaseNeighbor == tri)
			tln->BaseNeighbor = tlc;
		else if (tln->LeftNeighbor == tri)
			tln->LeftNeighbor = tlc;
		else if (tln->RightNeighbor == tri)
			tln->RightNeighbor = tlc;
		else
			;// illegal Left neighbor
	}

	// link our right-neighbor to the new children
	if (!trn->IsDummy()) {
		if (trn->BaseNeighbor == tri)
			trn->BaseNeighbor = trc;
		else if (trn->RightNeighbor == tri)
			trn->RightNeighbor = trc;
		else if (trn->LeftNeighbor == tri)
			trn->LeftNeighbor = trc;
		else
			;// illegal Right neighbor
	}

	// link our base-neighbor to the new children
	if (!tbn->IsDummy()) {
		if (tbn->IsBranch()) {
			assert(curTriPool->ValidNode(tbn->LeftChild));
			assert(curTriPool->ValidNode(tbn->RightChild));

			if (!(tlc->RightNeighbor = tbn->RightChild)->IsDummy())
				tbn->RightChild->LeftNeighbor = tlc;

			if (!(trc->LeftNeighbor = tbn->LeftChild)->IsDummy())
				tbn->LeftChild->RightNeighbor = trc;
		} else {
			// base Neighbor (in a diamond with us) was not split yet, do so now
			// FIXME: if pool ran out above, this will fail and leave a LOD-crack
			Split(tbn);
		}
	} else {
		// edge triangle, trivial case
		tlc->RightNeighbor = &TriTreeNode::dummyNode;
		trc-> LeftNeighbor = &TriTreeNode::dummyNode;
	}

	return true;
}



void Patch::RecursTessellate(TriTreeNode* tri, const int2 left, const int2 right, const int2 apex, const int varTreeIdx, const int curNodeIdx)
{
	// bail if we can not tessellate further in at least one dimension
	if ((abs(left.x - right.x) <= 1) && (abs(left.y - right.y) <= 1))
		return;
	if (tri->IsDummy())
		return;

	// default > 1; when variance isn't saved this issues further tessellation
	float triVariance = 10.0f;

	if (curNodeIdx < (1 << VARIANCE_DEPTH)) {
		// make maximum tessellation-level dependent on camDistLODFactor
		// huge cliffs cause huge variances and would otherwise always tessellate
		// regardless of the actual camera distance (-> huge/distfromcam ~= huge)
		const int sizeX = std::max(left.x - right.x, right.x - left.x);
		const int sizeY = std::max(left.y - right.y, right.y - left.y);
		const int size  = std::max(sizeX, sizeY);

		// take distance, variance and patch size into consideration
		triVariance = (std::min(varianceTrees[varTreeIdx][curNodeIdx], varianceMaxLimit) * PATCH_SIZE * size) * camDistLODFactor;
	}

	// stop tesselation
	if (triVariance <= 1.0f)
		return;

	if ((Split(tri), !tri->IsBranch()))
		return;

	// triangle was split, also try to split its children
	const int2 center = {(left.x + right.x) >> 1, (left.y + right.y) >> 1};

	RecursTessellate(tri-> LeftChild,  apex, left, center, varTreeIdx, (curNodeIdx << 1)    );
	RecursTessellate(tri->RightChild, right, apex, center, varTreeIdx, (curNodeIdx << 1) + 1);
}


void Patch::RecursGenIndices(const TriTreeNode* tri, const int2 left, const int2 right, const int2 apex)
{
	if (tri->IsDummy())
		return;
	if (tri->IsLeaf()) {
		indices.push_back( apex.x +  apex.y * (PATCH_SIZE + 1));
		indices.push_back( left.x +  left.y * (PATCH_SIZE + 1));
		indices.push_back(right.x + right.y * (PATCH_SIZE + 1));
		return;
	}

	const int2 center = {(left.x + right.x) >> 1, (left.y + right.y) >> 1};

	RecursGenIndices(tri-> LeftChild,  apex, left, center);
	RecursGenIndices(tri->RightChild, right, apex, center);
}

void Patch::GenerateIndices()
{
	indices.clear();
	indices.reserve(vertices.size() * 3);

	RecursGenIndices(&baseLeft , int2(         0, PATCH_SIZE), int2(PATCH_SIZE,          0), int2(         0,          0));
	RecursGenIndices(&baseRight, int2(PATCH_SIZE,          0), int2(         0, PATCH_SIZE), int2(PATCH_SIZE, PATCH_SIZE));
}



void Patch::GenerateBorderVertices()
{
	if (!isTesselated)
		return;

	isTesselated = false;

	borderVertices.clear();
	borderVertices.reserve((PATCH_SIZE + 1) * 2);

	#define PS PATCH_SIZE
	// border vertices are always part of base-level triangles
	// that have either no left or no right neighbor, i.e. are
	// on the map edge
	if (baseLeft . LeftNeighbor->IsDummy()) RecursGenBorderVertices(&baseLeft , { 0, PS}, {PS,  0}, { 0,  0}, {1,  true}); // left border
	if (baseLeft .RightNeighbor->IsDummy()) RecursGenBorderVertices(&baseLeft , { 0, PS}, {PS,  0}, { 0,  0}, {1, false}); // right border
	if (baseRight.RightNeighbor->IsDummy()) RecursGenBorderVertices(&baseRight, {PS,  0}, { 0, PS}, {PS, PS}, {1, false}); // bottom border
	if (baseRight. LeftNeighbor->IsDummy()) RecursGenBorderVertices(&baseRight, {PS,  0}, { 0, PS}, {PS, PS}, {1,  true}); // top border
	#undef PS
}

void Patch::RecursGenBorderVertices(
	const TriTreeNode* tri,
	const int2 left,
	const int2 rght,
	const int2 apex,
	const int2 depth
) {
	if (tri->IsDummy())
		return;

	if (tri->IsLeaf()) {
		const float3& v1 = *(float3*) &vertices[(apex.x + apex.y * (PATCH_SIZE + 1)) * 3];
		const float3& v2 = *(float3*) &vertices[(left.x + left.y * (PATCH_SIZE + 1)) * 3];
		const float3& v3 = *(float3*) &vertices[(rght.x + rght.y * (PATCH_SIZE + 1)) * 3];

		static constexpr unsigned char white[] = {255, 255, 255, 255};
		static constexpr unsigned char trans[] = {255, 255, 255,   0};

		if ((depth.x & 1) == 0) {
			borderVertices.push_back(VA_TYPE_C{ v2,                   {white}});
			borderVertices.push_back(VA_TYPE_C{{v2.x, -400.0f, v2.z}, {trans}});
			borderVertices.push_back(VA_TYPE_C{{v3.x,    v3.y, v3.z}, {white}});

			borderVertices.push_back(VA_TYPE_C{{v2.x, -400.0f, v2.z}, {trans}});
			borderVertices.push_back(VA_TYPE_C{{v3.x, -400.0f, v3.z}, {trans}});
			borderVertices.push_back(VA_TYPE_C{ v3                  , {white}});
			return;
		}

		if (depth.y) {
			// left child
			borderVertices.push_back(VA_TYPE_C{ v1                  , {white}});
			borderVertices.push_back(VA_TYPE_C{{v1.x, -400.0f, v1.z}, {trans}});
			borderVertices.push_back(VA_TYPE_C{{v2.x,    v2.y, v2.z}, {white}});

			borderVertices.push_back(VA_TYPE_C{{v1.x, -400.0f, v1.z}, {trans}});
			borderVertices.push_back(VA_TYPE_C{{v2.x, -400.0f, v2.z}, {trans}});
			borderVertices.push_back(VA_TYPE_C{ v2                  , {white}});
		} else {
			// right child
			borderVertices.push_back(VA_TYPE_C{ v3                  , {white}});
			borderVertices.push_back(VA_TYPE_C{{v3.x, -400.0f, v3.z}, {trans}});
			borderVertices.push_back(VA_TYPE_C{{v1.x,    v1.y, v1.z}, {white}});

			borderVertices.push_back(VA_TYPE_C{{v3.x, -400.0f, v3.z}, {trans}});
			borderVertices.push_back(VA_TYPE_C{{v1.x, -400.0f, v1.z}, {trans}});
			borderVertices.push_back(VA_TYPE_C{ v1                  , {white}});
		}

		return;
	}

	const int2 center = {(left.x + rght.x) >> 1, (left.y + rght.y) >> 1};

	// at even depths, descend down left *and* right children since both
	// are on the patch-edge; returns are needed for gcc's TCO (although
	// unlikely to be applied)
	if ((depth.x & 1) == 0) {
		       RecursGenBorderVertices(tri-> LeftChild, apex, left, center, {depth.x + 1, !depth.y});
		return RecursGenBorderVertices(tri->RightChild, rght, apex, center, {depth.x + 1,  depth.y});
	}

	// at odd depths (where only one triangle is on the edge), always force
	// a left-bias for the next call so the recursion ends up at the correct
	// leafs
	if (depth.y) {
		return RecursGenBorderVertices(tri-> LeftChild, apex, left, center, {depth.x + 1, true});
	} else {
		return RecursGenBorderVertices(tri->RightChild, rght, apex, center, {depth.x + 1, true});
	}
}



float Patch::GetHeight(int2 pos)
{
	const int vIdx = (pos.y * (PATCH_SIZE + 1) + pos.x) * 3 + 1;
	// const int hIdx = (coors.y + pos.y) * mapDims.mapxp1 + (coors.x + pos.x);

	// assert(readMap->GetCornerHeightMapUnsynced()[hIdx] == vertices[vIdx]);
	return vertices[vIdx];
}

float Patch::RecursComputeVariance(
	const   int2 left,
	const   int2 rght,
	const   int2 apex,
	const float3 hgts,
	const    int varTreeIdx,
	const    int curNodeIdx
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
	float mvar = math::fabs(mhgt - ((hgts.x + hgts.y) * 0.5f));

	// shore lines get more variance for higher accuracy
	// NOTE: .x := height(L), .y := height(R), .z := height(A)
	if ((hgts.x * hgts.y) < 0.0f || (hgts.x * mhgt) < 0.0f || (hgts.y * mhgt) < 0.0f)
		mvar = std::max(mvar * 1.5f, 20.0f);

	#if 0
	mvar = MAX(abs(left.x - rght.x), abs(left.y - rght.y)) * mvar;
	#endif

	// save some CPU, only calculate variance down to a 4x4 block
	if ((abs(left.x - rght.x) >= 4) || (abs(left.y - rght.y) >= 4)) {
		const float3 hgts1 = {hgts.z, hgts.x, mhgt};
		const float3 hgts2 = {hgts.y, hgts.z, mhgt};

		// final variance for this node is the max of its own variance and that of its children
		mvar = std::max(mvar, RecursComputeVariance(apex, left, mpos, hgts1, varTreeIdx, (curNodeIdx << 1)    ));
		mvar = std::max(mvar, RecursComputeVariance(rght, apex, mpos, hgts2, varTreeIdx, (curNodeIdx << 1) + 1));
	}

	// NOTE: Variance is never zero
	mvar = std::max(0.001f, mvar);

	// store the final variance for this node
	if (curNodeIdx < (1 << VARIANCE_DEPTH))
		varianceTrees[varTreeIdx][curNodeIdx] = mvar;

	return mvar;
}


void Patch::ComputeVariance()
{
	{
		const   int2 left = {         0, PATCH_SIZE};
		const   int2 rght = {PATCH_SIZE,          0};
		const   int2 apex = {         0,          0};
		const float3 hgts = {
			GetHeight(left),
			GetHeight(rght),
			GetHeight(apex),
		};

		RecursComputeVariance(left, rght, apex, hgts, 0, 1);
	}

	{
		const   int2 left = {PATCH_SIZE,          0};
		const   int2 rght = {         0, PATCH_SIZE};
		const   int2 apex = {PATCH_SIZE, PATCH_SIZE};
		const float3 hgts = {
			GetHeight(left),
			GetHeight(rght),
			GetHeight(apex),
		};

		RecursComputeVariance(left, rght, apex, hgts, 1, 1);
	}

	// Clear the dirty flag for this patch
	isDirty = false;
}


bool Patch::Tessellate(const float3& camPos, int viewRadius, bool shadowPass)
{
	isTesselated = true;

	// Set/Update LOD params (FIXME: wrong height?)
	float3 midPos;
	midPos.x = (coors.x + PATCH_SIZE / 2) * SQUARE_SIZE;
	midPos.z = (coors.y + PATCH_SIZE / 2) * SQUARE_SIZE;
	midPos.y = readMap->GetCurrAvgHeight();

	// Tessellate is called from multiple threads during both passes
	// caller ensures that two patches that are neighbors or share a
	// neighbor are never touched concurrently (crucial for ::Split)
	curTriPool = CTriNodePool::GetPoolForThread(shadowPass);

	// MAGIC NUMBER 1: scale factor to reduce LOD with camera distance
	camDistLODFactor  = midPos.distance(camPos);
	camDistLODFactor *= (300.0f / viewRadius);
	camDistLODFactor  = std::max(1.0f, camDistLODFactor);
	camDistLODFactor  = 1.0f / camDistLODFactor;

	// MAGIC NUMBER 2:
	//   regulates how deeply areas are tessellated by clamping variances to it
	//   (the maximum tessellation is still untouched, this reduces the maximum
	//   far-distance LOD while the param above defines an overall falloff-rate)
	varianceMaxLimit = viewRadius * 0.35f;

	{
		// split each of the base triangles
		const int2 left = {coors.x,              coors.y + PATCH_SIZE};
		const int2 rght = {coors.x + PATCH_SIZE, coors.y             };
		const int2 apex = {coors.x,              coors.y             };

		RecursTessellate(&baseLeft, left, rght, apex, 0, 1);
	}
	{
		const int2 left = {coors.x + PATCH_SIZE, coors.y             };
		const int2 rght = {coors.x,              coors.y + PATCH_SIZE};
		const int2 apex = {coors.x + PATCH_SIZE, coors.y + PATCH_SIZE};

		RecursTessellate(&baseRight, left, rght, apex, 1, 1);
	}

	return (!curTriPool->OutOfNodes());
}



void Patch::Draw()
{
	glBindVertexArray(vertexArrays[0]);
	glDrawRangeElements(GL_TRIANGLES, 0, vertices.size(), indices.size(), GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
}

void Patch::DrawBorder()
{
	glBindVertexArray(vertexArrays[1]);
	glDrawArrays(GL_TRIANGLES, 0, borderVertices.size());
	glBindVertexArray(0);
}



void Patch::UploadVertices()
{
	glBindVertexArray(vertexArrays[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[0]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(float3), VA_TYPE_OFFSET(float, 0));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(0);
}

void Patch::UploadBorderVertices()
{
	glBindVertexArray(vertexArrays[1]);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[1]);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VA_TYPE_C), VA_TYPE_OFFSET(float, 0));
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, false, sizeof(VA_TYPE_C), VA_TYPE_OFFSET(float, 3));

	glBufferData(GL_ARRAY_BUFFER, borderVertices.size() * sizeof(VA_TYPE_C), borderVertices.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);
}


void Patch::UploadIndices()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), &indices[0], GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}



void Patch::SetSquareTexture() const
{
	smfGroundDrawer->SetupBigSquare(coors.x / PATCH_SIZE, coors.y / PATCH_SIZE);
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
	void ResetState() final {}
	void ResetState(CCamera* c, Patch* p, int npx, int npy) {
		testCamera = c;
		patchArray = p;
		numPatchesX = npx;
		numPatchesY = npy;
	}

	void DrawQuad(int x, int y) final {
		assert(x >= 0 && x < numPatchesX);
		assert(y >= 0 && y < numPatchesY);

		patchArray[y * numPatchesX + x].lastDrawFrames[testCamera->GetCamType()] = globalRendering->drawFrame;
	}

private:
	CCamera* testCamera;
	Patch* patchArray;

	int numPatchesX;
	int numPatchesY;
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
	checker.ResetState(cam, &patches[0], numPatchesX, patches.size() / numPatchesX);

	cam->CalcFrustumLines(readMap->GetCurrMinHeight() - 100.0f, readMap->GetCurrMaxHeight() + 100.0f, SQUARE_SIZE);
	readMap->GridVisibility(cam, &checker, 1e9, PATCH_SIZE);
	#endif
}

bool Patch::IsVisible(const CCamera* cam) const {
	return (lastDrawFrames[cam->GetCamType()] >= globalRendering->drawFrame);
}

