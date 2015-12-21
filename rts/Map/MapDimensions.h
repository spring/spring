/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MAPDIMENSIONS_H
#define MAPDIMENSIONS_H

#include <cassert>
#include "System/bitops.h"
#include "System/creg/creg_cond.h"

struct MapDimensions {
public:
	CR_DECLARE_STRUCT(MapDimensions)

	MapDimensions()
	{
		// set in SMFReadMap::ParseHeader
		mapx   = 0;
		mapxm1 = 0;
		mapxp1 = 0;

		mapy   = 0;
		mapym1 = 0;
		mapyp1 = 0;

		hmapx  = 0;
		hmapy  = 0;

		pwr2mapx = 0;
		pwr2mapy = 0;

		mapSquares = 0;
	}

	void Initialize()
	{
		assert(mapx != 0);
		assert(mapy != 0);

		mapxm1 = mapx - 1;
		mapxp1 = mapx + 1;
		mapym1 = mapy - 1;
		mapyp1 = mapy + 1;

		hmapx = mapx >> 1;
		hmapy = mapy >> 1;

		pwr2mapx = next_power_of_2(mapx);
		pwr2mapy = next_power_of_2(mapy);

		mapSquares = mapx * mapy;
	}


	/**
	* @brief map x
	*
	* The map's number of squares in the x direction
	* (note that the number of vertices is one more)
	*/
	int mapx;
	int mapxm1; // mapx minus one
	int mapxp1; // mapx plus one

	/**
	* @brief map y
	*
	* The map's number of squares in the y direction
	*/
	int mapy;
	int mapym1; // mapy minus one
	int mapyp1; // mapy plus one


	/**
	* @brief half map x
	*
	* Contains half of the number of squares in the x direction
	*/
	int hmapx;

	/**
	* @brief half map y
	*
	* Contains half of the number of squares in the y direction
	*/
	int hmapy;


	/**
	* @brief map x power of 2
	*
	* Map's size in the x direction rounded
	* up to the next power of 2
	*/
	int pwr2mapx;

	/**
	* @brief map y power of 2
	*
	* Map's size in the y direction rounded
	* up to the next power of 2
	*/
	int pwr2mapy;


	/**
	* @brief map squares
	*
	* Total number of squares on the map
	*/
	int mapSquares;
};

#endif

