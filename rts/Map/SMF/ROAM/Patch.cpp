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
#include "Sim/Misc/GlobalConstants.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"
#include "System/OpenMP_cond.h"
#include <cfloat>

// -------------------------------------------------------------------------------------------------
//	PATCH CLASS

RenderMode Patch::renderMode = VBO;


// -------------------------------------------------------------------------------------------------
// C'tor etc.
//
Patch::Patch()
	: smfGroundDrawer(NULL)
	, m_HeightMap(NULL)
	, heightData(NULL)
	, m_CurrentVariance(NULL)
	, m_isVisible(false)
	, m_isDirty(true)
	, varianceMaxLimit(FLT_MAX)
	, camDistLODFactor(1.f)
	, m_WorldX(-1)
	, m_WorldY(-1)
	//, minHeight(FLT_MAX)
	//, maxHeight(FLT_MIN)
	, triList(0)
	, vertexBuffer(0)
	, vertexIndexBuffer(0)
{
	m_VarianceLeft.resize(1 << VARIANCE_DEPTH);
	m_VarianceRight.resize(1 << VARIANCE_DEPTH);
}


void Patch::Init(CSMFGroundDrawer* _drawer, int worldX, int worldZ)
{
	smfGroundDrawer = _drawer;
	heightData = readmap->GetCornerHeightMapUnsynced();;
	m_WorldX = worldX;
	m_WorldY = worldZ;

	// Attach the two m_Base triangles together
	m_BaseLeft.BaseNeighbor  = &m_BaseRight;
	m_BaseRight.BaseNeighbor = &m_BaseLeft;

	// Store pointer to first byte of the height data for this patch.
	m_HeightMap = &heightData[worldZ * gs->mapxp1 + worldX];

	// Create used OpenGL objects
	triList = glGenLists(1);
	glGenBuffersARB(1, &vertexBuffer);
	glGenBuffersARB(1, &vertexIndexBuffer);

	UpdateHeightMap();
}


Patch::~Patch()
{
	glDeleteLists(triList, 1);
	glDeleteBuffersARB(1, &vertexBuffer);
	glDeleteBuffersARB(1, &vertexIndexBuffer);
}


void Patch::Reset()
{
	// Reset the important relationships
	m_BaseLeft  = TriTreeNode();
	m_BaseRight = TriTreeNode();

	// Attach the two m_Base triangles together
	m_BaseLeft.BaseNeighbor  = &m_BaseRight;
	m_BaseRight.BaseNeighbor = &m_BaseLeft;
}


void Patch::UpdateHeightMap(const SRectangle& rect)
{
	if (vertices.empty()) {
		// Initialize
		vertices.resize(3 * (PATCH_SIZE + 1) * (PATCH_SIZE + 1));
		int index = 0;
		for (int z = m_WorldY; z <= (m_WorldY + PATCH_SIZE); z++) {
			for (int x = m_WorldX; x <= (m_WorldX + PATCH_SIZE); x++) {
				vertices[index++] = x * SQUARE_SIZE;
				vertices[index++] = 0.0f;
				vertices[index++] = z * SQUARE_SIZE;
			}
		}
	}

	static const float* hMap = readmap->GetCornerHeightMapUnsynced();
	for (int z = rect.z1; z <= rect.z2; z++) {
		for (int x = rect.x1; x <= rect.x2; x++) {
			const float& h = hMap[(z + m_WorldY) * gs->mapxp1 + (x + m_WorldX)];
			const int vindex = (z * (PATCH_SIZE + 1) + x) * 3;
			vertices[vindex + 1] = h; // only update Y coord

			//if (h < minHeight) minHeight = h;
			//if (h > maxHeight) maxHeight = h;
		}
	}

	//FIXME don't do so in DispList mode!
	// Upload vertexBuffer
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vertexBuffer);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW_ARB);
	/*
	int bufferSize = 0;
	glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &bufferSize);
	if(index != bufferSize) {
		glDeleteBuffersARB(1, &vertexBuffer);
		glDeleteBuffersARB(1, &vertexIndexBuffer);
		LOG("[createVBO()] Data size is mismatch with input array\n");
	}
	*/
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

	m_isDirty = true;
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
	tri->LeftChild  = CRoamMeshDrawer::AllocateTri();
	tri->RightChild = CRoamMeshDrawer::AllocateTri();

	// If creation failed, just exit.
	if (!tri->IsBranch())
		return;

	// Fill in the information we can get from the parent (neighbor pointers)
	tri->LeftChild->BaseNeighbor = tri->LeftNeighbor;
	tri->LeftChild->LeftNeighbor = tri->RightChild;

	tri->RightChild->BaseNeighbor = tri->RightNeighbor;
	tri->RightChild->RightNeighbor = tri->LeftChild;

	// Link our Left Neighbor to the new children
	if (tri->LeftNeighbor != NULL) {
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
	if (tri->RightNeighbor != NULL) {
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
	if (tri->BaseNeighbor != NULL) {
		if (tri->BaseNeighbor->IsBranch()) {
			tri->BaseNeighbor->LeftChild->RightNeighbor = tri->RightChild;
			tri->BaseNeighbor->RightChild->LeftNeighbor = tri->LeftChild;
			tri->LeftChild->RightNeighbor = tri->BaseNeighbor->RightChild;
			tri->RightChild->LeftNeighbor = tri->BaseNeighbor->LeftChild;
		} else
			Split(tri->BaseNeighbor); // Base Neighbor (in a diamond with us) was not split yet, so do that now.
	} else {
		// An edge triangle, trivial case.
		tri->LeftChild->RightNeighbor = NULL;
		tri->RightChild->LeftNeighbor = NULL;
	}
}


// ---------------------------------------------------------------------
// Tessellate a Patch.
// Will continue to split until the variance metric is met.
//
void Patch::RecursTessellate(TriTreeNode* const& tri, const int2& left, const int2& right, const int2& apex, const int& node)
{
	const bool canFurtherTes = ((abs(left.x - right.x) > 1) || (abs(left.y - right.y) > 1));
	if (!canFurtherTes)
		return;

	float TriVariance;
	const bool varianceSaved = (node < (1 << VARIANCE_DEPTH));
	if (varianceSaved) {
		// make max tessellation viewRadius dependent
		// w/o this huge cliffs cause huge variances and so will always tessellate fully independent of camdist (-> huge/distfromcam ~= huge)
		const float myVariance = std::min(m_CurrentVariance[node], varianceMaxLimit);

		const int sizeX = std::max(left.x - right.x, right.x - left.x);
		const int sizeY = std::max(left.y - right.y, right.y - left.y);
		const int size  = std::max(sizeX, sizeY);

		// Take both distance and variance and patch size into consideration
		TriVariance = (myVariance * PATCH_SIZE * size) * camDistLODFactor;
	} else {
		TriVariance = 10.0f; // >1 -> When variance isn't saved issue further tessellation
	}

	if (TriVariance > 1.0f)
	{
		Split(tri); // Split this triangle.

		if (tri->IsBranch()) { // If this triangle was split, try to split it's children as well.
			const int2 center(
				(left.x + right.x) >> 1, // Compute X coordinate of center of Hypotenuse
				(left.y + right.y) >> 1  // Compute Y coord...
			);
			RecursTessellate(tri->LeftChild,  apex,  left, center, (node << 1)    );
			RecursTessellate(tri->RightChild, right, apex, center, (node << 1) + 1);
		}
	} else {
		// stop tess
	}
}

// ---------------------------------------------------------------------
// Render the tree.
//


void Patch::RecursRender(TriTreeNode* const& tri, const int2& left, const int2& right, const int2& apex, int maxdepth)
{
	if ( tri->IsLeaf() /*|| maxdepth>12*/ ) {
		indices.push_back(apex.x  + apex.y  * (PATCH_SIZE + 1));
		indices.push_back(left.x  + left.y  * (PATCH_SIZE + 1));
		indices.push_back(right.x + right.y * (PATCH_SIZE + 1));
	} else {
		const int2 center(
			(left.x + right.x) >> 1, // Compute X coordinate of center of Hypotenuse
			(left.y + right.y) >> 1  // Compute Y coord...
		);
		RecursRender(tri->LeftChild,  apex,  left, center, maxdepth + 1);
		RecursRender(tri->RightChild, right, apex, center, maxdepth + 1);
	}
}



// ---------------------------------------------------------------------
// Computes Variance over the entire tree.  Does not examine node relationships.
//
float Patch::RecursComputeVariance(const int& leftX, const int& leftY, const float& leftZ,
		const int& rightX, const int& rightY, const float& rightZ,
		const int& apexX, const int& apexY, const float& apexZ, const int& node)
{
	//        /|\
	//      /  |  \
	//    /    |    \
	//  /      |      \
	//  ~~~~~~~*~~~~~~~  <-- Compute the X and Y coordinates of '*'

	int centerX = (leftX + rightX) >> 1; // Compute X coordinate of center of Hypotenuse
	int centerY = (leftY + rightY) >> 1; // Compute Y coord...

	// Get the height value at the middle of the Hypotenuse
	float centerZ = m_HeightMap[(centerY * gs->mapxp1) + centerX];

	// Variance of this triangle is the actual height at it's hypotenuse midpoint minus the interpolated height.
	// Use values passed on the stack instead of re-accessing the Height Field.
	float myVariance = abs(centerZ - ((leftZ + rightZ) / 2));
	
	if (leftZ*rightZ<0 || leftZ*centerZ<0 || rightZ*centerZ<0)
		myVariance = std::max(myVariance * 1.5f, 20.0f); //shore lines get more variance for higher accuracy

	//myVariance= MAX(abs(leftX - rightX),abs(leftY - rightY)) * myVariance;

	// Since we're after speed and not perfect representations,
	//    only calculate variance down to a 4x4 block
	if ((abs(leftX - rightX) >= 4) || (abs(leftY - rightY) >= 4)) {
		// Final Variance for this node is the max of it's own variance and that of it's children.
		const float child1Variance = RecursComputeVariance(apexX, apexY, apexZ, leftX, leftY, leftZ, centerX, centerY, centerZ, node<<1);
		const float child2Variance = RecursComputeVariance(rightX, rightY, rightZ, apexX, apexY, apexZ, centerX, centerY, centerZ, 1+(node<<1));
		myVariance = std::max(myVariance, child1Variance);
		myVariance = std::max(myVariance, child2Variance);
	}

	// Note Variance is never zero.
	myVariance = std::max(0.001f, myVariance);

	// Store the final variance for this node.
	if (node < (1 << VARIANCE_DEPTH))
		m_CurrentVariance[node] = myVariance;

	return myVariance;
}


// ---------------------------------------------------------------------
// Compute the variance tree for each of the Binary Triangles in this patch.
//
void Patch::ComputeVariance()
{
	// Compute variance on each of the base triangles...
	m_CurrentVariance = &m_VarianceLeft[0];
	RecursComputeVariance(0, PATCH_SIZE, m_HeightMap[PATCH_SIZE * gs->mapxp1],
			PATCH_SIZE, 0, m_HeightMap[PATCH_SIZE], 0, 0, m_HeightMap[0], 1);

	m_CurrentVariance = &m_VarianceRight[0];
	RecursComputeVariance(PATCH_SIZE, 0, m_HeightMap[PATCH_SIZE], 0,
			PATCH_SIZE, m_HeightMap[PATCH_SIZE * gs->mapxp1], PATCH_SIZE, PATCH_SIZE,
			m_HeightMap[(PATCH_SIZE * gs->mapxp1) + PATCH_SIZE], 1);

	// Clear the dirty flag for this patch
	m_isDirty = false;
}


// ---------------------------------------------------------------------
// Create an approximate mesh.
//
void Patch::Tessellate(const float3& campos, int groundDetail)
{
	// Set/Update LOD params
	const float myx = (m_WorldX + PATCH_SIZE / 2) * SQUARE_SIZE;
	const float myy = (readmap->currMinHeight + readmap->currMaxHeight) * 0.5f;
	const float myz = (m_WorldY + PATCH_SIZE / 2) * SQUARE_SIZE;
	const float3 myPos(myx,myy,myz);

	camDistLODFactor  = (myPos - campos).Length();
	camDistLODFactor *= 300.0f / groundDetail; // MAGIC NUMBER 1: increase the dividend to reduce LOD in camera distance
	camDistLODFactor  = std::max(1.0f, camDistLODFactor);
	camDistLODFactor  = 1.0f / camDistLODFactor;

	// MAGIC NUMBER 2: variances are clamped by it, so it regulates how strong areas are tessellated.
	//   Note, the maximum tessellation is untouched by it. Instead it reduces the maximum LOD in
	//   distance, while the param above defines the overall FallOff rate.
	varianceMaxLimit = groundDetail * 0.35f;

	// Split each of the base triangles
	m_CurrentVariance = &m_VarianceLeft[0];
	RecursTessellate(&m_BaseLeft,
		int2(m_WorldX,              m_WorldY + PATCH_SIZE),
		int2(m_WorldX + PATCH_SIZE, m_WorldY),
		int2(m_WorldX,              m_WorldY),
		1);

	m_CurrentVariance = &m_VarianceRight[0];
	RecursTessellate(&m_BaseRight,
		int2(m_WorldX + PATCH_SIZE, m_WorldY),
		int2(m_WorldX,              m_WorldY + PATCH_SIZE),
		int2(m_WorldX + PATCH_SIZE, m_WorldY + PATCH_SIZE),
		1);
}

// ---------------------------------------------------------------------
// Render the mesh.
//

void Patch::DrawTriArray()
{
	switch (renderMode) {
		case VA:
			glEnableClientState(GL_VERTEX_ARRAY);             // activate vertex coords array

				glVertexPointer(3, GL_FLOAT, 0, &vertices[0]);
				glDrawRangeElements(GL_TRIANGLES, 0, vertices.size(), indices.size(), GL_UNSIGNED_INT, &indices[0]);

			glDisableClientState(GL_VERTEX_ARRAY);            // deactivate vertex array
			break;

		case DL:
			glCallList(triList);
			break;

		case VBO:
			// enable VBOs
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, vertexBuffer);              // for vertex coordinates
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, vertexIndexBuffer); // for indices

				glEnableClientState(GL_VERTEX_ARRAY);             // activate vertex coords array

					glVertexPointer(3, GL_FLOAT, 0, 0);       // last param is offset, not ptr
					glDrawRangeElements(GL_TRIANGLES, 0, vertices.size(), indices.size(), GL_UNSIGNED_INT, 0);

				glDisableClientState(GL_VERTEX_ARRAY);            // deactivate vertex array

			// disable VBO mode
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
			break;
	}
}


int Patch::Render()
{
	indices.clear();
	RecursRender(&m_BaseLeft,  int2(0, PATCH_SIZE), int2(PATCH_SIZE, 0), int2(0, 0),                   0);
	RecursRender(&m_BaseRight, int2(PATCH_SIZE, 0), int2(0, PATCH_SIZE), int2(PATCH_SIZE, PATCH_SIZE), 0);

	switch (renderMode) {
		case DL:
			glNewList(triList, GL_COMPILE);
				glEnableClientState(GL_VERTEX_ARRAY);
					glVertexPointer(3, GL_FLOAT, 0, &vertices[0]);
					glDrawRangeElements(GL_TRIANGLES, 0, vertices.size(), indices.size(), GL_UNSIGNED_INT, &indices[0]);
				glDisableClientState(GL_VERTEX_ARRAY);
			glEndList();
			break;

		case VBO:
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
			break;
		default:
			break;
	}
	
	return GetTriCount(); //return the number of tris rendered
}


void Patch::SetSquareTexture() const
{
	smfGroundDrawer->SetupBigSquare(m_WorldX / PATCH_SIZE, m_WorldY / PATCH_SIZE);
}


void Patch::SwitchRenderMode(int mode)
{
	if (mode < 0) {
		mode = renderMode + 1;
		mode %= 3;
	}

	if (mode == renderMode)
		return;

	switch (mode) {
		case VA:
			LOG("Set ROAM mode to VA");
			renderMode = VA;
			break;
		case DL:
			LOG("Set ROAM mode to DisplayLists");
			renderMode = DL;
			break;
		case VBO:
			LOG("Set ROAM mode to VBO");
			renderMode = VBO;
			break;
	}

	CRoamMeshDrawer::forceRetessellate = true;
}


// ---------------------------------------------------------------------
// Visibility Update Functions
//

/*void Patch::UpdateVisibility(CCamera*& cam)
{
	const float3 mins(
		m_WorldX * SQUARE_SIZE,
		readmap->currMinHeight,
		m_WorldY * SQUARE_SIZE
	);
	const float3 maxs(
		(m_WorldX + PATCH_SIZE) * SQUARE_SIZE,
		readmap->currMaxHeight,
		(m_WorldY + PATCH_SIZE) * SQUARE_SIZE
	);
	m_isVisible = cam->InView(mins, maxs);
}*/


class CPatchInViewChecker : public CReadMap::IQuadDrawer
{
public:
	std::vector<Patch>* patches;
	int numPatchesX;

	void DrawQuad(int x, int y) {
		(*patches)[x + y * numPatchesX].m_isVisible = true;
	}
};


void Patch::UpdateVisibility(CCamera*& cam, std::vector<Patch>& patches, const int& numPatchesX)
{
	// very slow
	//for (std::vector<Patch>::iterator it = m_Patches.begin(); it != m_Patches.end(); ++it) {
	//	it->UpdateVisibility(cam);
	//}

	// very fast
	static CPatchInViewChecker checker;
	checker.patches     = &patches;
	checker.numPatchesX = numPatchesX;

	for (std::vector<Patch>::iterator it = patches.begin(); it != patches.end(); ++it) {
		it->m_isVisible = false;
	}
	readmap->GridVisibility(cam, PATCH_SIZE, 1e9, &checker, INT_MAX);
}
