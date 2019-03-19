/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COLOR_MAP_H
#define COLOR_MAP_H

#include <string>
#include <vector>

#include "System/Color.h"

/**
 * Simple class to interpolate between 32bit RGBA colors
 * Do not delete an instance of this class created by any Load function,
 * they are deleted automaticly.
 */
class CColorMap
{
public:
	CColorMap() = default;
	/// Loads from a float vector
	CColorMap(const float* data, size_t size);
	/// Loads from a file
	CColorMap(const std::string& fileName);

	CColorMap(const CColorMap&) = delete;
	CColorMap(CColorMap&& cm) { *this = std::move(cm); }

	CColorMap& operator = (const CColorMap&) = delete;
	CColorMap& operator = (CColorMap&&) = default;

public:
	/**
	 * @param color buffer with room for 4 bytes
	 * @param pos value between 0.0f and 1.0f, returns pointer to color
	 */
	void GetColor(unsigned char* color, float pos);
	void Clear() {
		id = -1;

		xsize = 2; nxsize = 1;
		ysize = 1; nysize = 0;

		map.clear();
	}

private:
	void LoadMap(const unsigned char* buf, int num);

public:
	static void InitStatic();

	// creg
	static CColorMap* GetColorMapByID(int id);

	/// Load colormap from a bitmap
	static CColorMap* LoadFromBitmapFile(const std::string& fileName);

	/// Takes a vector with float value 0.0-1.0, every 4 values make up a (RGBA) color
	static CColorMap* LoadFromFloatVector(const std::vector<float>& vec) { return LoadFromRawVector(vec.data(), vec.size()); }
	static CColorMap* LoadFromRawVector(const float* data, size_t size);

	/**
	 * Load from a string containing a number of float values or filename.
	 * example: "1.0 0.5 1.0 ... "
	 */
	static CColorMap* LoadFromDefString(const std::string& defString);

public:
	// directly accessible for creg
	int id = -1;

private:
	int  xsize = 2;
	int nxsize = 1;
	int  ysize = 1;
	int nysize = 0;

	std::vector<SColor> map;
};

#endif // COLOR_MAP_H
