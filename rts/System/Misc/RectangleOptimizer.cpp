/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RectangleOptimizer.h"
#ifdef RO_USE_DEQUE_CONTAINER
#include "System/creg/STL_Deque.h"
#else
#include "System/creg/STL_List.h"
#endif
#include "System/Log/ILog.h"

#include <cassert>

CR_BIND(CRectangleOptimizer, )
CR_REG_METADATA(CRectangleOptimizer, (
	CR_MEMBER(maxAreaPerRect),
	CR_MEMBER(rectangles),
	CR_MEMBER(needsUpdate)
))

unsigned CRectangleOptimizer::statsTotalSize = 0;
unsigned CRectangleOptimizer::statsOptSize   = 0;

CRectangleOptimizer::CRectangleOptimizer()
	: maxAreaPerRect(500 * 500) // FIXME auto adjust this in HeightMapUpdate!
	, needsUpdate(false)
{
}


CRectangleOptimizer::~CRectangleOptimizer()
{
	float reduction = 0.f;
	if (statsTotalSize > 0) {
		reduction = 100.f - ((100.f * statsOptSize) / statsTotalSize);
	}
	LOG("Statistics for RectangleOptimizer: %.0f%%", reduction);
}


unsigned CRectangleOptimizer::GetTotalArea() const
{
	unsigned ret = 0;
	for (CRectangleOptimizer::const_iterator it = rectangles.begin(); it != rectangles.end(); ++it) {
		const int w = it->GetWidth();
		const int h = it->GetHeight();
		ret += (w * h);
	}
	return ret;
}


void CRectangleOptimizer::Optimize()
{
	if (!needsUpdate)
		return;

	//TODO this is not fully correct, when there was still rectangles
	//     left from the last update we shouldn't count them twice!
	statsTotalSize += GetTotalArea();

	StageMerge();
	StageOverlap();
	StageMerge();
	StageSplitTooLarge();

	statsOptSize += GetTotalArea();

	needsUpdate = false;
}

void CRectangleOptimizer::StageMerge()
{
	CRectangleOptimizer::iterator it;
	CRectangleOptimizer::iterator jt;

	#ifdef RO_USE_DEQUE_CONTAINER
	std::sort(rectangles.begin(), rectangles.end());
	#else
	rectangles.sort();
	#endif

	for (it = rectangles.begin(); it != rectangles.end(); ++it) {
		for (jt = it, ++jt; jt != rectangles.end(); ++jt) {
			const bool del = HandleMerge(*it, *jt);
			if (del) {
				it = rectangles.erase(it);
				jt = it;
			}
		}
	}
}


void CRectangleOptimizer::StageOverlap()
{
	CRectangleOptimizer::iterator it;
	CRectangleOptimizer::iterator jt;

	//! Fix Overlap
	for (it = rectangles.begin(); it != rectangles.end(); ++it) {
		for (jt = it, ++jt; jt != rectangles.end(); ++jt) {
			const int del = HandleOverlapping(&(*it), &(*jt));
			if (del < 0) {
				it = rectangles.erase(it);
				jt = it;
			} else if (del > 0) {
				//jt = rectangles.erase(jt);
				std::swap(*it, *jt);
				it = rectangles.erase(it);
				jt = it;
			}
		}
	}
}


void CRectangleOptimizer::StageSplitTooLarge()
{
	CRectangleOptimizer::iterator it;

	//! Split too large
	for (it = rectangles.begin(); it != rectangles.end(); ++it) {
		SRectangle& rect1 = *it;
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


inline std::bitset<4> CRectangleOptimizer::GetEdgesInRect(const SRectangle& rect1, const SRectangle& rect2)
{
	std::bitset<4> bits;
	bits[0] = (rect2.x1 >= rect1.x1) && (rect2.x1 <= rect1.x2);
	bits[1] = (rect2.x2 >= rect1.x1) && (rect2.x2 <= rect1.x2);
	bits[2] = (rect2.z1 >= rect1.z1) && (rect2.z1 <= rect1.z2);
	bits[3] = (rect2.z2 >= rect1.z1) && (rect2.z2 <= rect1.z2);
	return bits;
}


inline std::bitset<4> CRectangleOptimizer::GetSharedEdges(const SRectangle& rect1, const SRectangle& rect2)
{
	std::bitset<4> bits;
	bits[0] = (rect2.x1 == rect1.x1) || (rect2.x1 == rect1.x2);
	bits[1] = (rect2.x2 == rect1.x1) || (rect2.x2 == rect1.x2);
	bits[2] = (rect2.z1 == rect1.z1) || (rect2.z1 == rect1.z2);
	bits[3] = (rect2.z2 == rect1.z1) || (rect2.z2 == rect1.z2);
	return bits;
}


inline bool CRectangleOptimizer::AreMergable(const SRectangle& rect1, const SRectangle& rect2)
{
	if (!rect1.CheckOverlap(rect2))
		return false;

	return (rect1.x1 == rect2.x1 && rect1.x2 == rect2.x2) || (rect1.z1 == rect2.z1 && rect1.z2 == rect2.z2);

}


bool CRectangleOptimizer::HandleMerge(SRectangle& rect1, SRectangle& rect2)
{
	if (!AreMergable(rect1, rect2)) {
		//  ____
		// |    |_____
		// |    |     |  etc.
		// |____|_____|
		//
		return false;
	}

	//! make rect1 point to the `larger`/`outer` rectangle
	std::bitset<4> edgesInRect12 = GetEdgesInRect(rect1, rect2);
	std::bitset<4> edgesInRect21 = GetEdgesInRect(rect2, rect1);
	if (edgesInRect12.count() < edgesInRect21.count()) {
		std::swap(edgesInRect12, edgesInRect21);
		std::swap(rect1, rect2); //FIXME only swap if we `return true`!!!
	}

	//! check if edges are really shared
	//! (if they are shared we can merge those two rects into one)
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

			//! merge
			rect2.x1 = std::min(rect1.x1, rect2.x1);
			rect2.x2 = std::max(rect1.x2, rect2.x2);
			rect2.z1 = std::min(rect1.z1, rect2.z1);
			rect2.z2 = std::max(rect1.z2, rect2.z2);
			return true; //! erase rect1
		}
	}

	return false;
}


int CRectangleOptimizer::HandleOverlapping(SRectangle* rect1, SRectangle* rect2)
{
	if (!rect1->CheckOverlap(*rect2)) {
		//  ______
		// |      |  ___
		// |      | |   |
		// |      | |___|
		// |______|
		//

		return 0;
	}

	//! make rect1 point to the `larger`/`outer` rectangle
	bool swapped = false;
	std::bitset<4> edgesInRect12 = GetEdgesInRect(*rect1, *rect2);
	std::bitset<4> edgesInRect21 = GetEdgesInRect(*rect2, *rect1);
	if (edgesInRect12.count() < edgesInRect21.count()) {
		std::swap(edgesInRect12, edgesInRect21);
		std::swap(rect1, rect2); //! swap the POINTERS (not the content!)
		swapped = true;
	}


	if (edgesInRect12.count() == 4) {
		//  ________
		// |  ____  |
		// | |    | |
		// | |____| |
		// |________|

		//! 2 is fully in 1
		return (swapped) ? -1 : 1;
	} else if (edgesInRect12.count() == 3) {
		//  ______
		// |     _|___
		// |    | |   |
		// |    |_|___|
		// |______|
		//

		//! make one rect smaller
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

		//! make one smaller and create a new one
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

		//! create a new one
		rectangles.push_back(rect3);
	} else if ((edgesInRect12.count() == 2) && (edgesInRect12[0] != edgesInRect21[0])) {
		//assert(edgesInRect12.count() == 2);

		//    ______
		//  _|______|_
		// | |      | |
		// |_|______|_|
		//   |______|
		//

		//! make one smaller and create a new one
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

		//! create a new one
		rectangles.push_back(rect3);
	}

	return 0;
}
