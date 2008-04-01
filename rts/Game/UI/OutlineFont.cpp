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
