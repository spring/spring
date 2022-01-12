/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RectangleOverlapHandler.h"
#include "System/Log/ILog.h"

#include <cassert>

CR_BIND(CRectangleOverlapHandler, )
CR_REG_METADATA(CRectangleOverlapHandler, (
	CR_MEMBER(rectangles),
	CR_MEMBER(frontIdx),
	CR_MEMBER(needsUpdate)
))

size_t CRectangleOverlapHandler::statsTotalSize = 0;
size_t CRectangleOverlapHandler::statsOptimSize = 0;

CRectangleOverlapHandler::~CRectangleOverlapHandler()
{
	float reduction = 0.0f;

	if (statsTotalSize > 0)
		reduction = 100.0f - ((100.0f * statsOptimSize) / statsTotalSize);

	LOG("[%s] %.0f%% overlap reduction", __func__, reduction);
}


size_t CRectangleOverlapHandler::GetTotalArea() const
{
	size_t ret = 0;
	for (const SRectangle& r: rectangles) {
		ret += r.GetArea();
	}
	return ret;
}


void CRectangleOverlapHandler::Process()
{
	if (!needsUpdate)
		return;

	// FIXME: any rectangles left from the last update will be counted twice
	statsTotalSize += GetTotalArea();

	StageMerge();
	StageOverlap();
	StageMerge();
	StageSplitTooLarge();

	statsOptimSize += GetTotalArea();

	needsUpdate = false;
}

void CRectangleOverlapHandler::StageMerge()
{
	std::sort(begin(), end());

	for (size_t i = frontIdx, n = rectangles.size(); i < n; i++) {
		if (rectangles[i].GetArea() == 0)
			continue;

		for (size_t j = i + 1; j < n; j++) {
			if (rectangles[j].GetArea() == 0)
				continue;

			if (!HandleMerge(rectangles[i], rectangles[j]))
				continue;

			rectangles[i] = {};
			break;
		}
	}

	RemoveEmptyRects();
}


void CRectangleOverlapHandler::StageOverlap()
{
	// fix overlap
	// NOTE: unlike HandleMerge, HandleOverlapping can add new rects
	for (size_t i = frontIdx; i < rectangles.size(); i++) {
		if (rectangles[i].GetArea() == 0)
			continue;

		for (size_t j = i + 1; j < rectangles.size(); j++) {
			bool breakInner = true;

			if (rectangles[j].GetArea() == 0)
				continue;

			switch (HandleOverlapping(&rectangles[i], &rectangles[j])) {
				case -1: {
					rectangles[i] = {};
				} break;
				case +1: {
					// rectangle <i> fully contains rectangle <j>
					// exchange their positions and get rid of j
					std::swap(rectangles[i], rectangles[j]);
					rectangles[i] = {};
				} break;
				default: {
					breakInner = false;
				} break;
			}

			if (breakInner)
				break;
		}
	}

	RemoveEmptyRects();
}


void CRectangleOverlapHandler::StageSplitTooLarge()
{
	// split too-large rects
	for (size_t i = frontIdx; i < rectangles.size(); i++) {
		SRectangle& rect1 = rectangles[i];

		int width  = rect1.GetWidth();
		int height = rect1.GetHeight();

		while ((width * height) > maxAreaPerRect) {
			SRectangle rect2 = rect1;

			if (width > maxAreaPerRect) {
				rect1.x2 = (rect1.x1 + rect1.x2) / 2;
				rect2.x1 = (rect2.x1 + rect2.x2) / 2;
			} else {
				rect1.z2 = (rect1.z1 + rect1.z2) / 2;
				rect2.z1 = (rect2.z1 + rect2.z2) / 2;
			}

			rectangles.push_back(rect2);

			width  = rect1.GetWidth();
			height = rect1.GetHeight();
		}
	}
}


inline std::bitset<4> CRectangleOverlapHandler::GetEdgesInRect(const SRectangle& rect1, const SRectangle& rect2)
{
	std::bitset<4> bits;
	bits[0] = (rect2.x1 >= rect1.x1) && (rect2.x1 <= rect1.x2);
	bits[1] = (rect2.x2 >= rect1.x1) && (rect2.x2 <= rect1.x2);
	bits[2] = (rect2.z1 >= rect1.z1) && (rect2.z1 <= rect1.z2);
	bits[3] = (rect2.z2 >= rect1.z1) && (rect2.z2 <= rect1.z2);
	return bits;
}


inline std::bitset<4> CRectangleOverlapHandler::GetSharedEdges(const SRectangle& rect1, const SRectangle& rect2)
{
	std::bitset<4> bits;
	bits[0] = (rect2.x1 == rect1.x1) || (rect2.x1 == rect1.x2);
	bits[1] = (rect2.x2 == rect1.x1) || (rect2.x2 == rect1.x2);
	bits[2] = (rect2.z1 == rect1.z1) || (rect2.z1 == rect1.z2);
	bits[3] = (rect2.z2 == rect1.z1) || (rect2.z2 == rect1.z2);
	return bits;
}


inline bool CRectangleOverlapHandler::AreMergable(const SRectangle& rect1, const SRectangle& rect2)
{
	if (!rect1.CheckOverlap(rect2))
		return false;

	return (rect1.x1 == rect2.x1 && rect1.x2 == rect2.x2) || (rect1.z1 == rect2.z1 && rect1.z2 == rect2.z2);

}


bool CRectangleOverlapHandler::HandleMerge(SRectangle& rect1, SRectangle& rect2)
{
	//  ____
	// |    |_____
	// |    |     |  etc.
	// |____|_____|
	//
	if (!AreMergable(rect1, rect2))
		return false;

	// make rect1 point to the `larger`/`outer` rectangle
	std::bitset<4> edgesInRect12 = GetEdgesInRect(rect1, rect2);
	std::bitset<4> edgesInRect21 = GetEdgesInRect(rect2, rect1);

	if (edgesInRect12.count() < edgesInRect21.count()) {
		std::swap(edgesInRect12, edgesInRect21);
		std::swap(rect1, rect2); //FIXME only swap if we `return true`!!!
	}

	// check if edges are really shared
	// (if they are shared we can merge those two rects into one)
	std::bitset<4> sharedEdges = GetSharedEdges(rect1, rect2);

	if (edgesInRect12.count() == 3) {
		if (sharedEdges.count() == 3 || (
			(sharedEdges.count() == 2) && (
				(sharedEdges[0] && sharedEdges[1]) || (sharedEdges[2] && sharedEdges[3])
			)
		)) {
			//  __________
			// |  | |     |
			// |  | |     |
			// |__|_|_____|
			//

			//FIXME check if no overlapping (only touching), if so check maxAreaPerRect!!!

			// merge, erase rect1
			rect2.x1 = std::min(rect1.x1, rect2.x1);
			rect2.x2 = std::max(rect1.x2, rect2.x2);
			rect2.z1 = std::min(rect1.z1, rect2.z1);
			rect2.z2 = std::max(rect1.z2, rect2.z2);
			return true;
		}
	}

	return false;
}


int CRectangleOverlapHandler::HandleOverlapping(SRectangle* rect1, SRectangle* rect2)
{
	//  ______
	// |      |  ___
	// |      | |   |
	// |      | |___|
	// |______|
	//
	if (!rect1->CheckOverlap(*rect2))
		return 0;

	// make rect1 point to the `larger`/`outer` rectangle
	bool swapped = false;
	std::bitset<4> edgesInRect12 = GetEdgesInRect(*rect1, *rect2);
	std::bitset<4> edgesInRect21 = GetEdgesInRect(*rect2, *rect1);
	if (edgesInRect12.count() < edgesInRect21.count()) {
		std::swap(edgesInRect12, edgesInRect21);
		std::swap(rect1, rect2); // swap the POINTERS (not the content!)
		swapped = true;
	}


	if (edgesInRect12.count() == 4) {
		//  ________
		// |  ____  |
		// | |    | |
		// | |____| |
		// |________|

		// 2 is fully in 1
		return (swapped) ? -1 : 1;
	} else if (edgesInRect12.count() == 3) {
		//  ______
		// |     _|___
		// |    | |   |
		// |    |_|___|
		// |______|
		//

		// make one rect smaller
		if (!edgesInRect12[0]) {
			rect2->x2 = rect1->x1;
		} else if (!edgesInRect12[1]) {
			rect2->x1 = rect1->x2;
		} else if (!edgesInRect12[2]) {
			rect2->z2 = rect1->z1;
		} else {
			rect2->z1 = rect1->z2;
		}
	} else if ((edgesInRect12[0] || edgesInRect12[1]) && (edgesInRect12[2] || edgesInRect12[3]) ) {
		//assert(edgesInRect12.count() == 2);

		//  ______
		// |      |
		// |  ____|___
		// |_|____|   |
		//   |________|

		// make one smaller and create a new one
		SRectangle rect3 = *rect2;

		if (edgesInRect12[2]) {
			rect2->z1 = rect1->z2;
			rect3.z2  = rect1->z2;
		} else { //(edgesInRect12[3])
			rect2->z2 = rect1->z1;
			rect3.z1  = rect1->z1;
		}

		if (edgesInRect12[0]) {
			rect3.x1 = rect1->x2;
		} else { //(edgesInRect12[1])
			rect3.x2 = rect1->x1;
		}

		// create a new one
		rectangles.push_back(rect3);
	} else if ((edgesInRect12.count() == 2) && (edgesInRect12[0] != edgesInRect21[0])) {
		//assert(edgesInRect12.count() == 2);

		//    ______
		//  _|______|_
		// | |      | |
		// |_|______|_|
		//   |______|
		//

		// make one smaller and create a new one
		SRectangle rect3 = *rect2;

		if (edgesInRect12[0]) {
			assert(edgesInRect12[0] && edgesInRect12[1] && !edgesInRect12[2] && !edgesInRect12[3]);
			rect2->z2 = rect1->z1;
			rect3.z1  = rect1->z2;
		} else {
			assert(!edgesInRect12[0] && !edgesInRect12[1] && edgesInRect12[2] && edgesInRect12[3]);
			rect2->x2 = rect1->x1;
			rect3.x1  = rect1->x2;
		}

		// create a new one
		rectangles.push_back(rect3);
	}

	return 0;
}

