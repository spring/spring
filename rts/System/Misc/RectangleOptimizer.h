/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RECTANGLE_OPTIMIZER_H
#define RECTANGLE_OPTIMIZER_H

#include <list>
#include <bitset>
#include "System/Rectangle.h"


class CRectangleOptimizer
{
public:
	CRectangleOptimizer();
	virtual ~CRectangleOptimizer();

	void Optimize();

public:
	//! std container funcs
	typedef std::list<Rectangle>::iterator iterator;
	typedef std::list<Rectangle>::const_iterator const_iterator;
	bool empty() {
		return rectangles.empty();
	}
	size_t size() {
		return rectangles.size();
	}
	Rectangle& front() {
		return rectangles.front();
	}
	void pop_front() {
		return rectangles.pop_front();
	}
	void clear() {
		needsUpdate = false;
		return rectangles.clear();
	}
	void push_back(const Rectangle& rect) {
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
	bool HandleMerge(Rectangle& rect1, Rectangle& rect2);
	int HandleOverlapping(Rectangle* rect1, Rectangle* rect2);
	static std::bitset<4> GetEdgesInRect(const Rectangle& rect1, const Rectangle& rect2);
	static std::bitset<4> GetSharedEdges(const Rectangle& rect1, const Rectangle& rect2);
	static bool DoOverlap(const Rectangle& rect1, const Rectangle& rect2);
	static bool AreMergable(const Rectangle& rect1, const Rectangle& rect2);

	unsigned GetTotalArea() const;

private:
	std::list<Rectangle> rectangles;
	bool needsUpdate;

private:
	static unsigned statsTotalSize;
	static unsigned statsOptSize;
};

#endif // RECTANGLE_OPTIMIZER_H

