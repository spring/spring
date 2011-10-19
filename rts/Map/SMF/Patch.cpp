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
#include "Landscape.h"
#include "SMFGroundDrawer.h"
#include "Game/Camera.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Log/ILog.h"

#include <cfloat>

// -------------------------------------------------------------------------------------------------
//	PATCH CLASS

RenderMode Patch::renderMode = VBO;

// -------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------
// Split a single Triangle and link it into the mesh.
// Will correctly force-split diamonds.
//
void Patch::Split(TriTreeNode *tri)
{
	// We are already split, no need to do it again.
	if (tri->LeftChild)
		return;

	// If this triangle is not in a proper diamond, force split our base neighbor
	if (tri->BaseNeighbor && (tri->BaseNeighbor->BaseNeighbor != tri))
		Split(tri->BaseNeighbor);

	// Create children and link into mesh
	tri->LeftChild = Landscape::AllocateTri();
	tri->RightChild = Landscape::AllocateTri();

	// If creation failed, just exit.
	if (!tri->LeftChild)
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
		if (tri->BaseNeighbor->LeftChild) {
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
void Patch::RecursTessellate(TriTreeNode *tri, int leftX, int leftY,
		int rightX, int rightY, int apexX, int apexY, int node)
{
	float TriVariance=0;
	const int centerX = (leftX + rightX) >> 1; // Compute X coordinate of center of Hypotenuse
	const int centerY = (leftY + rightY) >> 1; // Compute Y coord...
	const int sizeX = std::max(leftX - rightX, rightX - leftX);
	const int sizeY = std::max(leftY - rightY, rightY - leftY);
	const int size  = std::max(sizeX, sizeY);

	if (node < (1 << VARIANCE_DEPTH)) { 
		TriVariance = ((float) m_CurrentVariance[node] * PATCH_SIZE )
				/ distfromcam *size ; // Take both distance and variance and patch size into consideration
	}

	if ((node >= (1 << VARIANCE_DEPTH)) || // IF we do not have variance info for this node, then we must have gotten here by splitting, so continue down to the lowest level.
			((TriVariance > 1))) // OR if we are not below the variance tree, test for variance.
	{
		Split(tri); // Split this triangle.

		if (tri->LeftChild && // If this triangle was split, try to split it's children as well.
				((abs(leftX - rightX) >= 2) || (abs(leftY - rightY) >= 2))) // Tessellate all the way down to one vertex per height field entry
		{
			RecursTessellate(tri->LeftChild, apexX, apexY, leftX, leftY,
					centerX, centerY, node << 1);
			RecursTessellate(tri->RightChild, rightX, rightY, apexX, apexY,
					centerX, centerY, 1 + (node << 1));
		}
	}else{
		//bool stoptess=true; //this is just for them debuggings
	}
}

// ---------------------------------------------------------------------
// Render the tree.
//


void Patch::RecursRender(TriTreeNode* tri, int leftX, int leftY, int rightX,
		int rightY, int apexX, int apexY, bool dir, int maxdepth, bool waterdrawn)
{
	int m_depth=maxdepth+1;
	
	if ( tri->LeftChild == NULL || maxdepth>12 ) { // All non-leaf nodes have both children, so just check for one
		indices.push_back(apexX  + apexY  * (PATCH_SIZE + 1));
		indices.push_back(leftX  + leftY  * (PATCH_SIZE + 1));
		indices.push_back(rightX + rightY * (PATCH_SIZE + 1));
	} else {
		int centerX = (leftX + rightX) >> 1; // Compute X coordinate of center of Hypotenuse
		int centerY = (leftY + rightY) >> 1; // Compute Y coord...
		RecursRender(tri->LeftChild, apexX, apexY, leftX, leftY, centerX, centerY, dir, m_depth, waterdrawn);
		RecursRender(tri->RightChild, rightX, rightY, apexX, apexY, centerX, centerY, dir, m_depth, waterdrawn);
	}
}



// ---------------------------------------------------------------------
// Computes Variance over the entire tree.  Does not examine node relationships.
//
float Patch::RecursComputeVariance(int leftX, int leftY,
		float leftZ, int rightX, int rightY, float rightZ,
		int apexX, int apexY, float apexZ, int node)
{
	//        /|\
	//      /  |  \
	//    /    |    \
	//  /      |      \
	//  ~~~~~~~*~~~~~~~  <-- Compute the X and Y coordinates of '*'

	int centerX = (leftX + rightX) >> 1; // Compute X coordinate of center of Hypotenuse
	int centerY = (leftY + rightY) >> 1; // Compute Y coord...
	float myVariance;

	// Get the height value at the middle of the Hypotenuse
	float centerZ = m_HeightMap[(centerY * (mapx+1)) + centerX];
	// Variance of this triangle is the actual height at it's hypotenuse midpoint minus the interpolated height.
	// Use values passed on the stack instead of re-accessing the Height Field.
	myVariance = abs(centerZ - ((leftZ + rightZ) / 2));
	
	if (leftZ*rightZ<0 || leftZ*centerZ<0 || rightZ*centerZ<0)
		myVariance = std::max(myVariance*2,20.0f); //shore lines get more variance for higher accuracy

	//myVariance= MAX(abs(leftX - rightX),abs(leftY - rightY))*myVariance;
	if (myVariance<0)
		myVariance = 100;

	// Since we're after speed and not perfect representations,
	//    only calculate variance down to a 4x4 block
	if ((abs(leftX - rightX) >= 4) || (abs(leftY - rightY) >= 4)) {
		// Final Variance for this node is the max of it's own variance and that of it's children.
		myVariance =
			std::max( myVariance, RecursComputeVariance( apexX, apexY, apexZ, leftX, leftY, leftZ, centerX, centerY, centerZ, node<<1 ) );
		myVariance =
			std::max( myVariance, RecursComputeVariance( rightX, rightY, rightZ, apexX, apexY, apexZ, centerX, centerY, centerZ, 1+(node<<1)) );
	}
	
	// Store the final variance for this node.  Note Variance is never zero.
	if (node < (1 << VARIANCE_DEPTH))
		m_CurrentVariance[node] = myVariance;//NOW IT IS 0 MUWAHAHAHAHA

	if (myVariance<0)
		myVariance=0.001;

	return myVariance;
}

// ---------------------------------------------------------------------
// Initialize a patch.
//
void Patch::Init(CSMFGroundDrawer* _drawer, int worldX, int worldZ, const float* hMap, int mx)
{
	drawer = _drawer;

	// Clear all the relationships
	mapx = mx;

	// Attach the two m_Base triangles together
	m_BaseLeft = TriTreeNode();
	m_BaseRight = TriTreeNode();
	m_BaseLeft.BaseNeighbor = &m_BaseRight;
	m_BaseRight.BaseNeighbor = &m_BaseLeft;

	// Store Patch offsets for the world and heightmap.
	m_WorldX = worldX;
	m_WorldY = worldZ;

	// Store pointer to first byte of the height data for this patch.
	m_HeightMap = &hMap[worldZ * (mapx+1) + worldX];
	heightData = hMap;

	// Initialize flags
	m_VarianceDirty = 1;
	m_isVisible = false;

	vertices.resize(3 * (PATCH_SIZE + 1) * (PATCH_SIZE + 1));

	triList = glGenLists(1);

	glGenBuffersARB(1, &vertexBuffer);
	glGenBuffersARB(1, &vertexIndexBuffer);

	UpdateHeightMap();
}


Patch::~Patch()
{
	glDeleteLists(triList, 1);
}


void Patch::UpdateHeightMap()
{
	const float* hMap = readmap->GetCornerHeightMapSynced(); //FIXME

	float minHeight = FLT_MAX;
	float maxHeight = FLT_MIN;

	int index = 0;
	for (int z = m_WorldY; z <= (m_WorldY + PATCH_SIZE); z++) {
		for (int x = m_WorldX; x <= (m_WorldX + PATCH_SIZE); x++) {
			const float& h = hMap[z * (mapx+1) + x];
			
			vertices[index++] = x * SQUARE_SIZE;
			vertices[index++] = h;
			vertices[index++] = z * SQUARE_SIZE;

			//FIXME use map global ones instead?
			maxHeight = std::max(maxHeight, h);
			minHeight = std::min(minHeight, h);
		}
	}

	maxh = maxHeight;
	minh = minHeight;

	// Fill vertexBuffer
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
}

// ---------------------------------------------------------------------
// Reset the patch.
//
void Patch::Reset()
{
	// Assume patch is not visible.
	m_isVisible = false;

	// Reset the important relationships
	m_BaseLeft = TriTreeNode();
	m_BaseRight = TriTreeNode();

	// Attach the two m_Base triangles together
	m_BaseLeft.BaseNeighbor = &m_BaseRight;
	m_BaseRight.BaseNeighbor = &m_BaseLeft;
}

// ---------------------------------------------------------------------
// Compute the variance tree for each of the Binary Triangles in this patch.
//
void Patch::ComputeVariance()
{
	// Compute variance on each of the base triangles...

	m_CurrentVariance = m_VarianceLeft;
	RecursComputeVariance(0, PATCH_SIZE, m_HeightMap[PATCH_SIZE * (mapx+1)],
			PATCH_SIZE, 0, m_HeightMap[PATCH_SIZE], 0, 0, m_HeightMap[0], 1);

	m_CurrentVariance = m_VarianceRight;
	RecursComputeVariance(PATCH_SIZE, 0, m_HeightMap[PATCH_SIZE], 0,
			PATCH_SIZE, m_HeightMap[PATCH_SIZE * (mapx+1)], PATCH_SIZE, PATCH_SIZE,
			m_HeightMap[(PATCH_SIZE * (mapx+1)) + PATCH_SIZE], 1);

	// Clear the dirty flag for this patch
	m_VarianceDirty = 0;
}

// ---------------------------------------------------------------------
// Set patch's visibility flag.
//
void Patch::UpdateVisibility()
{
	const float3 mins(
		m_WorldX * SQUARE_SIZE,
		minh,
		m_WorldY * SQUARE_SIZE
	);
	const float3 maxs(
		(m_WorldX + PATCH_SIZE) * SQUARE_SIZE,
		maxh,
		(m_WorldY + PATCH_SIZE) * SQUARE_SIZE
	);
	m_isVisible = cam2->InView(mins, maxs);
}

// ---------------------------------------------------------------------
// Create an approximate mesh.
//
void Patch::Tessellate(const float3& campos, int viewradius)
{
	const float myx = (m_WorldX + PATCH_SIZE / 2) * SQUARE_SIZE;
	const float myz = (m_WorldY + PATCH_SIZE / 2) * SQUARE_SIZE;

	distfromcam  = math::fabs(campos.x - myx) + campos.y + math::fabs(campos.z - myz);
	distfromcam *= 200.0f / viewradius;

	// Split each of the base triangles
	m_CurrentVariance = m_VarianceLeft;
	RecursTessellate(
		&m_BaseLeft, 
		m_WorldX, 
		m_WorldY + PATCH_SIZE, 
		m_WorldX + PATCH_SIZE, 
		m_WorldY, 
		m_WorldX, 
		m_WorldY, 
		1);

	m_CurrentVariance = m_VarianceRight;
	RecursTessellate(&m_BaseRight, 
		m_WorldX + PATCH_SIZE, 
		m_WorldY, 
		m_WorldX,
		m_WorldY + PATCH_SIZE, 
		m_WorldX + PATCH_SIZE,
		m_WorldY + PATCH_SIZE, 
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

				glVertexPointer(3, GL_FLOAT, 0, &vertices[0]);       // last param is offset, not ptr
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


int Patch::Render(bool waterdrawn)
{
	indices.clear();
	RecursRender(&m_BaseLeft, 0, PATCH_SIZE, PATCH_SIZE, 0, 0, 0, true, 0, waterdrawn);
	RecursRender(&m_BaseRight, PATCH_SIZE, 0, 0, PATCH_SIZE, PATCH_SIZE, PATCH_SIZE, false, 0, waterdrawn);

	switch (renderMode) {
		case DL:
			glNewList(triList, GL_COMPILE);
				glEnableClientState(GL_VERTEX_ARRAY);             // activate vertex coords array
					glVertexPointer(3, GL_FLOAT, 0, &vertices[0]);       // last param is offset, not ptr
					glDrawRangeElements(GL_TRIANGLES, 0, vertices.size(), indices.size(), GL_UNSIGNED_INT, &indices[0]);
				glDisableClientState(GL_VERTEX_ARRAY);            // deactivate vertex array
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
	drawer->SetupBigSquare(m_WorldX / PATCH_SIZE, m_WorldY / PATCH_SIZE);
}


	void Patch::ToggleRenderMode() {
		switch (renderMode) {
			case VA:
				LOG("Set ROAM mode to DispList");
				renderMode = DL; break;
			case DL:
				LOG("Set ROAM mode to VBO");
				renderMode = VBO; break;
			default:
				LOG("Set ROAM mode to VA");
				renderMode = VA; break;
		}
	}
