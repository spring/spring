#include "StdAfx.h"
// OutlineFont.cpp: implementation of the COutlineFont class.
//
//////////////////////////////////////////////////////////////////////

#include "OutlineFont.h"
#include <string>
#include "Rendering/glFont.h"

/******************************************************************************/


COutlineFont outlineFont;


COutlineFont::COutlineFont() : enabled(true)
{
}


COutlineFont::~COutlineFont()
{
}


void COutlineFont::print(float xps, float yps,
                         const float color[4], const char* text) const
{
	const float luminance = (color[0] * 0.299f) +
	                        (color[1] * 0.587f) +
	                        (color[2] * 0.114f);
	                        
	const float darkOutline[4]  = { 0.25f, 0.25f, 0.25f, 0.8f };
	const float lightOutline[4] = { 0.85f, 0.85f, 0.85f, 0.8f };

	const float* outlineColor;
	if (luminance > 0.25f) {
		outlineColor = darkOutline;
	} else {
		outlineColor = lightOutline;
	}
	font->glPrintOutlined(text, xps, yps, color, outlineColor);
}
