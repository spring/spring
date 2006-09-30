#include "StdAfx.h"
// OutlineFont.cpp: implementation of the COutlineFont class.
//
//////////////////////////////////////////////////////////////////////

#include "OutlineFont.h"
#include <string>
#include "Rendering/glFont.h"


/******************************************************************************/


COutlineFont outlineFont;


COutlineFont::COutlineFont() : enabled(false)
{
}


COutlineFont::~COutlineFont()
{
}


void COutlineFont::print(float xps, float yps,
                         const float color[4], const char* text) const
{
	if (!enabled) {
		glColor4fv(color);
		font->glPrint("%s", text);
		return;
	}

	// strip any color codes
	std::string nocolor;
	const int len = (int)strlen(text);
	for (int i = 0; i < len; i++) {
		if ((unsigned char)text[i] == 255) {
			i = i + 3;
		} else {
			nocolor += text[i];
		}
	}
	text = nocolor.c_str();

	const float luminance = (color[0] * 0.299f) +
	                        (color[1] * 0.587f) +
	                        (color[2] * 0.114f);
	if (luminance > 0.4f) {
		glColor4f(0.25f, 0.25f, 0.25f, 0.8f);
	} else {
		glColor4f(0.85f, 0.85f, 0.85f, 0.8f);
	}
	glTranslatef(0.0f, +yps, 0.0f); font->glPrint("%s", text);
	glTranslatef(+xps, 0.0f, 0.0f); font->glPrint("%s", text);
	glTranslatef(0.0f, -yps, 0.0f); font->glPrint("%s", text);
	glTranslatef(0.0f, -yps, 0.0f); font->glPrint("%s", text);
	glTranslatef(-xps, 0.0f, 0.0f); font->glPrint("%s", text);
	glTranslatef(-xps, 0.0f, 0.0f); font->glPrint("%s", text);
	glTranslatef(0.0f, +yps, 0.0f); font->glPrint("%s", text);
	glTranslatef(0.0f, +yps, 0.0f); font->glPrint("%s", text);
	glTranslatef(+xps, -yps, 0.0f);
	if (color[3] > 0.0f) {
		glColor4fv(color);
		font->glPrint("%s", text);
	}
}
