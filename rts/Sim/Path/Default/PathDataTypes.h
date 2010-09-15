/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATH_DATATYPES_HDR
#define PATH_DATATYPES_HDR

#include <queue>
#include <vector>

#include "PathConstants.h"
#include "System/Vec2.h"

// represents either a single square (PF) or a block of squares (PE)
struct PathNode {
	PathNode(): cost(0.0f), currentCost(0.0f), nodeNum(0), nodePos(0, 0) {
	}

	float cost;
	float currentCost;

	int nodeNum;
	int2 nodePos;

	inline bool operator <  (const PathNode& pn) const { return (cost < pn.cost); }
	inline bool operator >  (const PathNode& pn) const { return (cost > pn.cost); }
	inline bool operator == (const PathNode& pn) const { return (nodeNum == pn.nodeNum); }
};

struct PathNodeState {
	PathNodeState(): cost(PATHCOST_INFINITY), mask(0) {
		parentNodePos.x = -1;
		parentNodePos.y = -1;
	}

	float cost;
	// combination of PATHOPT_* flags
	unsigned int mask;

	// needed for the PE to back-track path to goal
	int2 parentNodePos;
	// for the PE, each node (block) maintains the
	// best accessible offset (from its own center
	// position) with respect to each movetype
	std::vector<int2> nodeOffsets;
};

// functor to define node priority
struct lessCost: public std::binary_function<PathNode*, PathNode*, bool> {
	inline bool operator() (const PathNode* x, const PathNode* y) const {
		return (x->cost > y->cost);
	}
};

struct PathNodeBuffer {
public:
	PathNodeBuffer(): idx(0) {
		for (int i = 0; i < MAX_SEARCHED_NODES; ++i) {
			nodeBuffer[i] = PathNode();
		}
	}

	void SetSize(unsigned int i) { idx = i; }
	unsigned int GetSize() const { return idx; }

	const PathNode* GetNode(unsigned int i) const { return &nodeBuffer[i]; }
	PathNode* GetNode(unsigned int i) { return &nodeBuffer[i]; }

private:
	// index of the most recently added node
	unsigned int idx;

	PathNode nodeBuffer[MAX_SEARCHED_NODES];
};



// looks like a std::vector, but holds a fixed-size buffer
// used as a backing array for the PathPriorityQueue dtype
class PathVector {
public:
	typedef int size_type;
	typedef PathNode* value_type;
	typedef PathNode* reference;
	typedef PathNode** iterator;
	typedef const PathNode* const_reference;
	typedef const PathNode* const* const_iterator;

	// gcc 4.3 requires concepts, so provide them
		value_type& operator [] (size_type idx) { return buf[idx]; }
		const value_type& operator [] (size_type idx) const { return buf[idx]; }

		typedef iterator pointer;
		typedef const_iterator const_pointer;
		typedef int difference_type;

		typedef PathNode** reverse_iterator;
		typedef const PathNode* const* const_reverse_iterator;

		// FIXME: don't ever use these
		reverse_iterator rbegin() { return 0; }
		reverse_iterator rend() { return 0; }
		const_reverse_iterator rbegin() const { return 0; }
		const_reverse_iterator rend() const { return 0; }
		PathVector(int, const value_type&) { abort(); }
		PathVector(iterator, iterator) { abort(); }
		void insert(iterator, const value_type&) { abort(); }
		void insert(iterator, const size_type&, const value_type&) { abort(); }
		void insert(iterator, iterator, iterator) { abort(); }
		void erase(iterator, iterator) { abort(); }
		void erase(iterator) { abort(); }
		void erase(iterator, iterator, iterator) { abort(); }
		void swap(PathVector&) { abort(); }
	// end of concept hax

	PathVector(): bufPos(-1) {}

	inline void push_back(PathNode* os) { buf[++bufPos] = os; }
	inline void pop_back() { --bufPos; }
	inline PathNode* back() const { return buf[bufPos]; }
	inline const value_type& front() const { return buf[0]; }
	inline value_type& front() { return buf[0]; }
	inline bool empty() const { return (bufPos < 0); }
	inline size_type size() const { return bufPos + 1; }
	inline size_type max_size() const { return (1 << 30); }
	inline iterator begin() { return &buf[0]; }
	inline iterator end() { return &buf[bufPos + 1]; }
	inline const_iterator begin() const { return &buf[0]; }
	inline const_iterator end() const { return &buf[bufPos + 1]; }
	inline void clear() { bufPos = -1; }

private:
	int bufPos;

	PathNode* buf[MAX_SEARCHED_NODES];
};

class PathPriorityQueue: public std::priority_queue<PathNode*, PathVector, lessCost> {
public:
	// faster than "while (!q.empty()) { q.pop(); }"
	void Clear() { c.clear(); }
};

#endif
