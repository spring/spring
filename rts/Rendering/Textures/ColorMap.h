/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COLOR_MAP_H
#define COLOR_MAP_H

#include <string>
#include <vector>
#include <map>
#include <deque>

#include "System/creg/creg_cond.h"
#include "System/Color.h"

/**
 * Simple class to interpolate between 32bit RGBA colors
 * Do not delete an instance of this class created by any Load function,
 * they are deleted automaticly.
 */
class CColorMap
{
	CR_DECLARE_STRUCT(CColorMap)
public:
	CColorMap();
	/// Loads from a float vector
	CColorMap(const std::vector<float>& vec);
	/// Loads from a file
	CColorMap(const std::string& fileName);

public:
	/**
	 * @param color buffer with room for 4 bytes
	 * @param pos value between 0.0f and 1.0f, returns pointer to color
	 */
	void GetColor(unsigned char* color, float pos);

public:
	/// Load colormap from a bitmap
	static CColorMap* LoadFromBitmapFile(const std::string& fileName);

	/// Takes a vector with float value 0.0-1.0, 4 values makes up a rgba color
	static CColorMap* LoadFromFloatVector(const std::vector<float>& vec);

	/**
	 * Load from a string containing a number of float values or filename.
	 * example: "1.0 0.5 1.0 ... "
	 */
	static CColorMap* LoadFromDefString(const std::string& dString);

protected:
	std::vector<SColor> map;
	int xsize;
	int nxsize;
	int ysize;
	int nysize;

	void LoadMap(const unsigned char* buf, int num);

	static std::deque<CColorMap> colorMaps;
	static std::map<std::string, CColorMap*> colorMapsMap;
};

#endif // COLOR_MAP_H
