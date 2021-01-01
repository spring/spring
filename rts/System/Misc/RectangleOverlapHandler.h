/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RECTANGLE_OVERLAP_HANDLER_H
#define RECTANGLE_OVERLAP_HANDLER_H

#include <vector>
#include <bitset>

#include "System/Rectangle.h"
#include "System/creg/creg_cond.h"


/**
 * @brief CRectangleOverlapHandler
 *
 * Container & preprocessor for rectangles. It handles any overlap & merges+resizes rectangles.
 */
class CRectangleOverlapHandler
{
	CR_DECLARE_STRUCT(CRectangleOverlapHandler)

public:
	CRectangleOverlapHandler() { clear(); }
	~CRectangleOverlapHandler();

	void Process();

	size_t GetTotalArea() const;

public:
	typedef std::vector<SRectangle> container;
	typedef container::iterator iterator;
	typedef container::const_iterator const_iterator;

	bool empty() const { return (frontIdx >= rectangles.size()); }
	size_t size() const { return (rectangles.size() - frontIdx); }

	SRectangle& front() { return rectangles.at(frontIdx); }
	SRectangle& back() { return rectangles.back(); }

	void pop_front() {
		if (!empty()) {
			// leave a null-rectangle so RemoveEmptyRects will clean it up
			rectangles[frontIdx++] = {};
			return;
		}

		clear();
	}
	void push_back(const SRectangle& rect) {
		// skip zero- or negative-area rectangles
		// assert(rect.GetArea() > 0);
		if (rect.GetArea() <= 0)
			return;
		needsUpdate = true;
		rectangles.push_back(rect);
	}

	void swap(CRectangleOverlapHandler& other) {
		std::swap(rectangles, other.rectangles);
		std::swap(frontIdx, other.frontIdx);
		std::swap(needsUpdate, other.needsUpdate);
	}

	void append(CRectangleOverlapHandler& other) {
		needsUpdate = (other.needsUpdate || !empty());

		for (const SRectangle& r: other.rectangles) {
			rectangles.push_back(r);
		}

		other.clear();
	}

	void clear() {
		frontIdx = 0;
		needsUpdate = false;

		rectangles.clear();
		rectangles.reserve(512);
	}

	const_iterator cbegin() { return (rectangles.cbegin() + frontIdx); }
	const_iterator cend() { return (rectangles.cend()); }

	iterator begin() { return (rectangles.begin() + frontIdx); }
	iterator end() { return (rectangles.end()); }

private:
	void StageMerge();
	void StageOverlap();
	void StageSplitTooLarge();
	void RemoveEmptyRects() {
		size_t j = (frontIdx = 0);

		for (size_t i = j, n = rectangles.size(); i < n; i++) {
			if (rectangles[i].GetArea() <= 0)
				continue;

			rectangles[j++] = rectangles[i];
		}

		// shrink without reallocating
		rectangles.resize(j);
	}

	bool HandleMerge(SRectangle& rect1, SRectangle& rect2);
	int HandleOverlapping(SRectangle* rect1, SRectangle* rect2);
	static std::bitset<4> GetEdgesInRect(const SRectangle& rect1, const SRectangle& rect2);
	static std::bitset<4> GetSharedEdges(const SRectangle& rect1, const SRectangle& rect2);
	static bool AreMergable(const SRectangle& rect1, const SRectangle& rect2);

private:
	container rectangles;

	constexpr static int maxAreaPerRect = 500 * 500;

	static size_t statsTotalSize;
	static size_t statsOptimSize;

	size_t frontIdx = 0;

	bool needsUpdate = false;
};

#endif

