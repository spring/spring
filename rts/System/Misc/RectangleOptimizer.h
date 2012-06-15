/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RECTANGLE_OPTIMIZER_H
#define RECTANGLE_OPTIMIZER_H

#include <list>
#include <bitset>
#include "System/Rectangle.h"

/**
 * @brief CRectangleOptimizer
 *
 * Container & preprocessor for rectangles. It solves any overlapping & merges+resizes rectangles.
 */
class CRectangleOptimizer
{
public:
	CRectangleOptimizer();
	virtual ~CRectangleOptimizer();

	void Optimize();

public:
	//! std container funcs
	typedef std::list<SRectangle>::iterator iterator;
	typedef std::list<SRectangle>::const_iterator const_iterator;
	bool empty() const {
		return rectangles.empty();
	}
	size_t size() const {
		return rectangles.size();
	}
	SRectangle& front() {
		return rectangles.front();
	}
	void pop_front() {
		return rectangles.pop_front();
	}
	void clear() {
		needsUpdate = false;
		return rectangles.clear();
	}
	void push_back(const SRectangle& rect) {
		//! skip empty/negative rectangles
		//assert(rect.GetWidth() > 0 && rect.GetHeight() > 0);
		if (rect.GetWidth() <= 0 || rect.GetHeight() <= 0)
			return;
		needsUpdate = true;
		rectangles.push_back(rect);
	}
	iterator begin() {
		return rectangles.begin();
	}
	iterator end() {
		return rectangles.end();
	}

public:
	int maxAreaPerRect;

private:
	bool HandleMerge(SRectangle& rect1, SRectangle& rect2);
	int HandleOverlapping(SRectangle* rect1, SRectangle* rect2);
	static std::bitset<4> GetEdgesInRect(const SRectangle& rect1, const SRectangle& rect2);
	static std::bitset<4> GetSharedEdges(const SRectangle& rect1, const SRectangle& rect2);
	static bool DoOverlap(const SRectangle& rect1, const SRectangle& rect2);
	static bool AreMergable(const SRectangle& rect1, const SRectangle& rect2);

	unsigned GetTotalArea() const;

private:
	std::list<SRectangle> rectangles;
	bool needsUpdate;

private:
	static unsigned statsTotalSize;
	static unsigned statsOptSize;
};

#endif // RECTANGLE_OPTIMIZER_H

