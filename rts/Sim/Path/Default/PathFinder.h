/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATH_FINDER_H
#define PATH_FINDER_H

#include <list>
#include <queue>
#include <cstdlib>

#include "IPath.h"
#include "PathConstants.h"
#include "PathDataTypes.h"
#include "Sim/Objects/SolidObject.h"

struct MoveDef;
class CPathFinderDef;


class CPathFinder {
public:
	CPathFinder();
	~CPathFinder();

	void* operator new(size_t size);
	void operator delete(void* p, size_t size);

	/**
	 * Gives a detailed path from given starting location to target defined in
	 * CPathFinderDef, whenever any such are available.
	 * If no complete path was found, any path leading as "close" to target as
	 * possible will be created, and SearchResult::OutOfRange will be returned.
	 * Only when no "closer" position than the given starting location could be
	 * found no path is created, and SearchResult::CantGetCloser is returned.
	 * Path resolution: 2*SQUARE_SIZE
	 *
	 * @param moveDef defining the footprint of the unit requesting the path.
	 * @param startPos The starting location of the path. (Projected onto (x,z))
	 * @param pfDef Object defining the target/goal of the search.
	 *   Could also be used to put constraints on the searchspace used.
	 * @param path If any path could be found, it will be generated and put into
	 *   this structure.
	 * @param exactPath Overrides the return of the "closest" path.
	 *   If this option is true, a path is returned only if it's completed all
	 *   the way to the goal defined in pfDef. All SearchResult::OutOfRange are
	 *   then turned into SearchResult::CantGetCloser.
	 * @param maxSearchedNodes The maximum number of nodes/squares the search
	 *   are allowed to analyze. This restriction could be used in cases where
	 *   CPU-consumption are critical.
	 */
	IPath::SearchResult GetPath(
		const MoveDef& moveDef,
		const float3& startPos,
		const CPathFinderDef& pfDef,
		IPath::Path& path,
		bool testMobile,
		bool exactPath,
		unsigned int maxSearchedNodes,
		bool needPath,
		const CSolidObject* owner,
		bool synced
	);



	/**
	 * @brief Enable/disable heat mapping.
	 *
	 * Heat mapping makes the pathfinder value unused paths more. Less path overlap should
	 * make units behave more intelligently.
	 */
	void InitHeatMap();
	void SetHeatMapState(bool enabled);
	bool GetHeatMapState() { return heatMapping; }
	void UpdateHeatMap();

	void UpdateHeatValue(const int x, const int y, const int value, const CSolidObject* owner)
	{
		const int i = GetHeatMapIndex(x, y);
		if (heatmap[i].value < value + heatMapOffset) {
			heatmap[i].value = value + heatMapOffset;
			heatmap[i].ownerId = ((owner != NULL) ? owner->id : -1);
		}
	}

	const int GetHeatOwner(const int x, const int y)
	{
		const int i = GetHeatMapIndex(x, y);
		return heatmap[i].ownerId;
	}

	const int GetHeatValue(const int x, const int y)
	{
		const int i = GetHeatMapIndex(x, y);
		return std::max(0, heatmap[i].value - heatMapOffset);
	}


	// size of the memory-region we hold allocated (excluding sizeof(*this))
	unsigned int GetMemFootPrint() const { return ((heatmap.size() * sizeof(HeatMapValue)) + squareStates.GetMemFootPrint()); }

	PathNodeStateBuffer& GetNodeStateBuffer() { return squareStates; }

private:
	// Heat mapping
	int GetHeatMapIndex(int x, int y);

	/**
	 * Clear things up from last search.
	 */
	void ResetSearch();
	/// Set up the starting point of the search.
	IPath::SearchResult InitSearch(const MoveDef& moveDef, const CPathFinderDef& pfDef, const CSolidObject* owner, bool synced);
	/// Performs the actual search.
	IPath::SearchResult DoSearch(const MoveDef& moveDef, const CPathFinderDef& pfDef, const CSolidObject* owner, bool synced);
	/**
	 * Test the availability and value of a square,
	 * and possibly add it to the queue of open squares.
	 */
	bool TestSquare(
		const MoveDef& moveDef,
		const CPathFinderDef& pfDef,
		const PathNode* parentOpenSquare,
		unsigned int enterDirection,
		const CSolidObject* owner,
		bool synced
	);
	/**
	 * Recreates the path found by pathfinder.
	 * Starting at goalSquare and tracking backwards.
	 *
	 * Perform adjustment of waypoints so not all turns are 90 or 45 degrees.
	 */
	void FinishSearch(const MoveDef&, IPath::Path&);
	/**
	 * Adjusts the found path to cut corners where possible.
	 */
	void AdjustFoundPath(
		const MoveDef&,
		IPath::Path&,
		float3& nextPoint,
		std::deque<int2>& previous,
		int2 square
	);

	struct HeatMapValue {
		HeatMapValue()
			: value(0)
			, ownerId(0)
		{}
		int value;
		int ownerId;
	};

	std::vector<HeatMapValue> heatmap; ///< resolution is hmapx*hmapy
	int heatMapOffset;                 ///< heatmap values are relative to this
	bool heatMapping;

	int2 dirVectors2D[16];             ///< Unit square-movement in given direction.
	float3 dirVectors3D[16];
	float moveCost[16];                ///< The cost of moving in given direction.

	float3 start;
	int startxSqr;
	int startzSqr;
	int startSquare;

	int goalSquare;                    ///< Is sat during the search as the square closest to the goal.
	float goalHeuristic;               ///< The heuristic value of goalSquare.

	bool exactPath;
	bool testMobile;
	bool needPath;

	unsigned int maxSquaresToBeSearched;
	unsigned int testedNodes;
	float maxNodeCost;

	PathNodeBuffer openSquareBuffer;
	PathNodeStateBuffer squareStates;
	PathPriorityQueue openSquares;

	std::vector<int> dirtySquares;     ///< Squares tested by search.
};

#endif // PATH_FINDER_H
