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



#include "Rendering/GL/VertexArray.h"
#include "SMFGroundDrawer.h"
#include "Landscape.h"
#include "Game/Camera.h"
#include "System/LogOutput.h"

// -------------------------------------------------------------------------------------------------
//	PATCH CLASS

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
	int centerX = (leftX + rightX) >> 1; // Compute X coordinate of center of Hypotenuse
	int centerY = (leftY + rightY) >> 1; // Compute Y coord...	
	int size=MAX(MAX(leftY - rightY,rightY - leftY ),MAX(leftX - rightX,rightX - leftX ));
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
		bool stoptess=true; //this is just for them debuggings
	}
}

// ---------------------------------------------------------------------
// Render the tree.
//


void Patch::RecursRender3(TriTreeNode *tri, int leftX, int leftY, int rightX,
		int rightY, int apexX, int apexY, int n, bool dir, int maxdepth,bool waterdrawn)
{

	int m_depth=maxdepth+1;
	
	if ( tri->LeftChild == NULL || maxdepth>12 )  // All non-leaf nodes have both children, so just check for one
	{
		#ifndef ROAM_VBO
		superfloat[lend]=(apexX+m_WorldX)*8;
		superfloat[lend + 1]= heightData[(apexY+m_WorldY) * (mapx+1) + apexX+m_WorldX];
		if (waterdrawn && superfloat[lend+1]<0) superfloat[lend+1] *=2;
		superfloat[lend+2]=(apexY+m_WorldY)*8;
		lend+=3;
		//push left
		superfloat[lend]=(leftX+m_WorldX)*8;
		superfloat[lend + 1]= heightData[(leftY+m_WorldY) * (mapx+1) + leftX+m_WorldX];
		if (waterdrawn && superfloat[lend+1]<0) superfloat[lend+1] *=2;
		superfloat[lend+2]=(leftY+m_WorldY)*8;
		lend+=3;
		//push right
		superfloat[lend]=(rightX+m_WorldX)*8;
		superfloat[lend + 1]= heightData[(rightY+m_WorldY) * (mapx+1) + rightX+m_WorldX];
		if (waterdrawn && superfloat[lend+1]<0) superfloat[lend+1] *=2;
		superfloat[lend+2]=(rightY+m_WorldY)*8;
		lend+=3;
		#else
				//roamvbo
		superint[rend++]=apexX	+(PATCH_SIZE+1)*apexY	;
		superint[rend++]=leftX	+(PATCH_SIZE+1)*leftY	;
		superint[rend++]=rightX	+(PATCH_SIZE+1)*rightY	;
		#endif
	} 
	else
	{
		int centerX = (leftX + rightX) >> 1; // Compute X coordinate of center of Hypotenuse
		int centerY = (leftY + rightY) >> 1; // Compute Y coord...
		RecursRender3(tri->LeftChild, apexX, apexY, leftX, leftY, centerX,	centerY, n,dir,m_depth,waterdrawn);
		RecursRender3(tri->RightChild, rightX, rightY, apexX, apexY, centerX,centerY, n,dir,m_depth,waterdrawn);
	
	}
}



// ---------------------------------------------------------------------
// Computes Variance over the entire tree.  Does not examine node relationships.
//
unsigned char Patch::RecursComputeVariance(int leftX, int leftY,
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
	if (leftZ*rightZ<0 ||leftZ*centerZ<0||rightZ*centerZ<0) myVariance=MAX(myVariance*2,20.0f); //shore lines get more variance for higher accuracy
	//myVariance= MAX(abs(leftX - rightX),abs(leftY - rightY))*myVariance;
	if (myVariance<0){
		myVariance=100;
	}
	// Since we're after speed and not perfect representations,
	//    only calculate variance down to a 4x4 block
	if ((abs(leftX - rightX) >= 4) || (abs(leftY - rightY) >= 4)) {
		// Final Variance for this node is the max of it's own variance and that of it's children.
		myVariance	=
				MAX( myVariance, RecursComputeVariance( apexX, apexY, apexZ, leftX, leftY, leftZ, centerX, centerY, centerZ, node<<1 ) );
		myVariance=
			MAX( myVariance, RecursComputeVariance( rightX, rightY, rightZ, apexX, apexY, apexZ, centerX, centerY, centerZ, 1+(node<<1)) );
	}
	
	// Store the final variance for this node.  Note Variance is never zero.
	if (node < (1 << VARIANCE_DEPTH))
		m_CurrentVariance[node] = myVariance;//NOW IT IS 0 MUWAHAHAHAHA
	if (myVariance<0){
		myVariance=0.001;
	}
	return myVariance;
}

// ---------------------------------------------------------------------
// Initialize a patch.
//
void Patch::Init(int heightX, int heightY, int worldX, int worldY,
		const float *hMap, int mx, float maxH, float minH)
{
	// Clear all the relationships
	maxh = maxH;
	minh = minH;
	mapx = mx;
	m_BaseLeft.RightNeighbor = m_BaseLeft.LeftNeighbor
			= m_BaseRight.RightNeighbor = m_BaseRight.LeftNeighbor
					= m_BaseLeft.LeftChild = m_BaseLeft.RightChild
							= m_BaseRight.LeftChild = m_BaseLeft.LeftChild
									= NULL;

	// Attach the two m_Base triangles together
	m_BaseLeft.BaseNeighbor = &m_BaseRight;
	m_BaseRight.BaseNeighbor = &m_BaseLeft;

	// Store Patch offsets for the world and heightmap.
	m_WorldX = worldX;
	m_WorldY = worldY;

	// Store pointer to first byte of the height data for this patch.
	m_HeightMap = &hMap[heightY * (mapx+1) + heightX];

	// Initialize flags
	m_VarianceDirty = 1;
	m_isVisible = 0;
	lend=0;
#ifdef ROAM_DL
	triList=0;
#endif

#ifdef ROAM_VBO
	//roamvbo
	rend=0;
	float * vbuf;
	int index=0;
	vbuf=new float[3*(PATCH_SIZE+1)*(PATCH_SIZE+1)];
	for (int x=heightX; x<=heightX+PATCH_SIZE;x++){
		for (int y=heightY; y<=heightY+PATCH_SIZE;y++){
				vbuf[index++]=heightX*8;
				vbuf[index++]=hMap[heightY * (mapx+1) + heightX];
				vbuf[index++]=heightY*8;
		}
	}
	glGenBuffersARB(1,&vertexBuffer);
	glGenBuffersARB(1,&vertexIndexBuffer);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vertexBuffer);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,3*(PATCH_SIZE+1)*(PATCH_SIZE+1),vbuf,GL_STATIC_DRAW_ARB );
	int bufferSize = 0;
    glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &bufferSize);
	if(index != bufferSize)
    {
        glDeleteBuffersARB(1, &vertexIndexBuffer);
		LogObject() << "[createVBO()] Data size is mismatch with input array\n";
    }
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	delete [] vbuf;
#endif
}

// ---------------------------------------------------------------------
// Reset the patch.
//
void Patch::Reset()
{
	// Assume patch is not visible.
	m_isVisible = 0;

	// Reset the important relationships
	m_BaseLeft.LeftChild = m_BaseLeft.RightChild = m_BaseRight.LeftChild
			= m_BaseLeft.LeftChild = NULL;

	// Attach the two m_Base triangles together
	m_BaseLeft.BaseNeighbor = &m_BaseRight;
	m_BaseRight.BaseNeighbor = &m_BaseLeft;

	// Clear the other relationships.
	m_BaseLeft.RightNeighbor = m_BaseLeft.LeftNeighbor
			= m_BaseRight.RightNeighbor = m_BaseRight.LeftNeighbor = NULL;
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
void Patch::SetVisibility() {
	float3 patchPos;
		patchPos.x = (m_WorldX + PATCH_SIZE / 2) * 8;
		patchPos.z = (m_WorldY + PATCH_SIZE / 2) * 8;
		patchPos.y = (maxh + minh) * 0.5f;
	m_isVisible = int(cam2->InView(patchPos, 768));
}

// ---------------------------------------------------------------------
// Create an approximate mesh.
//
void Patch::Tessellate(float cx, float cy, float cz, int viewradius)
{
	const float myx = (m_WorldX + PATCH_SIZE / 2) * 8;
	const float myz = (m_WorldY + PATCH_SIZE / 2) * 8;

	this->distfromcam = (math::fabs(cx - myx) + cy + math::fabs(cz - myz))
			* ((float) 200 / viewradius);
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

void Patch::DrawTriArray(CSMFGroundDrawer * parent,bool inShadowPass)
{
	if (m_WorldX==0 && m_WorldY==0 && parent->shc %1000==50){
		for(int i=0;i<=lend;i+=9){
			//LogObject() << "@" << superfloat[i] << "," <<  superfloat[i+1] << "," << superfloat[i+2]<< "@" << superfloat[i+3] << "," <<  superfloat[i+4] << "," << superfloat[i+5]<< "@" << superfloat[i+6] << "," <<  superfloat[i+7] << "," << superfloat[i+8];
		}
		//LogObject() << "push done shc:" << parent->shc << "inshadowpass="<<inShadowPass;
	}
#ifdef ROAM_VA

	//roamarray
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, superfloat);
	glDrawArrays(GL_TRIANGLES, 0, lend);
	//DrawArrays(drawType, stride);
	glDisableClientState(GL_VERTEX_ARRAY);
#endif


#ifdef ROAM_DL
		glCallList(triList);
#endif
#ifdef ROAM_VBO
		//roamvbo

	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vertexBuffer);         // for vertex coordinates
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, vertexIndexBuffer); // for indices
	glIndexPointer(GL_UNSIGNED_SHORT, 0, 0);
	// do same as vertex array except pointer
	glEnableClientState(GL_VERTEX_ARRAY);             // activate vertex coords array
	glVertexPointer(3, GL_FLOAT, 0, 0);               // last param is offset, not ptr
	// draw 6 quads using offset of index array
	glDrawElements(GL_TRIANGLES, rend, GL_UNSIGNED_SHORT, 0);
	glDisableClientState(GL_VERTEX_ARRAY);            // deactivate vertex array
	// bind with 0, so, switch back to normal pointer operation
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
#endif
}
int Patch::Render2(CSMFGroundDrawer * parent, int n,bool waterdrawn)
{

	lend=0;
	rend=0;

	RecursRender3(&m_BaseLeft, 0, PATCH_SIZE, PATCH_SIZE, 0, 0, 0, n, true,0,waterdrawn);

	RecursRender3(&m_BaseRight, PATCH_SIZE, 0, 0, PATCH_SIZE, PATCH_SIZE,
			PATCH_SIZE, n,false,0,waterdrawn);
	//assert(rend<200000);
	//assert(rend>-1);


#ifdef ROAM_DL
	if (triList!=0) glDeleteLists(triList,1);
	triList=glGenLists(1);
	glNewList(triList,GL_COMPILE);
	glBegin(GL_TRIANGLES);
	for (int i=0; i<lend;i+=3){
		glVertex3f(superfloat[i],superfloat[i+1],superfloat[i+2]);
	}
	glEnd();
	glEndList();
#endif
#ifdef ROAM_VBO
	//roamvbo
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,vertexIndexBuffer);
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,rend,superint,GL_DYNAMIC_DRAW_ARB);
	int bufferSize = 0;
    glGetBufferParameterivARB(GL_ELEMENT_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &bufferSize);
    if(rend != bufferSize)
    {
        glDeleteBuffersARB(1, &vertexIndexBuffer);
		LogObject() << "[createVBO()] Data size is mismatch with input array\n";
    }
	for (int i=0;i<rend;i+=3){
		LogObject() << "VBO" << superint[i] <<" " <<superint[i+1]<< " "<< superint[i+2] << "\n";
	
	}
	
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

#endif	
	
	return MAX(rend/3,lend/9); //return the number of tris rendered
}


