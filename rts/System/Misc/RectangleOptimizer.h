/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RECTANGLE_OPTIMIZER_H
#define RECTANGLE_OPTIMIZER_H

#ifdef RO_USE_DEQUE_CONTAINER
#include <deque>
#else
#include <list>
#endif

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
	CR_DECLARE_STRUCT(CRectangleOptimizer)

public:
	CRectangleOptimizer();
	~CRectangleOptimizer();

	void Optimize();

	unsigned GetTotalArea() const;

public:
	//! std container funcs
	#ifdef RO_USE_DEQUE_CONTAINER
	typedef std::deque<SRectangle> container;
	#else
	typedef std::list<SRectangle> container;
	#endif
	typedef container::iterator iterator;
	typedef container::const_iterator const_iterator;

	bool empty() const { return rectangles.empty(); }
	size_t size() const { return rectangles.size(); }

	SRectangle& front() { return rectangles.front(); }
	SRectangle& back() { return rectangles.back(); }

	void pop_front() { return rectangles.pop_front(); }
	void pop_back() { return rectangles.pop_back(); }

	void swap(CRectangleOptimizer& other) {
		rectangles.swap(other.rectangles);
		std::swap(needsUpdate, other.needsUpdate);
		other.maxAreaPerRect = maxAreaPerRect; // intentional one-way copy here
	}
	void splice(iterator pos, CRectangleOptimizer& other) {
		needsUpdate = other.needsUpdate || !rectangles.empty();
		#ifdef RO_USE_DEQUE_CONTAINER
		while (!other.empty()) {
			pos = rectangles.insert(pos, other.back());
			other.pop_back();
		}
		#else
		rectangles.splice(pos, other.rectangles);
		#endif
	}
	void clear() {
		needsUpdate = false;
		rectangles.clear();
	}
	void push_back(const SRectangle& rect) {
		//! skip empty/negative rectangles
		//assert(rect.GetWidth() > 0 && rect.GetHeight() > 0);
		if (rect.GetWidth() <= 0 || rect.GetHeight() <= 0)
			return;
		needsUpdate = true;
		rectangles.push_back(rect);
	}
	void swap(container& dq) {
		rectangles.swap(dq);
		needsUpdate = !rectangles.empty();
	}

	const_iterator cbegin() { return rectangles.cbegin(); }
	const_iterator cend() { return rectangles.cend(); }

	iterator begin() { return rectangles.begin(); }
	iterator end() { return rectangles.end(); }

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
	container rectangles;
	bool needsUpdate;

private:
	static unsigned statsTotalSize;
	static unsigned statsOptSize;
};

#endif // RECTANGLE_OPTIMIZER_H

