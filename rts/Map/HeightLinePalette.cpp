/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "HeightLinePalette.h"
#include "System/Config/ConfigHandler.h"



CONFIG(bool, ColorElev).defaultValue(true).description("If heightmap (default hotkey [F1]) should be colored or not.");


static std::array<SColor, 256> CreateColored()
{
	std::array<SColor, 256> arr;
	for(int a = 0; a < 86; ++a) {
		arr[a].r = 255 - a*3;
		arr[a].g = a*3;
		arr[a].b = 0;
	}
	for (int a = 86; a < 172; ++a) {
		arr[a].r = 0;
		arr[a].g = 255 - (a - 86) * 3;
		arr[a].b = (a - 86) * 3;
	}
	for (int a = 172; a < 256; ++a) {
		arr[a].r = (a - 172) * 3;
		arr[a].g = 0;
		arr[a].b = 255 - (a - 172) * 3;
	}
	for (SColor& c: arr) {
		c.r = (64 + c.r) >> 1;
		c.g = (64 + c.g) >> 1;
		c.b = (64 + c.b) >> 1;
	}
	return arr;
}

static std::array<SColor, 256> CreateBW()
{
	std::array<SColor, 256> arr;
	for (int a = 0; a < 29; ++a) {
		arr[a].r = 255 - a*8;
		arr[a].g = 255 - a*8;
		arr[a].b = 255 - a*8;
	}
	for (int a = 29; a < 256; ++a) {
		arr[a].r = a;
		arr[a].g = a;
		arr[a].b = a;
	}
	for (SColor& c: arr) {
		c.r = 64 + (c.r >> 1);
		c.g = 64 + (c.g >> 1);
		c.b = 64 + (c.b >> 1);
	}
	return arr;
}


std::array<SColor, 256> CHeightLinePalette::paletteColored       = CreateColored();
std::array<SColor, 256> CHeightLinePalette::paletteBlackAndWhite = CreateBW();


const SColor* CHeightLinePalette::GetData()
{
	if (configHandler->GetBool("ColorElev")) {
		return &paletteColored[0];
	} else {
		return &paletteBlackAndWhite[0];
	}
}
