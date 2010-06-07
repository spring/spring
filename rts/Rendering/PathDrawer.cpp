/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "StdAfx.h"

#include "Game/SelectedUnits.h"
#include "Map/Ground.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveInfo.h"

// FIXME
#define private public
#include "Sim/Path/IPath.h"
#include "Sim/Path/PathFinder.h"
#include "Sim/Path/PathEstimator.h"
#include "Sim/Path/PathManager.h"
#undef private

#include "Sim/Units/UnitDef.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/PathDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/glExtra.h"
#include "System/GlobalUnsynced.h"

PathDrawer* PathDrawer::GetInstance() {
	static PathDrawer pd;
	return &pd;
}

void PathDrawer::Draw() const {
	 // PathManager is not thread safe; making it
	// so might be too costly (performance-wise)
	#if !defined(USE_GML) || !GML_ENABLE_SIM
	if (globalRendering->drawdebug && (gs->cheatEnabled || gu->spectating)) {
		glPushAttrib(GL_ENABLE_BIT);
		Draw(pathManager);

		Draw(pathManager->pf);
		Draw(pathManager->pe);
		Draw(pathManager->pe2);
		glPopAttrib();
	}
	#endif
}



void PathDrawer::Draw(const CPathManager* pm) const {
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glLineWidth(3);

	const std::map<unsigned int, CPathManager::MultiPath*>& pathMap = pm->pathMap;
	std::map<unsigned int, CPathManager::MultiPath*>::const_iterator pi;

	for (pi = pathMap.begin(); pi != pathMap.end(); pi++) {
		const CPathManager::MultiPath* path = pi->second;

		glBegin(GL_LINE_STRIP);

			typedef IPath::path_list_type::const_iterator PathIt;

			// draw estimatePath2 (green)
			glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
			for (PathIt pvi = path->estimatedPath2.path.begin(); pvi != path->estimatedPath2.path.end(); pvi++) {
				float3 pos = *pvi; pos.y += 5; glVertexf3(pos);
			}

			// draw estimatePath (blue)
			glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
			for (PathIt pvi = path->estimatedPath.path.begin(); pvi != path->estimatedPath.path.end(); pvi++) {
				float3 pos = *pvi; pos.y += 5; glVertexf3(pos);
			}

			// draw detailPath (red)
			glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
			for (PathIt pvi = path->detailedPath.path.begin(); pvi != path->detailedPath.path.end(); pvi++) {
				float3 pos = *pvi; pos.y += 5; glVertexf3(pos);
			}

		glEnd();

		// visualize the path definition (goal, radius)
		Draw(path->peDef);
	}

	glLineWidth(1);
}



void PathDrawer::Draw(const CPathFinderDef* pfd) const {
	glColor4f(0.0f, 1.0f, 1.0f, 1.0f);
	glSurfaceCircle(pfd->goal, sqrt(pfd->sqGoalRadius), 20);
}

void PathDrawer::Draw(const CPathFinder* pf) const {
	glColor3f(0.7f, 0.2f, 0.2f);
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);

	for (const CPathFinder::OpenSquare* os = pf->openSquareBuffer; os != pf->openSquareBufferPointer; ++os) {
		const int2 sqr = os->square;
		const int square = os->sqr;

		if (pf->squareState[square].status & CPathFinder::PATHOPT_START)
			continue;

		float3 p1;
			p1.x = sqr.x * SQUARE_SIZE;
			p1.z = sqr.y * SQUARE_SIZE;
			p1.y = ground->GetHeight(p1.x, p1.z) + 15;
		float3 p2;

		const int dir = pf->squareState[square].status & CPathFinder::PATHOPT_DIRECTION;
		const int obx = sqr.x - pf->directionVector[dir].x;
		const int obz = sqr.y - pf->directionVector[dir].y;
		const int obsquare =  obz * gs->mapx + obx;

		if (obsquare >= 0) {
			p2.x = obx * SQUARE_SIZE;
			p2.z = obz * SQUARE_SIZE;
			p2.y = ground->GetHeight(p2.x, p2.z) + 15;

			glVertexf3(p1);
			glVertexf3(p2);
		}
	}

	glEnd();
}



void PathDrawer::Draw(const CPathEstimator* pe) const {
	GML_RECMUTEX_LOCK(sel); // Draw

	MoveData* md = NULL;

	if (!moveinfo->moveData.empty()) {
		md = moveinfo->moveData[0];
	} else {
		return;
	}

	if (!selectedUnits.selectedUnits.empty() && (*selectedUnits.selectedUnits.begin())->unitDef->movedata) {
		md = (*selectedUnits.selectedUnits.begin())->unitDef->movedata;
	}

	glDisable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 0.0f);

	/*
	float blue = BLOCK_SIZE == 32? 1: 0;
	glBegin(GL_LINES);
	for (int z = 0; z < nbrOfBlocksZ; z++) {
		for (int x = 0; x < nbrOfBlocksX; x++) {
			int blocknr = z * nbrOfBlocksX + x;
			float3 p1;
			p1.x = (blockState[blocknr].sqrCenter[md->pathType].x) * 8;
			p1.z = (blockState[blocknr].sqrCenter[md->pathType].y) * 8;
			p1.y = ground->GetHeight(p1.x, p1.z) + 10;

			glColor3f(1, 1, blue);
			glVertexf3(p1);
			glVertexf3(p1 - UpVector * 10);
			for (int dir = 0; dir < PATH_DIRECTION_VERTICES; dir++) {
				int obx = x + directionVector[dir].x;
				int obz = z + directionVector[dir].y;

				if (obx >= 0 && obz >= 0 && obx < nbrOfBlocksX && obz < nbrOfBlocksZ) {
					float3 p2;
					int obblocknr = obz * nbrOfBlocksX + obx;

					p2.x = (blockState[obblocknr].sqrCenter[md->pathType].x) * 8;
					p2.z = (blockState[obblocknr].sqrCenter[md->pathType].y) * 8;
					p2.y = ground->GetHeight(p2.x, p2.z) + 10;

					int vertexNbr = md->pathType * nbrOfBlocks * PATH_DIRECTION_VERTICES + blocknr * PATH_DIRECTION_VERTICES + directionVertex[dir];
					float cost = vertex[vertexNbr];

					glColor3f(1 / (sqrt(cost/BLOCK_SIZE)), 1 / (cost/BLOCK_SIZE), blue);
					glVertexf3(p1);
					glVertexf3(p2);
				}
			}
		}

	}
	glEnd();


	glEnable(GL_TEXTURE_2D);
	for (int z = 0; z < nbrOfBlocksZ; z++) {
		for (int x = 0; x < nbrOfBlocksX; x++) {
			int blocknr = z * nbrOfBlocksX + x;
			float3 p1;
			p1.x = (blockState[blocknr].sqrCenter[md->pathType].x) * SQUARE_SIZE;
			p1.z = (blockState[blocknr].sqrCenter[md->pathType].y) * SQUARE_SIZE;
			p1.y = ground->GetHeight(p1.x, p1.z) + 10;

			glColor3f(1, 1, blue);
			for (int dir = 0; dir < PATH_DIRECTION_VERTICES; dir++) {
				int obx = x + directionVector[dir].x;
				int obz = z + directionVector[dir].y;

				if (obx >= 0 && obz >= 0 && obx < nbrOfBlocksX && obz < nbrOfBlocksZ) {
					float3 p2;
					int obblocknr = obz * nbrOfBlocksX + obx;

					p2.x = (blockState[obblocknr].sqrCenter[md->pathType].x) * SQUARE_SIZE;
					p2.z = (blockState[obblocknr].sqrCenter[md->pathType].y) * SQUARE_SIZE;
					p2.y = ground->GetHeight(p2.x, p2.z) + 10;

					int vertexNbr = md->pathType * nbrOfBlocks * PATH_DIRECTION_VERTICES + blocknr * PATH_DIRECTION_VERTICES + directionVertex[dir];
					float cost = vertex[vertexNbr];

					glColor3f(1, 1 / (cost/BLOCK_SIZE), blue);

					p2 = (p1 + p2) / 2;
					if (camera->pos.SqDistance(p2) < 250000) {
						font->glWorldPrint(p2,5,"%.0f", cost);
					}
				}
			}
		}
	}
	*/


	if (pe->BLOCK_SIZE == 8) {
		glColor3f(0.2f, 0.7f, 0.2f);
	} else {
		glColor3f(0.2f, 0.2f, 0.7f);
	}

	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);

	for (const CPathEstimator::OpenBlock* ob = pe->openBlockBuffer; ob != pe->openBlockBufferPointer; ++ob) {
		const int blocknr = ob->blocknr;

		float3 p1;
			p1.x = (pe->blockState[blocknr].sqrCenter[md->pathType].x) * SQUARE_SIZE;
			p1.z = (pe->blockState[blocknr].sqrCenter[md->pathType].y) * SQUARE_SIZE;
			p1.y = ground->GetHeight(p1.x, p1.z) + 15;
		float3 p2;

		const int obx = pe->blockState[ob->blocknr].parentBlock.x;
		const int obz = pe->blockState[ob->blocknr].parentBlock.y;
		const int obblocknr = obz * pe->nbrOfBlocksX + obx;

		if (obblocknr >= 0) {
			p2.x = (pe->blockState[obblocknr].sqrCenter[md->pathType].x) * SQUARE_SIZE;
			p2.z = (pe->blockState[obblocknr].sqrCenter[md->pathType].y) * SQUARE_SIZE;
			p2.y = ground->GetHeight(p2.x, p2.z) + 15;

			glVertexf3(p1);
			glVertexf3(p2);
		}
	}

	glEnd();


	/*
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glColor4f(1,0, blue, 0.7f);
	glAlphaFunc(GL_GREATER, 0.05f);

	for (const CPathEstimator::OpenBlock* ob = pe->openBlockBuffer; ob != pe->openBlockBufferPointer; ++ob) {
		const int blocknr = ob->blocknr;
		float3 p1;
			p1.x = (ob->block.x * BLOCK_SIZE + pe->blockState[blocknr].sqrCenter[md->pathType].x) * SQUARE_SIZE;
			p1.z = (ob->block.y * BLOCK_SIZE + pe->blockState[blocknr].sqrCenter[md->pathType].y) * SQUARE_SIZE;
			p1.y = ground->GetHeight(p1.x, p1.z) + 15;

		if (camera->pos.SqDistance(p1) < 250000) {
			font->glWorldPrint(p1, 5, "%.0f %.0f", ob->cost, ob->currentCost);
		}
	}
	glDisable(GL_BLEND);
	*/
}
