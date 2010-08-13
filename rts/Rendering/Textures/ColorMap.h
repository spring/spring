/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COLOR_MAP_H
#define COLOR_MAP_H

#include <string>
#include <vector>
#include <list>
#include <map>

#include "creg/creg_cond.h"

/**
 * Simple class to interpolate between 32bit RGBA colors
 * Do not delete an instance of this class created by any Load function,
 * they are deleted automaticly.
 */
class CColorMap
{
	CR_DECLARE(CColorMap);
public:
	CColorMap();
	/// loads from a float vector
	CColorMap(std::vector<float>& vec);
	/// loads from a filename
	CColorMap(std::string filename);
	/// loads from a char buffer as one dimensional array
	CColorMap(const unsigned char* buf, int num);
	~CColorMap();

	/// load colormap from a bitmap
	static CColorMap* LoadFromBitmapFile(std::string filename);

	/// takes a vector with float value 0.0-1.0, 4 values makes up a rgba color
	static CColorMap* LoadFromFloatVector(std::vector<float>& vec);

	/**
	 * load from a string containing a numbef
	 * or float values example: "1.0 0.5 1.0 ... "
	 */
	static CColorMap* LoadFromFloatString(std::string fstring);

	static CColorMap* LoadFromDefString(std::string dstring);

	/// to load default values...
	static CColorMap* Load8f(float r1, float g1, float b1, float a1, float r2, float g2, float b2, float a2);
	static CColorMap* Load12f(float r1, float g1, float b1, float a1, float r2, float g2, float b2, float a2, float r3, float g3, float b3, float a3);

	/**
	 * color - buffer with room for 4 bytes
	 * pos - value between 0 and 1, returns pointer to color
	 */
	unsigned char* GetColor(unsigned char* color, float pos);

	static void DeleteColormaps();

protected:
	unsigned char* map;
	int xsize, nxsize;
	int ysize, nysize;

	void LoadMap(const unsigned char* buf, int num);

	static std::vector<CColorMap*> colorMaps;
	static std::map<std::string, CColorMap*> colorMapsMap;
};

#endif // COLOR_MAP_H
