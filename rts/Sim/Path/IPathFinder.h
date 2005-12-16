#ifndef IPATHFINDER_HPP
#define IPATHFINDER_HPP

#include <vector>

struct MoveData;
class float3;
class CSolidObject;

//Cost constants.
const float PATHCOST_INFINITY = 1e9;
const float PATHCOST_BLOCKED = 1e6;

//Options
const unsigned int PATHOPT_IGNORE_GROUND_BLOCKING = 1;
const unsigned int PATHOPT_IGNORE_MOBILE_UNITS = 2;
const unsigned int PATHOPT_RESTRICTED_AREA = 4;

//Common constants
const unsigned int FOOT_MARGINAL = 2;

class IPathFinder {
public:
	enum SearchResult {
		Ok,
		OutOfRange,
		Blocked,
		Error
	};

	struct Path {
		std::vector<float3> path;
		float pathCost;
	};


	/*
	Trying to find an optimal path between the two given positions.

	Param:
		searchingUnit
			The unit searching the path.

		start
			The location from which the search is started. (x,y,z)

		goal
			The destination to be reached from the start-location.

		path
			A vector to be filled with the eventually found path.
			This vector contains the nodes of the path, with the start node not included
			and with no specified length of the vertexes.
			If no path could be found the returned path shall be of size() == 0.
	Return:
		A SearchResult indicating the success or failure-type of the search.
	*/
	virtual SearchResult GetPath(MoveData *moveData, float3 start, float3 goal, float goalRadius, Path& path, unsigned int options = 0, unsigned int maxSearchedNodes = 10000) = 0;


	/*
	Checks if an position would be classified as accessable by the algorithm.
	The function could also be sat to modify the given position to an accessable
	nearby position, in case the exact one is blocked.
	In case no alternative position could be found Blocked is returned and
	pos stay unchanged.
	Note: The result of this check may change during game for some algorithms.

	Param:
		unit
			The unit that should be able to access the position.

		pos
			The exakt position to be checked. (x,y,z)
			Could be changed by function.

		modify
			If the function is allowed to suggest and change the exakt position
			into an accessable one, in case the exakt one is blocked.

	Return:
		SearchResult::Ok or SearchResult::Blocked.
	*/
	virtual SearchResult CheckPosition(MoveData* moveData, float3& pos, bool modify = false, unsigned int options = 0) = 0;


	/*
	Tests if a path is free from blocking objects.

	Param:
		searchingUnit
			The unit that shall use the path.

		path
			A vector containing the path to be tested.

		blockingObjects
			Fills with all objects found blocking the path.
			If no object where found, then will be of size() == 0.
	*/
	virtual SearchResult CheckPath(MoveData* moveData, Path& path, std::vector<CSolidObject*>& blockingObjects) = 0;


	/*
	To be called when changes in map structure have occured.
	Gives the pathfinder a chance to update it's information about the world.

	Param:
		x1, z1, x2, z2
			Two opposit corners giving a square including all changes.
			Given in square-coordinates.
	*/
	virtual void MapChanged(unsigned int x1, unsigned int z1, unsigned int x2, unsigned int z2) = 0;
};


#endif
