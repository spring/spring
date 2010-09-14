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
	IPath::SearchResult GetPath(const MoveData& moveData, const float3 startPos,
	                     const CPathFinderDef& pfDef, IPath::Path& path,
	                     bool testMobile, bool exactPath,
	                     unsigned int maxSearchedNodes, bool needPath,
	                     int ownerId = 0);

	IPath::SearchResult GetPath(const MoveData& moveData, const std::vector<float3>& startPos,
	                     const CPathFinderDef& pfDef, IPath::Path& path, int ownerId = 0);

	enum PATH_OPTIONS {
		PATHOPT_RIGHT     =   1,      //-x
		PATHOPT_LEFT      =   2,      //+x
		PATHOPT_UP        =   4,      //+z
		PATHOPT_DOWN      =   8,      //-z
		PATHOPT_DIRECTION = (PATHOPT_RIGHT | PATHOPT_LEFT | PATHOPT_UP | PATHOPT_DOWN),
		PATHOPT_START     =  16,
		PATHOPT_OPEN      =  32,
		PATHOPT_CLOSED    =  64,
		PATHOPT_FORBIDDEN = 128,
		PATHOPT_BLOCKED   = 256,
	};

public:
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

	void UpdateHeatValue(const int& x, const int& y, const int& value, const int& ownerId)
	{
		const int i = GetHeatMapIndex(x, y);
		if (heatmap[i].value < value + heatMapOffset) {
			heatmap[i].value = value + heatMapOffset;
			heatmap[i].ownerId = ownerId;
		}
	}

	const int GetHeatOwner(const int& x, const int& y)
	{
		const int i = GetHeatMapIndex(x, y);
		return heatmap[i].ownerId;
	}

	const int GetHeatValue(const int& x, const int& y)
	{
		const int i = GetHeatMapIndex(x, y);
		return std::max(0, heatmap[i].value - heatMapOffset);
	}

private:
	// Heat mapping
	int GetHeatMapIndex(int x, int y);

	struct HeatMapValue {
		HeatMapValue(): value(0), ownerId(0) {}
		int value;
		int ownerId;
	};

	std::vector<HeatMapValue> heatmap; //! resolution is hmapx*hmapy
	int heatMapOffset;  //! heatmap values are relative to this
	bool heatMapping;

private:
	struct SquareState {
		SquareState(): status(0), cost(PATHCOST_INFINITY) {
		}

		unsigned int status;
		float cost;
	};



	void ResetSearch();
	IPath::SearchResult InitSearch(const MoveData& moveData, const CPathFinderDef& pfDef, int ownerId);
	IPath::SearchResult DoSearch(const MoveData& moveData, const CPathFinderDef& pfDef, int ownerId);
	bool TestSquare(
		const MoveData& moveData,
		const CPathFinderDef& pfDef,
		const PathNode* parentOpenSquare,
		unsigned int enterDirection,
		int ownerId
	);
	void FinishSearch(const MoveData& moveData, IPath::Path& path);
	void AdjustFoundPath(const MoveData& moveData, IPath::Path& path, float3& nextPoint,
			std::deque<int2>& previous, int2 square);

	unsigned int maxNodesToBeSearched;

	std::vector<SquareState> squareStates;			///< Map of all squares on map.
	std::vector<int> dirtySquares;					///< Squares tested by search.

	int2 directionVector[16];						///< Unit square-movement in given direction.
	float moveCost[16];								///< The cost of moving in given direction.

	float3 start;
	int startxSqr, startzSqr;
	int startSquare;

	int goalSquare;					///< Is sat during the search as the square closest to the goal.
	float goalHeuristic;			///< The heuristic value of goalSquare.

	bool exactPath;
	bool testMobile;
	bool needPath;

	// Statistic
	unsigned int testedNodes;

	PathNodeBuffer openSquareBuffer;
	PathPriorityQueue openSquares;
};

#endif // PATHFINDER_H
