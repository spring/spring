/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATH_DATATYPES_H
#define PATH_DATATYPES_H

#include <queue>
#include <vector>
#include <algorithm> // for std::fill

#include "PathConstants.h"
#include "System/type2.h"
#include <boost/cstdint.hpp>

/// represents either a single square (PF) or a block of squares (PE)
struct PathNode {
	PathNode()
		: fCost(0.0f)
		, gCost(0.0f)
		, nodeNum(0)
		, nodePos(0, 0)
	{}

	float fCost;
	float gCost;

	int nodeNum;
	ushort2 nodePos;

	inline bool operator <  (const PathNode& pn) const { return (fCost < pn.fCost); }
	inline bool operator >  (const PathNode& pn) const { return (fCost > pn.fCost); }
	inline bool operator == (const PathNode& pn) const { return (nodeNum == pn.nodeNum); }
};


/// functor to define node priority
struct lessCost: public std::binary_function<PathNode*, PathNode*, bool> {
	inline bool operator() (const PathNode* x, const PathNode* y) const {
		return (x->fCost ==  y->fCost) ? (x->gCost > y->gCost) : (x->fCost > y->fCost);
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
	/// index of the most recently added node
	unsigned int idx;

	PathNode buffer[MAX_SEARCHED_NODES];
};


struct PathNodeStateBuffer {
	PathNodeStateBuffer(const int2& bufRes, const int2& mapRes)
		: extraCostsOverlaySynced(NULL)
		, extraCostsOverlayUnsynced(NULL)
		, ps(mapRes / bufRes)
		, br(bufRes)
		, mr(mapRes)
	{
		fCost.resize(br.x * br.y, PATHCOST_INFINITY);
		gCost.resize(br.x * br.y, PATHCOST_INFINITY);
		nodeMask.resize(br.x * br.y, 0);

		// create on-demand
		//extraCostSynced.resize(br.x * br.y, 0.0f);
		//extraCostUnsynced.resize(br.x * br.y, 0.0f);

		// Note: Full resolution buffer does not need those!
		if (bufRes != mapRes) {
			//peNodeOffsets.resize(); is done in PathEstimator
		}

		maxCosts[NODE_COST_F] = 0.0f;
		maxCosts[NODE_COST_G] = 0.0f;
		maxCosts[NODE_COST_H] = 0.0f;
	}

	unsigned int GetSize() const { return fCost.size(); }

	void ClearSquare(int idx) {
		//assert(idx>=0 && idx<fCost.size());
		fCost[idx] = PATHCOST_INFINITY;
		gCost[idx] = PATHCOST_INFINITY;
		nodeMask[idx] &= PATHOPT_OBSOLETE; // clear all except PATHOPT_OBSOLETE
	}


	/// size of the memory-region we hold allocated (excluding sizeof(*this))
	unsigned int GetMemFootPrint() const {
		unsigned int memFootPrint = 0;

		if (!peNodeOffsets.empty()) {
			memFootPrint += (peNodeOffsets.size() * (sizeof(std::vector<short2>) + peNodeOffsets[0].size() * sizeof(short2)));
		}

		memFootPrint += (nodeMask.size() * sizeof(boost::uint8_t));
		memFootPrint += ((fCost.size() + gCost.size()) * sizeof(float));
		memFootPrint += ((extraCostSynced.size() + extraCostUnsynced.size()) * sizeof(float));

		return memFootPrint;
	}

	void SetMaxCost(unsigned int t, float c) { maxCosts[t] = c; }
	float GetMaxCost(unsigned int t) const { return maxCosts[t]; }

	/// {@param xhm} and {@param zhm} are always passed in heightmap-coordinates
	float GetNodeExtraCost(unsigned int xhm, unsigned int zhm, bool synced) const {
		float c = 0.0f;

		if (synced) {
			if (extraCostsOverlaySynced != NULL) {
				// (mr / sr) is the synced downsample-factor
				c = extraCostsOverlaySynced[ (zhm / (mr.y / sr.y)) * sr.x  +  (xhm / (mr.x / sr.x)) ];
			} else if (!extraCostSynced.empty()) {
				c = extraCostSynced[ (zhm / ps.y) * br.x  +  (xhm / ps.x) ];
			}
		} else {
			if (extraCostsOverlayUnsynced != NULL) {
				// (mr / ur) is the unsynced downsample-factor
				c = extraCostsOverlayUnsynced[ (zhm / (mr.y / ur.y)) * ur.x  +  (xhm / (mr.x / ur.x)) ];
			} else if (!extraCostUnsynced.empty()) {
				c = extraCostUnsynced[ (zhm / ps.y) * br.x  +  (xhm / ps.x) ];
			}
		}

		return c;
	}

	const float* GetNodeExtraCosts(bool synced) const {
		if (synced) {
			return extraCostsOverlaySynced;
		} else {
			return extraCostsOverlayUnsynced;
		}
	}

	void SetNodeExtraCost(unsigned int xhm, unsigned int zhm, float cost, bool synced) {
		if (synced) {
			if (extraCostSynced.empty())
				extraCostSynced.resize(br.x * br.y, 0.0f); // alloc on-demand

			extraCostSynced[ (zhm / ps.y) * br.x  +  (xhm / ps.x) ] = cost;
		} else {
			if (extraCostUnsynced.empty())
				extraCostUnsynced.resize(br.x * br.y, 0.0f); // alloc on-demand

			extraCostUnsynced[ (zhm / ps.y) * br.x  +  (xhm / ps.x) ] = cost;
		}
	}

	void SetNodeExtraCosts(const float* costs, unsigned int sx, unsigned int sz, bool synced) {
		if (synced) {
			extraCostsOverlaySynced = costs;

			sr.x = sx;
			sr.y = sz;
		} else {
			extraCostsOverlayUnsynced = costs;

			ur.x = sx;
			ur.y = sz;
		}
	}

public:
	std::vector<float> fCost;
	std::vector<float> gCost;

	/// bitmask of PATHOPT_{OPEN, ..., OBSOLETE} flags
	std::vector<boost::uint8_t> nodeMask;
#if !defined(_MSC_FULL_VER) || _MSC_FULL_VER > 180040000 // ensure that ::max() is constexpr
	static_assert(PATHOPT_SIZE <= std::numeric_limits<boost::uint8_t>::max(), "nodeMask basic type to small to hold bitmask of PATHOPT");
#endif
	/// for the PE, maintains an array of the
	/// best accessible offset (from its own center
	/// position)
	/// peNodeOffsets[pathType][blockIdx]
	std::vector< std::vector<short2> > peNodeOffsets;

private:
	// overlay-cost modifiers for nodes (when non-zero, these
	// modify the behavior of GetPath() and GetNextWaypoint())
	//
	// <extraCostUnsynced> may not be read in synced context,
	// because AI's and unsynced Lua have write-access to it
	// <extraCostSynced> may not be written in unsynced context
	//
	// NOTE: if more than one local AI instance is active,
	// each must undo its changes or they will be visible
	// to the other AI's
	// NOTE: don't make public cause we create those on-demand
	std::vector<float> extraCostSynced;
	std::vector<float> extraCostUnsynced;

	// if non-NULL, these override PathNodeState::extraCost{Synced, Unsynced}
	// (NOTE: they can have arbitrary resolutions between 1 and mapDims.map{x,y})
	const float* extraCostsOverlaySynced;
	const float* extraCostsOverlayUnsynced;

private:
	float maxCosts[3];

	int2 ps; ///< patch size (eg. 1 for PF, BLOCK_SIZE for PE); ignored when extraCosts != NULL
	int2 br; ///< buffer resolution (equal to mr / ps); ignored when extraCosts != NULL
	int2 mr; ///< heightmap resolution (equal to mapDims.map{x,y})
	int2 sr; ///< extraCostsSynced resolution
	int2 ur; ///< extraCostsUnsynced resolution
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
		PathVector(int, const value_type&): bufPos(-1) { abort(); }
		PathVector(iterator, iterator): bufPos(-1) { abort(); }
		void insert(iterator, const value_type&) { abort(); }
		void insert(iterator, const size_type&, const value_type&) { abort(); }
		void insert(iterator, iterator, iterator) { abort(); }
		void erase(iterator, iterator) { abort(); }
		void erase(iterator) { abort(); }
		void erase(iterator, iterator, iterator) { abort(); }
		void swap(PathVector&) { abort(); }
	// end of concept hax

	PathVector(): bufPos(-1) {
#ifdef DEBUG
		// only do this in DEBUG builds for performance reasons
		// it could help finding logic errors
		std::fill(std::begin(buf), std::end(buf), nullptr);
#endif
	}

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
	/// faster than "while (!q.empty()) { q.pop(); }"
	void Clear() { c.clear(); }
};

#endif // PATH_DATATYPES_H
