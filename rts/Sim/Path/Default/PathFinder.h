/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATHFINDER_H
#define PATHFINDER_H

#include <list>
#include <queue>
#include <cstdlib>

#include "IPath.h"
#include "PathConstants.h"
#include "PathDataTypes.h"



struct MoveData;
class CPathFinderDef;

class CPathFinder {
public:
	CPathFinder();
	~CPathFinder();

#if !defined(USE_MMGR)
	void* operator new(size_t size);
	void operator delete(void* p, size_t size);
#endif

	/**
	Gives a detailed path from given starting location to target defined in CPathFinderDef,
	whenever any such are available.
	If no complete path was found, any path leading as "close" to target as possible will be
	created, and SearchResult::OutOfRange will be returned.
	Only when no "closer" position than the given starting location could be found no path
	is created, and SearchResult::CantGetCloser is returned.
	Path resolution: 2*SQUARE_SIZE
	Params:
		moveData
			MoveData defining the footprint of the unit requesting the path.

		startPos
			The starting location of the path. (Projected onto (x,z).)

		pfDef
			Object defining the target/goal of the search.
			Could also be used to put constraints on the searchspace used.

		path
			If any path could be found, it will be generated and put into this structure.

		moveMathOpt
			MoveMath options. Used to tell what types of objects that could be considered
			as blocking objects.

		exactPath
			Overrides the return of the "closest" path. If this option is true, a path is
			returned only if it's completed all the way to the goal defined in pfDef.
			All SearchResult::OutOfRange are then turned into SearchResult::CantGetCloser.

		maxSearchedNodes
			The maximum number of nodes/squares the search are allowed to analyze.
			This restriction could be used in cases where CPU-consumption are critical.
	*/
	IPath::SearchResult GetPath(
		const MoveData& moveData,
		const float3& startPos,
		const CPathFinderDef&,
		IPath::Path&,
		bool testMobile,
		bool exactPath,
		unsigned int maxSearchedNodes,
		bool needPath,
		int ownerId,
		bool synced
	);

	IPath::SearchResult GetPath(
		const MoveData&,
		const std::vector<float3>&,
		const CPathFinderDef&,
		IPath::Path&,
		int ownerId,
		bool synced
	);


	PathNodeStateBuffer& GetNodeStateBuffer() { return squareStates; }

private:
	void ResetSearch();
	IPath::SearchResult InitSearch(const MoveData&, const CPathFinderDef&, int ownerId, bool synced);
	IPath::SearchResult DoSearch(const MoveData&, const CPathFinderDef&, int ownerId, bool synced);
	bool TestSquare(
		const MoveData& moveData,
		const CPathFinderDef& pfDef,
		const PathNode* parentOpenSquare,
		unsigned int pathOpt,
		int ownerId,
		bool synced
	);
	void FinishSearch(const MoveData&, IPath::Path&);
	void AdjustFoundPath(
		const MoveData&,
		IPath::Path&,
		float3& nextPoint,
		std::deque<int2>& previous,
		int2 square
	);


	int2 directionVectors[PATH_DIRECTIONS << 1];
	float directionCosts[PATH_DIRECTIONS << 1];

	float3 start;
	int startxSqr, startzSqr;
	int startSquare;

	int goalSquare;									///< Is sat during the search as the square closest to the goal.
	float goalHeuristic;							///< The heuristic value of goalSquare.

	bool exactPath;
	bool testMobile;
	bool needPath;

	unsigned int maxSquaresToBeSearched;
	unsigned int testedNodes;
	float maxNodeCost;

	PathNodeBuffer openSquareBuffer;
	PathNodeStateBuffer squareStates;
	PathPriorityQueue openSquares;

	std::vector<int> dirtySquares;					///< Squares tested by search.
};

#endif // PATHFINDER_H
