/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AIColor.h"

#include "System/MainDefines.h"

#include <string>
#include <cassert>

springai::AIColor::AIColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
	: r(r), g(g), b(b), a(a)
{
}

springai::AIColor::AIColor()
	: r(0), g(0), b(0), a(1)
{
}

/*springai::AIColor::AIColor(short _r, short _g, short _b, short _a)
	: r(_r), g(_g), b(_b), a(_a)
{
	assert(_r < 256);
	assert(_g < 256);
	assert(_b < 256);
	assert(_a < 256);
}*/

springai::AIColor::AIColor(float _r, float _g, float _b, float _a)
	: r((unsigned char)(_r * 255))
	, g((unsigned char)(_g * 255))
	, b((unsigned char)(_b * 255))
	, a((unsigned char)(_a * 255))
{
	assert((_r <= 1.0f) && (_r >= 0.0f));
	assert((_g <= 1.0f) && (_g >= 0.0f));
	assert((_b <= 1.0f) && (_b >= 0.0f));
	assert((_a <= 1.0f) && (_a >= 0.0f));
}


void springai::AIColor::LoadInto3(short* rgb) const
{
	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;
}

void springai::AIColor::LoadInto4(short* rgba) const
{
	rgba[0] = r;
	rgba[1] = g;
	rgba[2] = b;
	rgba[4] = a;
}


std::string springai::AIColor::ToString() const
{
	char resBuff[32];
	SNPRINTF(resBuff, sizeof(resBuff), "(%u, %u, %u, %u)", r, g, b, a);
	return std::string(resBuff);
}

const springai::AIColor springai::AIColor::NULL_VALUE;
