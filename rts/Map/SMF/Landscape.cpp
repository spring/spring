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


#include "Landscape.h"
#include "Game/Camera.h"
#include "Map/MapDamage.h"
#include "Map/SMF/SMFGroundDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Rectangle.h"
#include "System/Log/ILog.h"
#include <cmath>

// -------------------------------------------------------------------------------------------------
//	LANDSCAPE CLASS
// -------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------
// Definition of the static member variables
//
int Landscape::m_NextTriNode;
TriTreeNode Landscape::m_TriPool[POOL_SIZE];

// ---------------------------------------------------------------------
// Ctor
//
Landscape::Landscape()
	: CEventClient("[Landscape]", 271989, false)
{
	eventHandler.AddClient(this);
}


void Landscape::Init(CSMFGroundDrawer* _drawer, const float* hMap, int bx, int by)
{
	drawer = _drawer;

	// Store the Height Field array
	h = by;
	w = bx;
	updateCount = 0;

	m_Patches.resize((bx / PATCH_SIZE) * (by / PATCH_SIZE));

	// Initialize all terrain patches
	for (int Y = 0; Y < by / PATCH_SIZE; Y++) {
		for (int X = 0; X < bx / PATCH_SIZE; X++) {
			Patch& patch = m_Patches[Y * (bx / PATCH_SIZE) + X];
			patch.Init(
					drawer,
					X * PATCH_SIZE,
					Y * PATCH_SIZE,
					hMap,
					bx);
			patch.ComputeVariance();
		}
	}
}


// ---------------------------------------------------------------------
// Allocate a TriTreeNode from the pool.
//
TriTreeNode* Landscape::AllocateTri()
{
	// IF we've run out of TriTreeNodes, just return NULL (this is handled gracefully)
	if (m_NextTriNode >= POOL_SIZE)
		return NULL;

	TriTreeNode* pTri = &(m_TriPool[m_NextTriNode++]);
	pTri->LeftChild = pTri->RightChild = NULL;
	return pTri;
}


// ---------------------------------------------------------------------
// Reset all patches, recompute variance if needed
//
void Landscape::Reset()
{
	// Set the next free triangle pointer back to the beginning
	SetNextTriNode(0);

	// Go through the patches performing resets, compute variances, and linking.
	for (int Y = 0; Y < h / PATCH_SIZE; Y++)
		for (int X = 0; X < w / PATCH_SIZE; X++) {
			Patch* patch = &(m_Patches[Y * (w / PATCH_SIZE) + X]);

			// Reset the patch
			patch->Reset();
			patch->UpdateVisibility();

			// Check to see if this patch has been deformed since last frame.
			// If so, recompute the varience tree for it.
			//if (!mapDamage->disabled):

			//patch->ComputeVariance();

			//if (patch->isVisibile()) {
			if (true){
				// Link all the patches together.
				if (X > 0)
					patch->GetBaseLeft()->LeftNeighbor = m_Patches[Y * (w
							/ PATCH_SIZE) + X - 1].GetBaseRight();
				else
					patch->GetBaseLeft()->LeftNeighbor = NULL; // Link to bordering Landscape here..

				if (X < ((w / PATCH_SIZE) - 1))
					patch->GetBaseRight()->LeftNeighbor = m_Patches[Y * (w
							/ PATCH_SIZE) + X + 1].GetBaseLeft();
				else
					patch->GetBaseRight()->LeftNeighbor = NULL; // Link to bordering Landscape here..

				if (Y > 0)
					patch->GetBaseLeft()->RightNeighbor = m_Patches[(Y - 1)
							* (w / PATCH_SIZE) + X].GetBaseRight();
				else
					patch->GetBaseLeft()->RightNeighbor = NULL; // Link to bordering Landscape here..

				if (Y < ((h / PATCH_SIZE) - 1))
					patch->GetBaseRight()->RightNeighbor = m_Patches[(Y + 1)
							* (w / PATCH_SIZE) + X].GetBaseLeft();
				else
					patch->GetBaseRight()->RightNeighbor = NULL; // Link to bordering Landscape here..
			}
		}

}

// ---------------------------------------------------------------------
// Create an approximate mesh of the landscape.
//
void Landscape::Tessellate(const float3& campos, int viewradius)
{
	// Perform Tessellation
	for (std::vector<Patch>::iterator it = m_Patches.begin(); it != m_Patches.end(); it++) {
		if (it->isVisibile())
			it->Tessellate(campos, viewradius);
	}
	updateCount = 0;
}

// ---------------------------------------------------------------------
// Render each patch of the landscape & adjust the frame variance.
//
int Landscape::Render(bool camMoved, bool inShadowPass, bool waterdrawn)
{
	bool dirty = false;

	for (std::vector<Patch>::iterator it = m_Patches.begin(); it != m_Patches.end(); it++) {
		if (it->isDirty() == 1) {
			dirty = true;
			it->ComputeVariance();
		}
	}

	if (dirty) {
		Reset();
		Tessellate(ZeroVector, 200);
	}

	int tricount = 0;
	for (std::vector<Patch>::iterator it = m_Patches.begin(); it != m_Patches.end(); it++) {
		if (it->isVisibile()) {
			if (camMoved)
				it->Render(waterdrawn);

			if (!inShadowPass)
				it->SetSquareTexture();

			tricount += it->GetTriCount();
			it->DrawTriArray();
		}
	}

	//if (GetNextTriNode() != gDesiredTris)
	//	gFrameVariance += ((float) GetNextTriNode() - (float) gDesiredTris)
	//			/ (float) gDesiredTris;
	//if ( gFrameVariance < 0.5 )
	//gFrameVariance = 1;
	return tricount;
}


void Landscape::UnsyncedHeightMapUpdate(const SRectangle& rect)
{
	//FIXME find a better to way to find out if ROAM is used
	if (m_Patches.empty())
		return;

	updateCount = 0;

	const int xstart = std::floor(rect.x1 / PATCH_SIZE);
	const int xend   = std::ceil( rect.x2 / PATCH_SIZE);
	const int zstart = std::floor(rect.z1 / PATCH_SIZE);
	const int zend   = std::ceil( rect.z2 / PATCH_SIZE);

	const int pwidth = (gs->mapx / PATCH_SIZE);

	for (int z = zstart; z <= zend; z++) {
		for (int x = xstart; x <= xend; x++) {
			Patch& p = m_Patches[z * pwidth + x];
			p.UpdateHeightMap();
			p.m_VarianceDirty = 1;

			updateCount++;
		}
	}
}
