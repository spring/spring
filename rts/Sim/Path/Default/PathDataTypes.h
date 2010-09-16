/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATH_DATATYPES_HDR
#define PATH_DATATYPES_HDR

#include <queue>
#include <vector>

#include "PathConstants.h"
#include "System/Vec2.h"

// represents either a single square (PF) or a block of squares (PE)
struct PathNode {
	PathNode(): fCost(0.0f), gCost(0.0f), nodeNum(0), nodePos(0, 0) {
	}

	float fCost; // f
	float gCost; // g

	int nodeNum;
	int2 nodePos;

	inline bool operator <  (const PathNode& pn) const { return (fCost < pn.fCost); }
	inline bool operator >  (const PathNode& pn) const { return (fCost > pn.fCost); }
	inline bool operator == (const PathNode& pn) const { return (nodeNum == pn.nodeNum); }
};

struct PathNodeState {
	PathNodeState(): pathCost(PATHCOST_INFINITY), extraCostSynced(0.0f), extraCostUnsynced(0.0f), pathMask(0) {
		parentNodePos.x = -1;
		parentNodePos.y = -1;
	}

	float pathCost;
	// overlay-cost modifiers for nodes (while these are active,
	// calls to GetPath and GetNextWaypoint behave differently)
	//
	// <extraCostUnsynced> may not be read in synced context,
	// because AI's and unsynced Lua have write-access to it
	// <extraCostSynced> may not be written in unsynced context
	//
	// NOTE: if more than one local AI instance is active,
	// each must undo its changes or they will be visible
	// to the other AI's
	float extraCostSynced;
	float extraCostUnsynced;

	// combination of PATHOPT_{OPEN, ..., OBSOLETE} flags
	unsigned int pathMask;

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
		return (x->fCost > y->fCost);
	}
};



struct PathNodeBuffer {
public:
	PathNodeBuffer(): idx(0) {
		for (int i = 0; i < MAX_SEARCHED_NODES; ++i) {
			buffer[i] = PathNode();
		}
	}

	void SetSize(unsigned int i) { idx = i; }
	unsigned int GetSize() const { return idx; }

	const PathNode* GetNode(unsigned int i) const { return &buffer[i]; }
	      PathNode* GetNode(unsigned int i)       { return &buffer[i]; }

private:
	// index of the most recently added node
	unsigned int idx;

	PathNode buffer[MAX_SEARCHED_NODES];
};

struct PathNodeStateBuffer {
	PathNodeStateBuffer(const int2& bufRes, const int2& mapRes) {
		extraCostsSynced = NULL;
		extraCostsUnsynced = NULL;

		br.x = bufRes.x; ps.x = mapRes.x / bufRes.x;
		br.y = bufRes.y; ps.y = mapRes.y / bufRes.y;

		buffer.resize(br.x * br.y, PathNodeState());
	}

	void Clear() { buffer.clear(); }
	unsigned int GetSize() const { return buffer.size(); }

	const std::vector<PathNodeState>& GetBuffer() const { return buffer; }
	      std::vector<PathNodeState>& GetBuffer()       { return buffer; }

	const PathNodeState& operator [] (unsigned int idx) const { return buffer[idx]; }
	      PathNodeState& operator [] (unsigned int idx)       { return buffer[idx]; }

	// <xhm> and <zhm> are always passed in heightmap-coordinates
	float GetNodeExtraCost(unsigned int xhm, unsigned int zhm, bool synced) const {
		float c = 0.0f;

		if (synced) {
			if (extraCostsSynced != NULL) {
				// (mr / sr) is the synced downsample-factor
				c = extraCostsSynced[ (zhm / (mr.y / sr.y)) * sr.x  +  (xhm / (mr.x / sr.x)) ];
			} else {
				c = buffer[ (zhm / ps.y) * br.x  +  (xhm / ps.x) ].extraCostSynced;
			}
		} else {
			if (extraCostsUnsynced != NULL) {
				// (mr / ur) is the unsynced downsample-factor
				c = extraCostsUnsynced[ (zhm / (mr.y / ur.y)) * ur.x  +  (xhm / (mr.x / ur.x)) ];
			} else {
				c = buffer[ (zhm / ps.y) * br.x  +  (xhm / ps.x) ].extraCostUnsynced;
			}
		}

		return c;
	}

	const float* GetNodeExtraCosts(bool synced) const {
		if (synced) {
			return extraCostsSynced;
		} else {
			return extraCostsUnsynced;
		}
	}
	void SetNodeExtraCosts(const float* costs, unsigned int sx, unsigned int sz, bool synced) {
		if (synced) {
			extraCostsSynced = costs;

			sr.x = sx;
			sr.y = sz;
		} else {
			extraCostsUnsynced = costs;

			ur.x = sx;
			ur.y = sz;
		}
	}

private:
	std::vector<PathNodeState> buffer;

	// if non-NULL, these override PathNodeState::extraCost{Synced, Unsynced}
	// (NOTE: they can have arbitrary resolutions between 1 and gs->map{x,y})
	const float* extraCostsSynced;
	const float* extraCostsUnsynced;

	int2 ps; // patch size (eg. 1 for PF, BLOCK_SIZE for PE); ignored when extraCosts != NULL
	int2 br; // buffer resolution (equal to mr / ps); ignored when extraCosts != NULL
	int2 mr; // heightmap resolution (equal to gs->map{x,y})
	int2 sr; // extraCostsSynced resolution
	int2 ur; // extraCostsUnsynced resolution
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
