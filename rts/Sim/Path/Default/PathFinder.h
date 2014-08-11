/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATH_FINDER_H
#define PATH_FINDER_H

#include <list>
#include <vector>
#include <deque>

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
	 * @param maxNodes The maximum number of nodes / squares the search is
	 *   allowed to analyze. This restriction could be used in cases where
	 *   CPU-consumption is critical.
	 */
	IPath::SearchResult GetPath(
		const MoveDef& moveDef,
		const CPathFinderDef& pfDef,
		const CSolidObject* owner,
		const float3& startPos,
		IPath::Path& path,
		unsigned int maxNodes,
		bool testMobile,
		bool exactPath,
		bool needPath,
		bool peCall,
		bool synced
	);


	// size of the memory-region we hold allocated (excluding sizeof(*this))
	// (PathManager stores HeatMap and FlowMap, so we do not need to add them)
	unsigned int GetMemFootPrint() const { return (blockStates.GetMemFootPrint()); }

	PathNodeStateBuffer& GetNodeStateBuffer() { return blockStates; }

	static void InitDirectionVectorsTable();
	static void InitDirectionCostsTable();

	static const   int2* GetDirectionVectorsTable2D();
	static const float3* GetDirectionVectorsTable3D();

protected: // IPathFinder impl
	/// Clear things up from last search.
	void ResetSearch();
	/// Set up the starting point of the search.
	IPath::SearchResult InitSearch(const MoveDef& moveDef, const CPathFinderDef& pfDef, const CSolidObject* owner, bool peCall, bool synced);
	/// Performs the actual search.
	IPath::SearchResult DoSearch(const MoveDef& moveDef, const CPathFinderDef& pfDef, const CSolidObject* owner, bool peCall, bool synced);

	/**
	 * Test the availability and value of a square,
	 * and possibly add it to the queue of open squares.
	 */
	bool TestBlock(
		const MoveDef& moveDef,
		const CPathFinderDef& pfDef,
		const PathNode* parentSquare,
		const CSolidObject* owner,
		const unsigned int pathOptDir,
		const unsigned int blockStatus,
		float speedMod,
		bool withinConstraints,
		bool synced
	);
	/**
	 * Recreates the path found by pathfinder.
	 * Starting at goalSquare and tracking backwards.
	 *
	 * Perform adjustment of waypoints so not all turns are 90 or 45 degrees.
	 */
	void FinishSearch(const MoveDef&, IPath::Path&) const;

private:
	void TestNeighborSquares(
		const MoveDef& moveDef,
		const CPathFinderDef& pfDef,
		const PathNode* parentSquare,
		const CSolidObject* owner,
		bool synced
	);

	/**
	 * Adjusts the found path to cut corners where possible.
	 */
	void AdjustFoundPath(
		const MoveDef&,
		IPath::Path&,
		float3& nextPoint,
		std::deque<int2>& previous,
		int2 square
	) const;

	bool exactPath;
	bool testMobile;
	bool needPath;

private: //IPathFinder stuff
	int2 mStartBlock;

	unsigned int mStartBlockIdx;
	unsigned int mGoalBlockIdx;  ///< set during each search as the square closest to the goal
	float mGoalHeuristic;        ///< heuristic value of goalSquareIdx

	unsigned int maxBlocksToBeSearched;
	unsigned int testedBlocks;

	PathNodeBuffer openBlockBuffer;
	PathNodeStateBuffer blockStates;
	PathPriorityQueue openBlocks;

	std::vector<unsigned int> dirtyBlocks;         ///< Squares tested by search.
};

#endif // PATH_FINDER_H
