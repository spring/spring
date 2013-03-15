/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RECTANGLE_OPTIMIZER_H
#define RECTANGLE_OPTIMIZER_H

#include <list>
#include <bitset>
#include "System/Rectangle.h"
#include "System/creg/creg_cond.h"


/**
 * @brief CRectangleOptimizer
 *
 * Container & preprocessor for rectangles. It solves any overlapping & merges+resizes rectangles.
 */
class CRectangleOptimizer
{
	CR_DECLARE_STRUCT(CRectangleOptimizer);

public:
	CRectangleOptimizer();
	~CRectangleOptimizer();

	void Optimize();

	unsigned GetTotalArea() const;

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
	void swap(CRectangleOptimizer &other) {
		rectangles.swap(other.rectangles);
		std::swap(needsUpdate, other.needsUpdate);
		other.maxAreaPerRect = maxAreaPerRect; // intentional one-way copy here
	}
	void splice(iterator pos, CRectangleOptimizer &other) {
		needsUpdate = other.needsUpdate || !rectangles.empty();
		rectangles.splice(pos, other.rectangles);
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
	void swap(std::list<SRectangle>& lst)
	{
		rectangles.swap(lst);
		needsUpdate = !rectangles.empty();
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
	void StageMerge();
	void StageOverlap();
	void StageSplitTooLarge();

	bool HandleMerge(SRectangle& rect1, SRectangle& rect2);
	int HandleOverlapping(SRectangle* rect1, SRectangle* rect2);
	static std::bitset<4> GetEdgesInRect(const SRectangle& rect1, const SRectangle& rect2);
	static std::bitset<4> GetSharedEdges(const SRectangle& rect1, const SRectangle& rect2);
	static bool AreMergable(const SRectangle& rect1, const SRectangle& rect2);

private:
	std::list<SRectangle> rectangles;
	bool needsUpdate;

private:
	static unsigned statsTotalSize;
	static unsigned statsOptSize;
};

#endif // RECTANGLE_OPTIMIZER_H

