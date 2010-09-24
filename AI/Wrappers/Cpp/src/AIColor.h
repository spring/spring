/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CPPWRAPPER_AI_COLOR_H
#define _CPPWRAPPER_AI_COLOR_H

#include <string>

namespace springai {

/**
 * Represents a position on the map.
 */
class AIColor {
public:

	/// Black
	AIColor();

	/// Range of values: [0, 255]
	AIColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255);

	/// Range of values: [0, 255]
	//AIColor(short r, short g, short b, short a = 255);

	/// Range of values: [0.0f, 1.0f]
	AIColor(float r, float g, float b, float a = 1.0f);

	//AIColor(float* rgba);
	//AIColor(unsigned char* rgba);
	//AIColor(float* rgba);

	void LoadInto3(short* rgb) const;
	void LoadInto4(short* rgba) const;

	virtual std::string ToString() const;

	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;

	static const AIColor NULL_VALUE;
}; // class AIColor

}  // namespace springai

#endif // _CPPWRAPPER_AI_COLOR_H
