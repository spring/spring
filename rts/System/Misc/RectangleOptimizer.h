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

	void Update();

public:
	//! std container funcs
	bool empty() {
		return rectangles.empty();
	}
	Rectangle& front() {
		return rectangles.front();
	}
	void pop_front() {
		return rectangles.pop_front();
	}
	void push_back(const Rectangle& rect) {
		//! skip empty/negative rectangles
		//assert(rect.GetWidth() > 0 && rect.GetHeight() > 0);
		if (rect.GetWidth() <= 0 || rect.GetHeight() <= 0)
			return;
		rectangles.push_back(rect);
	}
	void begin() {
		rectangles.begin();
	}
	void end() {
		rectangles.begin();
	}
	typedef std::list<Rectangle>::iterator iterator;
	typedef std::list<Rectangle>::const_iterator const_iterator;

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

private:
	static unsigned statsTotalSize;
	static unsigned statsOptSize;
};

#endif // RECTANGLE_OPTIMIZER_H

